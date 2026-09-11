// Trafico ADS-B masticado para la Pico.
//
// A diferencia de /api/sky, que devuelve el JSON crudo de adsb.fi, esto
// devuelve texto plano: una linea por avion, campos separados por "|". Dos
// razones, las dos del lado de la placa:
//
//   - No hace falta un parser de JSON en C. Se corta por "\n" y por "|" y
//     listo.
//   - La respuesta pasa de decenas de kilobytes a dos o tres. En la Pico el
//     buffer de TLS es lo mas caro que hay, asi que cada kilobyte cuenta.
//
// Ademas resuelve el origen y el destino de cada vuelo. Eso en la web son
// hasta cincuenta pedidos sueltos (ver fillRoutes en docs/radar.js): desde la
// placa seria imposible, asi que se hacen aca, en paralelo y con cache.
//
// Formato de la respuesta:
//
//   #1 <epoch utc> <tz en minutos> <fuente> <cantidad>
//   hex|vuelo|matricula|tipo|lat|lon|alt|gs|track|origen|destino
//
// lat y lon van en grados por 10000 y enteros, igual que adentro del
// firmware: la placa no tiene que tocar un solo float para ubicar un avion.
// Los campos que no se saben van vacios.

const RUTAS = new Map();          // callsign -> "ORI|DES", mientras viva el lambda
const RUTAS_MAX = 4000;


function limpiar(v) {
  return (v == null ? "" : String(v)).trim().replace(/[|\n\r]/g, "");
}

// adsb.fi y adsb.lol no coinciden en donde ponen la altitud.
function altitud(a) {
  let v = a.alt_baro;
  if (v == null) v = a.alt;
  if (v == null) v = a.alt_geom;
  if (v === "ground") return 0;
  const n = Number(v);
  return Number.isFinite(n) ? Math.round(n) : "";
}

function entero(v, escala = 1) {
  const n = Number(v);
  return Number.isFinite(n) ? Math.round(n * escala) : "";
}

// Un indicativo sirve para buscar la ruta si tiene forma de vuelo comercial:
// dos o tres letras de aerolinea y despues numeros. Una matricula (LV-S100,
// que ADS-B manda como LVS100) no sirve y solo gasta un pedido.
function claveDeRuta(vuelo) {
  const cs = vuelo.toUpperCase().replace(/[^A-Z0-9]/g, "");
  if (cs.length < 4 || cs.length > 8) return null;
  return /^[A-Z]{2,3}\d{1,4}[A-Z]?$/.test(cs) ? cs : null;
}

async function pedirJson(url, ms) {
  const r = await fetch(url, {
    headers: { accept: "application/json", "user-agent": "pico-radar/1.0" },
    signal: AbortSignal.timeout(ms),
  });
  if (!r.ok) throw new Error("http " + r.status);
  return r.json();
}

// La base de rutas esta partida en carpetas por las dos primeras letras del
// indicativo. Devuelve "ORI|DES" en IATA, o "|" si no esta.
async function buscarRuta(cs) {
  if (RUTAS.has(cs)) return RUTAS.get(cs);
  let par = "|";
  try {
    const d = await pedirJson(
      "https://vrs-standing-data.adsb.lol/routes/" + cs.slice(0, 2) + "/" + cs + ".json",
      3500
    );
    const iata = String(d._airport_codes_iata || "")
      .split("-")
      .map((x) => x.trim().toUpperCase())
      .filter((x) => x.length === 3);
    // Con escalas vienen tres o mas: interesan las puntas.
    if (iata.length >= 2) par = iata[0] + "|" + iata[iata.length - 1];
  } catch (e) {
    /* queda sin ruta: no es un error, hay vuelos que no estan en la base */
  }
  if (RUTAS.size >= RUTAS_MAX) RUTAS.clear();
  RUTAS.set(cs, par);
  return par;
}

export default async function handler(req, res) {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "*");
  if (req.method === "OPTIONS") {
    res.status(204).end();
    return;
  }
  res.setHeader("Content-Type", "text/plain; charset=utf-8");

  const lat = Number(req.query.lat);
  const lon = Number(req.query.lon);
  const dist = Math.min(250, Math.max(15, Math.round(Number(req.query.dist) || 80)));
  // Cuantos aviones como mucho. La placa guarda 32 (RADAR_MAX_AVIONES).
  const tope = Math.min(48, Math.max(1, Math.round(Number(req.query.n) || 32)));
  // Minutos que hay que sumarle al UTC para la hora local del equipo.
  const tz = Math.min(840, Math.max(-720, Math.round(Number(req.query.tz) || 0)));

  if (!Number.isFinite(lat) || !Number.isFinite(lon)) {
    res.status(400).send("#1 0 0 error 0\n");
    return;
  }

  const fuentes = [
    ["https://opendata.adsb.fi/api/v2/lat/" + lat + "/lon/" + lon + "/dist/" + dist, "adsb.fi", 8000],
    ["https://api.adsb.lol/v2/lat/" + lat + "/lon/" + lon + "/dist/" + dist, "adsb.lol", 7000],
  ];

  let lista = null, fuente = "";
  for (const [url, nombre, ms] of fuentes) {
    try {
      const data = await pedirJson(url, ms);
      const l = (data && (data.ac || data.aircraft)) || [];
      if (Array.isArray(l)) { lista = l; fuente = nombre; break; }
    } catch (e) {
      /* la que sigue */
    }
  }
  if (!lista) {
    res.status(502).send("#1 " + Math.floor(Date.now() / 1000) + " " + tz + " caido 0\n");
    return;
  }

  // Los que no reportan posicion no se pueden dibujar. Los mas cercanos
  // primero: si sobran, los que se cortan son los del borde del alcance.
  const utiles = lista
    .filter((a) => a && a.lat != null && a.lon != null)
    .sort((a, b) => (Number(a.dst) || 1e9) - (Number(b.dst) || 1e9))
    .slice(0, tope);

  // Las rutas, todas de una. Las que ya estan en cache no salen a la red.
  const claves = [...new Set(utiles.map((a) => claveDeRuta(limpiar(a.flight))).filter(Boolean))];
  const rutas = new Map();
  await Promise.all(
    claves.map(async (cs) => { rutas.set(cs, await buscarRuta(cs)); })
  );

  const lineas = utiles.map((a) => {
    const vuelo = limpiar(a.flight);
    const matricula = limpiar(a.r);
    const cs = claveDeRuta(vuelo);
    const ruta = (cs && rutas.get(cs)) || "|";
    return [
      limpiar(a.hex),
      vuelo || matricula || limpiar(a.hex),
      matricula,
      limpiar(a.t),
      entero(a.lat, 10000),
      entero(a.lon, 10000),
      altitud(a),
      entero(a.gs),
      entero(a.track),
      ruta,                        // ya son dos campos: "ORI|DES"
    ].join("|");
  });

  // El mismo tiempo de cache que /api/sky: la placa pide cada quince o veinte
  // segundos y no tiene sentido castigar a las fuentes por eso.
  res.setHeader("Cache-Control", "s-maxage=12, stale-while-revalidate=45");
  res.status(200).send(
    "#1 " + Math.floor(Date.now() / 1000) + " " + tz + " " + fuente + " " + lineas.length + "\n" +
    lineas.join("\n") + (lineas.length ? "\n" : "")
  );
}
