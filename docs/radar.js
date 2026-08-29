/* Pico Radar v14 — GitHub Pages CRT. Proxy CORS + catálogo local. */
const PUBLIC_SKY = "https://pico-vga-radar-sky.vercel.app/api";
const SKY_API = (function () {
  const q = new URLSearchParams(location.search).get("api");
  if (q) return q.replace(/\/$/, "");
  if (location.hostname === "localhost" || location.hostname === "127.0.0.1") return location.origin + "/api";
  return PUBLIC_SKY;
})();
const THEMES = {
  crt_amber: { fg: "#e8b86d", bg: "#0a0805" },
  crt_green: { fg: "#7dff7a", bg: "#031105" },
  atc_dark: { fg: "#7ec8e3", bg: "#07090c" },
  navy: { fg: "#8ab4ff", bg: "#061018" },
  red: { fg: "#ff6b4a", bg: "#120606" },
  ice: { fg: "#d9f6ff", bg: "#081016" },
};
const SEED_AP = [
  ["EZE", "Buenos Aires (Ezeiza)", -34.822, -58.536, "Argentina", "Ezeiza"],
  ["AEP", "Buenos Aires", -34.559, -58.416, "Argentina", "Aeroparque"],
  ["FDO", "Buenos Aires", -34.453, -58.59, "Argentina", "San Fernando"],
  ["ASU", "Asuncion", -25.24, -57.52, "Paraguay", "Silvio Pettirossi"],
  ["SYD", "Sydney", -33.946, 151.177, "Australia", "Kingsford Smith"],
  ["SIN", "Singapur", 1.364, 103.991, "Singapore", "Changi"],
  ["NRT", "Tokio", 35.765, 140.386, "Japan", "Narita"],
  ["HND", "Tokio", 35.549, 139.78, "Japan", "Haneda"],
  ["KIX", "Osaka", 34.427, 135.244, "Japan", "Kansai"],
  ["RAI", "Praia", 14.941, -23.485, "Cape Verde", "Nelson Mandela"],
  ["SID", "Sal", 16.741, -22.949, "Cape Verde", "Amilcar Cabral"],
  ["BVC", "Boa Vista", 16.137, -22.889, "Cape Verde", "Rabil"],
  ["DAC", "Dhaka", 23.843, 90.398, "Bangladesh", "Hazrat Shahjalal"],
  ["GRU", "Sao Paulo", -23.436, -46.473, "Brazil", "Guarulhos"],
  ["SCL", "Santiago", -33.393, -70.786, "Chile", "Arturo Merino"],
  ["MIA", "Miami", 25.796, -80.287, "United States", "Miami"],
  ["JFK", "Nueva York", 40.641, -73.778, "United States", "JFK"],
  ["LHR", "Londres", 51.47, -0.454, "United Kingdom", "Heathrow"],
  ["MAD", "Madrid", 40.472, -3.563, "Spain", "Barajas"],
];
const SEED_AL = [
  ["ARG", "AR", "Aerolineas Argentinas", "Argentina"],
  ["JES", "JA", "JetSMART", "Chile"],
  ["FBZ", "FO", "Flybondi", "Argentina"],
  ["LAN", "LA", "LATAM", "Chile"],
  ["CMP", "CM", "Copa", "Panama"],
  ["AVA", "AV", "Avianca", "Colombia"],
  ["AFR", "AF", "Air France", "France"],
  ["ANS", "OY", "Andes Lineas Aereas", "Argentina"],
  ["TCV", "VR", "Cabo Verde Airlines", "Cape Verde"],
  ["QFA", "QF", "Qantas", "Australia"],
  ["SIA", "SQ", "Singapore Airlines", "Singapore"],
  ["UAL", "UA", "United", "United States"],
  ["DAL", "DL", "Delta", "United States"],
  ["AAL", "AA", "American", "United States"],
  ["IBE", "IB", "Iberia", "Spain"],
  ["UAE", "EK", "Emirates", "UAE"],
  ["BAW", "BA", "British Airways", "United Kingdom"],
  ["THY", "TK", "Turkish Airlines", "Turkey"],
  ["GLO", "G3", "GOL", "Brazil"],
];
const ALIAS = {
  EZE: "ezeiza pistarini",
  AEP: "aeroparque newbery",
  ASU: "asuncion pettirossi",
  RAI: "cabo verde cape verde praia",
  SID: "cabo verde cape verde sal",
  BVC: "cabo verde cape verde",
  NRT: "tokio tokyo narita",
  HND: "tokio tokyo haneda",
  KIX: "osaka kansai",
  SIN: "singapur singapore changi",
  SYD: "sydney sidney",
};

let AIRPORTS = SEED_AP.slice();
let ALINES = SEED_AL.slice();
const ROUTECACHE = {};
const POS = {};
let home = null, demoOver = false, srcLabel = "";
let RAW = [], AC = [], running = false, looping = false;
let view = "hybrid", theme = "crt_amber", listStyle = "fa";
let center = [-34.822, -58.536], includeGround = true, radiusKm = 150, maxN = 16, rotateS = 7;
let beam = -Math.PI / 2, lastSweep = 0, lastRot = 0, page = 0, loading = false;
let followAl = "", followFlt = "";
let want = { air: true, ga: false, biz: false, heli: false };
let lastOk = null;

const radar = document.getElementById("radar");
const wall = document.getElementById("wall");
const loadEl = document.getElementById("load");
const statusEl = document.getElementById("status");
const monMeta = document.getElementById("monMeta");
const foot = document.getElementById("foot");

function fold(s) {
  return (s || "")
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, " ")
    .trim();
}
function rewrite(q) {
  let f = fold(q);
  f = f.replace(/\btokio\b/g, "tokyo").replace(/\bsingapur\b/g, "singapore").replace(/\bcabo verde\b/g, "cape verde");
  if (f === "cabo" || f === "cape") f = "cape verde";
  return f;
}
function hay(a) {
  const base = Array.isArray(a) ? a.join(" ") : "";
  const extra = ALIAS[a[0]] || "";
  return fold(base + " " + extra);
}
function letters(s) { return (s || "").toUpperCase().replace(/[^A-Z]/g, ""); }
function compactId(s) { return (s || "").toUpperCase().replace(/[^A-Z0-9]/g, ""); }
function viewLabel() {
  return view === "hybrid" ? "Hibrido" : view === "radar" ? "Radar" : "Pared";
}
function setMeta() {
  monMeta.textContent = AC.length + " · " + radiusKm + " km · " + viewLabel();
}
function airlineOf(cs) {
  const L = letters(cs);
  const hit = ALINES.find((a) => a[0] === L.slice(0, 3));
  if (hit) return [hit[1] || hit[0].slice(0, 2), hit[2], "#334155"];
  return ["", L.slice(0, 3) || "ACFT", "#334155"];
}
function kindOf(a) {
  const t = (a.t || "").toUpperCase();
  const cs = (a.flight || "").toUpperCase().trim();
  const compact = compactId(cs);
  const L = letters(cs);
  const known = ALINES.some((x) => x[0] === L.slice(0, 3));
  if (/^(R44|R66|B06|B407|H125|H135)/.test(t)) return "heli";
  if (/^(LJ|C25|C56|GLF|GLEX)/.test(t) && !known) return "biz";
  if (known || /^(A3|A2|B7|B38|E1|E19|CRJ)/.test(t)) return "air";
  if (/^[A-Z]{3}\d/.test(compact) || (/^[A-Z]{2}\d/.test(compact) && compact[0] !== "N")) return "air";
  if (cs.startsWith("LV") && !known) return "ga";
  return "ga";
}
function fmtAlt(alt) {
  if (alt === "ground" || alt == null || alt === 0) return "GND";
  const n = Number(alt);
  if (!n) return "-";
  return n >= 1000 ? (n / 1000).toFixed(1) + "k ft" : Math.round(n) + " ft";
}
function ktToMph(gs) { return gs == null ? "-" : Math.round(gs * 1.15078) + " mph"; }
function hhmm(ts) {
  if (!ts) return "--:--";
  const d = new Date(ts);
  return String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
}
function kmLL(a, b) {
  return Math.hypot((a[0] - b[0]) * 111, (a[1] - b[1]) * 111 * Math.cos((a[0] * Math.PI) / 180));
}
function aptCoord(code) {
  const a = AIRPORTS.find((x) => x[0] === code);
  return a ? [a[2], a[3]] : null;
}
function setLoad(show, msg) {
  if (msg) statusEl.textContent = msg;
  loadEl.classList.toggle("on", !!show);
}
function applyTheme() {
  const pair = THEMES[theme] || THEMES.crt_amber;
  const mon = document.getElementById("monitor");
  mon.style.setProperty("--scope-fg", pair.fg);
  mon.style.setProperty("--scope-bg", pair.bg);
  mon.style.background = pair.bg;
  mon.style.color = pair.fg;
}
function currentApt() {
  const q = document.getElementById("preset").value || "EZE";
  const up = q.trim().toUpperCase();
  if (up === "CASA" && home) return ["CASA", "Casa", home[0], home[1], "AR", "Casa"];
  const code = up.slice(0, 3);
  const exact = AIRPORTS.find((a) => a[0] === code && (up.length === 3 || up.startsWith(a[0] + " ") || up === a[0]));
  if (exact && /^[A-Z]{3}\b/.test(up)) return exact;
  const s = rewrite(q);
  const scored = AIRPORTS.map((a) => {
    const h = hay(a);
    const city = fold(a[1]);
    let n = 0;
    if (a[0].toLowerCase() === s) n = 100;
    else if (a[0].toLowerCase().startsWith(s)) n = 92;
    else if (city === s) n = 88;
    else if (h.includes(s)) n = city.includes(s) ? 75 : fold(a[4]).includes(s) ? 40 : 60;
    return { a, n };
  }).filter((x) => x.n).sort((x, y) => y.n - x.n);
  return (scored[0] && scored[0].a) || AIRPORTS[0];
}

function bindSuggest(input, listEl, getRows, fmt, onPick) {
  function show(q) {
    const s = rewrite(q);
    const rows = getRows();
    const hits = (!s ? rows.slice(0, 8) : rows.filter((r) => hay(r).includes(s) || fold(r[0]).startsWith(s)).slice(0, 12));
    listEl.innerHTML = hits.map((r) => '<button type="button" data-v="' + r[0] + '">' + fmt(r) + "</button>").join("")
      || '<button type="button" disabled>Sin resultados</button>';
    listEl.style.display = "block";
  }
  input.addEventListener("input", () => show(input.value));
  input.addEventListener("focus", () => show(input.value));
  listEl.addEventListener("mousedown", (e) => {
    const b = e.target.closest("button");
    if (!b || !b.dataset.v) return;
    onPick(b.dataset.v, b.textContent);
    listEl.style.display = "none";
  });
  document.addEventListener("click", (e) => {
    if (!listEl.contains(e.target) && e.target !== input) listEl.style.display = "none";
  });
}
bindSuggest(
  document.getElementById("preset"),
  document.getElementById("sugAp"),
  () => AIRPORTS,
  (r) => "<b>" + r[0] + "</b> " + (r[1] || r[5]) + " · " + (r[4] || ""),
  (code) => {
    const a = AIRPORTS.find((x) => x[0] === code) || SEED_AP[0];
    document.getElementById("preset").value = a[0] + " — " + a[1] + " · " + a[4];
    grabForm();
    loadLive(true);
  },
);
bindSuggest(
  document.getElementById("followAl"),
  document.getElementById("sugAl"),
  () => ALINES,
  (r) => r[2] + (r[1] ? " (" + r[1] + ")" : "") + " · " + r[0],
  (code, label) => {
    document.getElementById("followAl").value = label;
    followAl = code;
    applyFilters();
    renderWall();
  },
);

function enrich(a) {
  const now = Date.now();
  const key = compactId(a.flight);
  const cached = ROUTECACHE[key];
  if (cached) { a.origin = cached[0]; a.dest = cached[1]; }
  const gnd = a.alt === "ground" || a.alt === 0 || (a.gs != null && a.gs < 30 && (Number(a.alt) || 0) < 800);
  if (gnd) {
    a.status = "EN TIERRA"; a.depKind = "EST"; a.arrKind = "EST";
    a.depAt = now + 25 * 60000; a.arrAt = a.depAt + 80 * 60000; a.pct = 6;
    return a;
  }
  a.status = "EN RUTA"; a.depKind = "EST"; a.arrKind = "EST";
  const destLL = aptCoord(a.dest);
  const origLL = aptCoord(a.origin);
  const gs = Number(a.gs) || 0;
  if (destLL && gs > 40) {
    const rem = kmLL([a.lat, a.lon], destLL);
    const minRem = Math.max(8, Math.round((rem / (gs * 1.852)) * 60));
    a.arrAt = now + minRem * 60000;
    if (origLL) {
      const tot = Math.max(rem + 20, kmLL(origLL, destLL));
      const done = Math.max(0.06, Math.min(0.94, 1 - rem / tot));
      a.depAt = now - Math.round(minRem * (done / (1 - done))) * 60000;
      a.pct = Math.round(done * 100);
    } else {
      a.depAt = now - 40 * 60000;
      a.pct = 40;
    }
  } else { a.depAt = null; a.arrAt = null; a.pct = 22; }
  return a;
}
function applyFilters() {
  const radiusNm = radiusKm / 1.852;
  const alQ = (document.getElementById("followAl").value || "").trim();
  const alHit = alQ ? ALINES.find((a) => fold(a.join(" ")).includes(rewrite(alQ)) || a[0] === alQ.toUpperCase().slice(0, 3)) : null;
  const alCode = alHit ? alHit[0] : letters(alQ).slice(0, 3);
  AC = RAW.filter((a) => {
    if (!includeGround && (a.alt === "ground" || a.alt === 0)) return false;
    if (a.dst != null && a.dst > radiusNm * 1.08) return false;
    if (!want[kindOf(a)]) return false;
    if (alCode && !letters(a.flight).startsWith(alCode)) return false;
    if (followFlt && !compactId(a.flight).includes(compactId(followFlt))) return false;
    return a.lat != null;
  }).sort((a, b) => (a.dst || 99) - (b.dst || 99)).slice(0, maxN);
  AC.forEach(enrich);
  if (!AC.length && RAW.length && !alQ && !followFlt) {
    AC = RAW.slice().sort((a, b) => (a.dst || 99) - (b.dst || 99)).slice(0, maxN);
    AC.forEach(enrich);
  }
  setMeta();
}
async function pullJson(url, ms) {
  const ctrl = new AbortController();
  const t = setTimeout(() => ctrl.abort(), ms || 9000);
  try {
    const res = await fetch(url, { signal: ctrl.signal });
    if (!res.ok) throw new Error("bad " + res.status);
    return await res.json();
  } finally { clearTimeout(t); }
}
function skyUrls(lat, lon, nm) {
  const a = "https://opendata.adsb.fi/api/v2/lat/" + lat + "/lon/" + lon + "/dist/" + nm;
  const b = "https://api.adsb.lol/v2/lat/" + lat + "/lon/" + lon + "/dist/" + nm;
  return [
    SKY_API + "/sky?lat=" + lat + "&lon=" + lon + "&dist=" + nm,
    a,
    b,
    "https://corsproxy.io/?" + encodeURIComponent(a),
    "https://api.allorigins.win/raw?url=" + encodeURIComponent(a),
    "https://api.codetabs.com/v1/proxy?quest=" + encodeURIComponent(a),
  ];
}
function normList(data) {
  const list = data && (data.ac || data.aircraft);
  return Array.isArray(list) ? list : [];
}
async function fetchSky(lat, lon, nm) {
  let lastErr;
  for (const u of skyUrls(lat, lon, nm)) {
    try {
      const data = await pullJson(u, u.indexOf("vercel.app") >= 0 ? 12000 : 8000);
      const list = normList(data);
      if (data && Array.isArray(list)) {
        const src = data.source || (u.indexOf("adsb.fi") >= 0 ? "adsb.fi" : u.indexOf("adsb.lol") >= 0 ? "adsb.lol" : "vivo");
        return { list, src };
      }
    } catch (e) { lastErr = e; }
  }
  throw lastErr || new Error("sin cielo");
}
async function fillRoutes(list) {
  const pending = list.filter((a) => a.flight && !ROUTECACHE[compactId(a.flight)]).slice(0, 10);
  await Promise.all(pending.map(async (a) => {
    const cs = compactId(a.flight);
    try {
      const d = await pullJson("https://vrs-standing-data.adsb.lol/routes/" + cs.slice(0, 2) + "/" + cs + ".json", 4000);
      const iata = (d._airport_codes_iata || "").split("-").filter((x) => x && x.length <= 4);
      if (iata.length >= 2) { ROUTECACHE[cs] = [iata[0], iata[iata.length - 1]]; a.origin = iata[0]; a.dest = iata[iata.length - 1]; }
    } catch (e) { /* optional */ }
  }));
}
function norm(a) {
  return {
    hex: a.hex,
    flight: (a.flight || a.r || a.hex || "").toString().trim(),
    r: a.r, t: a.t, lat: a.lat, lon: a.lon,
    alt: a.alt_baro != null ? a.alt_baro : (a.alt != null ? a.alt : a.alt_geom),
    gs: a.gs, track: a.track, dst: a.dst,
  };
}
async function loadLive(showOverlay) {
  if (loading) return;
  loading = true;
  const apt = currentApt();
  center = [apt[2], apt[3]];
  if (showOverlay) setLoad(true, "Cargando " + apt[0]);
  const nm = Math.max(25, Math.round(radiusKm / 1.852) + 8);
  try {
    const got = await fetchSky(center[0], center[1], nm);
    srcLabel = got.src;
    lastOk = { list: got.list, src: got.src, at: Date.now() };
    RAW = got.list.map(norm).filter((a) => a.lat != null && a.hex);
    applyFilters();
    renderWall();
    fillRoutes(AC).then(() => { applyFilters(); renderWall(); });
    const now = performance.now();
    AC.forEach((a) => { POS[a.hex] = { lat: a.lat, lon: a.lon, t: now, gs: a.gs, track: a.track }; });
    lastSweep = now;
    if (!RAW.length) statusEl.textContent = "0 aeronaves en " + apt[0] + " (radio " + radiusKm + " km). No hay cobertura ADS-B o no hay trafico ahora.";
    else if (followAl && !AC.length) statusEl.textContent = RAW.length + " aviones en zona, 0 de " + followAl;
    else statusEl.textContent = RAW.length + " aeronaves · " + apt[0] + " · " + srcLabel;
    foot.textContent = "ADS-B en vivo · " + srcLabel;
  } catch (e) {
    if (lastOk && Date.now() - lastOk.at < 15 * 60 * 1000) {
      srcLabel = lastOk.src + " (cache)";
      RAW = lastOk.list.map(norm).filter((a) => a.lat != null && a.hex);
      applyFilters(); renderWall();
      statusEl.textContent = RAW.length + " aeronaves · " + apt[0] + " · " + srcLabel;
    } else {
      RAW = []; AC = []; renderWall();
      statusEl.textContent = "No pude leer el cielo de " + apt[0] + ". Reintenta.";
      foot.textContent = "ADS-B en vivo · error";
    }
  } finally {
    loading = false;
    loadEl.classList.remove("on");
  }
}
function displayPos(a) {
  const p = POS[a.hex];
  if (!p || a.alt === "ground" || !p.gs || p.track == null) return { lat: a.lat, lon: a.lon };
  const dt = Math.min(40, (performance.now() - p.t) / 1000);
  const km = p.gs * 1.852 * dt / 3600;
  const rad = p.track * Math.PI / 180;
  return { lat: p.lat + Math.cos(rad) * km / 111, lon: p.lon + Math.sin(rad) * km / (111 * Math.cos(p.lat * Math.PI / 180)) };
}
function logoHTML(a) {
  const pair = airlineOf(a.flight);
  const iata = pair[0];
  const name = pair[1];
  const fb = '<div class="fb">' + (iata || name || "GA").slice(0, 2) + "</div>";
  if (!iata) return fb;
  return '<img alt="' + iata + '" src="' + SKY_API + "/logo?code=" + iata + '" onerror="this.outerHTML=this.dataset.fb" data-fb=\'' + fb + "'>";
}
function cardHTML(a) {
  const name = airlineOf(a.flight)[1];
  const pct = a.pct != null ? a.pct : 18;
  const origin = a.origin || currentApt()[0];
  const dest = a.dest || "---";
  const met = a.status === "EN TIERRA"
    ? "Preparandose para el despegue"
    : "Alt " + fmtAlt(a.alt) + " · " + ktToMph(a.gs) + (a.arrAt ? " · llega en " + Math.max(0, Math.round((a.arrAt - Date.now()) / 60000)) + " min" : "");
  return (
    '<div class="fa-card">' +
      '<div class="fa-row"><div class="logo">' + logoHTML(a) + "</div><div>" +
        '<div class="fa-top"><span class="fa-id">' + (a.flight || a.hex) + '</span><span class="fa-st">' + a.status + "</span></div>" +
        '<div class="fa-air">' + name + " · " + (a.t || a.r || "-") + "</div>" +
      "</div></div>" +
      '<div class="fa-route"><span>' + origin + '</span><span class="bar"><i style="width:' + pct + '%"></i></span><span>' + dest + "</span></div>" +
      '<div class="fa-times">' +
        '<div><span class="lbl">Despegue ' + a.depKind + "</span><b>" + hhmm(a.depAt) + "</b></div>" +
        '<div><span class="lbl">Aterrizaje ' + a.arrKind + "</span><b>" + hhmm(a.arrAt) + "</b></div>" +
      "</div>" +
      '<div class="fa-met">' + met + "</div>" +
    "</div>"
  );
}
function clipCards() {
  if (listStyle === "carousel" || listStyle === "fids") return;
  const box = wall.getBoundingClientRect();
  [].slice.call(wall.querySelectorAll(".fa-card")).forEach((c) => {
    const r = c.getBoundingClientRect();
    if (r.bottom > box.bottom + 2 || r.top < box.top - 2) c.style.display = "none";
  });
}
function visibleList() {
  if (!AC.length) return [];
  if (listStyle === "carousel") return [AC[page % AC.length]];
  const h = wall.clientHeight || 300;
  const n = listStyle === "fids" ? Math.max(1, Math.floor((h - 22) / 34)) : Math.max(1, Math.min(AC.length, 12));
  const start = (page * n) % Math.max(1, AC.length);
  const out = [];
  for (let i = 0; i < Math.min(n, AC.length); i++) out.push(AC[(start + i) % AC.length]);
  return out;
}
function overhead() {
  if (demoOver) return enrich({ hex: "demo", flight: "ARG1716", t: "A320", lat: center[0], lon: center[1], alt: 4200, gs: 210, track: 180, origin: "AEP", dest: "COR" });
  if (!document.getElementById("house").checked || !home) return null;
  let best = null, bestD = 9;
  AC.forEach((a) => { const d = kmLL([displayPos(a).lat, displayPos(a).lon], home); if (d < bestD) { bestD = d; best = a; } });
  return best;
}
function updateOver() {
  const el = document.getElementById("overCard");
  const a = overhead();
  if (!a || view === "wall") { el.style.display = "none"; return; }
  el.style.display = "block";
  el.innerHTML = cardHTML(a);
}
function renderWall() {
  applyTheme();
  updateOver();
  radar.style.display = view === "wall" ? "none" : "block";
  wall.classList.toggle("on", view !== "radar");
  wall.classList.toggle("hybrid", view === "hybrid");
  wall.classList.toggle("full", view === "wall");
  if (!AC.length) {
    wall.innerHTML = '<div class="empty"><div>' + (loading ? "Cargando el cielo…" : "Sin vuelos en este radio") + '</div><p class="fa-air">' + (statusEl.textContent || "") + "</p></div>";
    return;
  }
  const list = visibleList();
  if (listStyle === "fids") {
    wall.innerHTML = '<table class="fids"><thead><tr><th></th><th>VUELO</th><th>RUTA</th><th>DEP</th><th>ARR</th><th>ESTADO</th></tr></thead><tbody>' +
      list.map((a) => '<tr><td><div class="logo" style="width:28px;height:28px;font-size:10px">' + logoHTML(a) + "</div></td><td>" + (a.flight || "") + "</td><td>" + (a.origin || "") + " → " + (a.dest || "") + "</td><td>" + hhmm(a.depAt) + "</td><td>" + hhmm(a.arrAt) + "</td><td>" + (a.status === "EN TIERRA" ? "EN TIERRA" : "EN VUELO") + "</td></tr>").join("") +
      "</tbody></table>";
  } else if (listStyle === "carousel") {
    wall.innerHTML = '<div class="carousel-wrap">' + (list[0] ? cardHTML(list[0]) : "") + "</div>";
  } else {
    wall.innerHTML = list.map(cardHTML).join("");
  }
  requestAnimationFrame(clipCards);
}
function angDiff(a, b) {
  let d = a - b;
  while (d > Math.PI) d -= Math.PI * 2;
  while (d < -Math.PI) d += Math.PI * 2;
  return d;
}
function loop() {
  if (!running || view === "wall") { looping = false; return; }
  looping = true;
  const ctx = radar.getContext("2d");
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const w = radar.clientWidth || 640, h = radar.clientHeight || 480;
  radar.width = w * dpr; radar.height = h * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  const pair = THEMES[theme] || THEMES.crt_amber;
  ctx.fillStyle = pair.bg; ctx.fillRect(0, 0, w, h);
  const left = view === "hybrid" ? 0.54 : 1;
  const cx = w * left * 0.5, cy = h * 0.52;
  const R = Math.min(cx, cy) * 0.86;
  ctx.strokeStyle = pair.fg + "28"; ctx.lineWidth = 1;
  [0.25, 0.5, 0.75, 1].forEach((r) => { ctx.beginPath(); ctx.arc(cx, cy, R * r, 0, Math.PI * 2); ctx.stroke(); });
  ctx.beginPath(); ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy); ctx.stroke();
  beam += 0.012; if (beam > Math.PI * 3) beam -= Math.PI * 2;
  if (performance.now() - lastSweep > 55000) { lastSweep = performance.now(); loadLive(false); }
  if (performance.now() - lastRot > rotateS * 1000) { lastRot = performance.now(); page++; renderWall(); }
  ctx.strokeStyle = pair.fg; ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(cx + Math.cos(beam) * R, cy + Math.sin(beam) * R); ctx.stroke();
  ctx.fillStyle = pair.fg; ctx.font = "11px ui-monospace, monospace";
  ctx.fillText(currentApt()[0], 10, 18);
  const span = Math.max(0.25, radiusKm / 111);
  const labeled = new Set(visibleList().map((a) => a.hex));
  AC.forEach((a) => {
    const pos = displayPos(a);
    const x = cx + ((pos.lon - center[1]) / span) * R;
    const y = cy - ((pos.lat - center[0]) / span) * R;
    if (Math.abs(angDiff(Math.atan2(y - cy, x - cx), beam)) < 0.08) a.paint = 1;
    a.paint = Math.max(0.32, (a.paint || 0.32) - 0.0024);
    ctx.globalAlpha = a.paint; ctx.fillStyle = pair.fg;
    ctx.save(); ctx.translate(x, y); ctx.rotate(((a.track || 0) * Math.PI) / 180);
    ctx.beginPath(); ctx.moveTo(0, -6); ctx.lineTo(3.6, 5); ctx.lineTo(-3.6, 5); ctx.closePath(); ctx.fill(); ctx.restore();
    if (labeled.has(a.hex) || view === "radar") ctx.fillText((a.flight || "").slice(0, 8), x + 8, y - 6);
    ctx.globalAlpha = 1;
  });
  if (demoOver) {
    ctx.strokeStyle = "#fff"; ctx.beginPath(); ctx.arc(cx, cy, 18, 0, Math.PI * 2); ctx.stroke();
    ctx.fillStyle = "#fff"; ctx.fillText("SOBRE CASA", cx + 14, cy - 8);
    updateOver();
  }
  requestAnimationFrame(loop);
}
function grabForm() {
  const vBtn = document.querySelector("[data-view].on");
  const tBtn = document.querySelector("[data-theme].on");
  const lBtn = document.querySelector("[data-list].on");
  view = (vBtn && vBtn.dataset.view) || "hybrid";
  theme = (tBtn && tBtn.dataset.theme) || "crt_amber";
  listStyle = (lBtn && lBtn.dataset.list) || "fa";
  const apt = currentApt();
  center = [apt[2], apt[3]];
  followAl = document.getElementById("followAl").value.trim();
  followFlt = document.getElementById("followFlt").value.trim();
  includeGround = document.getElementById("ground").checked;
  radiusKm = Number(document.getElementById("radius").value) || 150;
  maxN = Math.max(4, Math.min(24, Number(document.getElementById("max").value) || 16));
  rotateS = Math.max(3, Math.min(30, Number(document.getElementById("rotate").value) || 7));
  want = {
    air: document.getElementById("fAir").checked,
    ga: document.getElementById("fGa").checked,
    biz: document.getElementById("fBiz").checked,
    heli: document.getElementById("fHeli").checked,
  };
  document.getElementById("radLbl").textContent = "Radio " + radiusKm + " km";
  document.getElementById("maxLbl").textContent = "Max. " + maxN + " vuelos";
  document.getElementById("rotLbl").textContent = "Rotacion " + rotateS + "s";
  document.querySelectorAll(".checks label").forEach((l) => l.classList.toggle("on", l.querySelector("input").checked));
  applyTheme();
  setMeta();
}
document.getElementById("tabMon").onclick = function () {
  document.getElementById("tabMon").classList.add("on");
  document.getElementById("tabCfg").classList.remove("on");
  document.getElementById("paneMon").hidden = false;
  document.getElementById("paneCfg").hidden = true;
};
document.getElementById("tabCfg").onclick = function () {
  document.getElementById("tabCfg").classList.add("on");
  document.getElementById("tabMon").classList.remove("on");
  document.getElementById("paneMon").hidden = true;
  document.getElementById("paneCfg").hidden = false;
};
document.querySelectorAll("[data-view],[data-list],[data-theme]").forEach((b) => {
  b.addEventListener("click", () => {
    const key = b.dataset.view ? "view" : b.dataset.list ? "list" : "theme";
    document.querySelectorAll("[data-" + key + "]").forEach((x) => x.classList.remove("on"));
    b.classList.add("on");
    grabForm(); applyFilters(); renderWall();
    running = true; if (view !== "wall" && !looping) loop();
  });
});
["fAir", "fGa", "fBiz", "fHeli", "ground", "house", "followFlt", "max", "rotate"].forEach((id) => {
  document.getElementById(id).addEventListener("change", () => { grabForm(); applyFilters(); renderWall(); });
});
document.getElementById("radius").addEventListener("input", () => {
  radiusKm = Number(document.getElementById("radius").value) || 150;
  document.getElementById("radLbl").textContent = "Radio " + radiusKm + " km";
  setMeta();
});
document.getElementById("radius").addEventListener("change", () => { grabForm(); loadLive(true); });
document.getElementById("max").addEventListener("input", () => {
  document.getElementById("maxLbl").textContent = "Max. " + document.getElementById("max").value + " vuelos";
});
document.getElementById("rotate").addEventListener("input", () => {
  document.getElementById("rotLbl").textContent = "Rotacion " + document.getElementById("rotate").value + "s";
});
document.getElementById("refreshBtn").onclick = function () {
  grabForm(); loadLive(true); running = true; if (view !== "wall" && !looping) loop();
};
document.getElementById("geo").onclick = function () {
  navigator.geolocation.getCurrentPosition((pos) => {
    home = [pos.coords.latitude, pos.coords.longitude];
    document.getElementById("preset").value = "CASA";
    document.getElementById("house").checked = true;
    grabForm(); loadLive(true);
  });
};
document.getElementById("demoHouse").onclick = function () {
  document.getElementById("house").checked = true;
  demoOver = true;
  if (!home) home = center.slice();
  grabForm(); running = true; if (!looping) loop(); updateOver();
};

async function loadCatalogs() {
  try {
    const [ap, al] = await Promise.all([
      pullJson("airports.json", 8000),
      pullJson("airlines.json", 8000),
    ]);
    if (Array.isArray(ap) && ap.length > 20) {
      const seed = {};
      SEED_AP.forEach((a) => { seed[a[0]] = 1; });
      AIRPORTS = SEED_AP.concat(ap.filter((a) => a && a[0] && !seed[a[0]]));
    }
    if (Array.isArray(al) && al.length > 20) {
      const seen = {};
      SEED_AL.forEach((a) => { seen[a[0]] = 1; });
      ALINES = SEED_AL.concat(al.filter((a) => a && a[0] && !seen[a[0]]));
    }
  } catch (e) { /* seed is enough for EZE/Tokio/Cabo */ }
}

async function boot() {
  grabForm();
  running = true; if (view !== "wall") loop();
  loadLive(true);
  await loadCatalogs();
}
boot();
setInterval(() => { if (view !== "wall") return; if (performance.now() - lastRot > rotateS * 1000) { lastRot = performance.now(); page++; renderWall(); } }, 400);
window.addEventListener("resize", () => renderWall());
