/* Pico Radar v20 — GitHub Pages CRT. Cielo vivo via proxy, pistas finas, logos kiwi. */
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
  phosphor: { fg: "#c6f59a", bg: "#030805" },
  atc_dark: { fg: "#7ec8e3", bg: "#07090c" },
  navy: { fg: "#8ab4ff", bg: "#061018" },
  violet: { fg: "#c4a8ff", bg: "#0c0814" },
  magenta: { fg: "#ff7ad9", bg: "#120814" },
  red: { fg: "#ff6b4a", bg: "#120606" },
  orange: { fg: "#ff9a4a", bg: "#120804" },
  gold: { fg: "#ffd56a", bg: "#0c0a04" },
  ice: { fg: "#d9f6ff", bg: "#081016" },
  cyan: { fg: "#5eead4", bg: "#041210" },
  olive: { fg: "#b7c47a", bg: "#0c0e08" },
  rose: { fg: "#ff8fab", bg: "#14080c" },
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
  ["JAT", "JA", "JetSMART", "Chile"],
  ["JMR", "WJ", "JetSMART Argentina", "Argentina"],
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
  ["TAM", "LA", "LATAM Brasil", "Brazil"],
  ["ACA", "AC", "Air Canada", "Canada"],
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
let center = [-34.822, -58.536], includeGround = true, radiusKm = 150, maxN = 16, rotateS = 7, carouselN = 2;
let beam = -Math.PI / 2, lastSweep = 0, lastRot = performance.now(), page = 0, loading = false;
let loadSeq = 0;
let followAl = "", followFlt = "";
let want = { air: true, ga: false, biz: false, heli: false };
let lastOk = null;
let RUNWAYS = {};
let bumpedFor = "";
let rwyNear = { key: "", list: [] };

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
  f = f.replace(/\btokio\b/g, "tokyo").replace(/\bsingapur\b/g, "singapore").replace(/\bsidney\b/g, "sydney").replace(/\bcabo verde\b/g, "cape verde");
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
function isReg(s) {
  const c = compactId(s);
  return /^(N[0-9]{1,5}[A-Z]{0,3}|LV[A-Z]{3}|LV[A-Z]?\d{3,}|VH[A-Z]{3}|CC[A-Z]{3}|HK\d{4}|PR[A-Z]{3}|PT[A-Z]{3}|G[A-Z]{4}|F[A-Z]{4}|XA[A-Z]{3}|C[A-Z]{4})$/.test(c);
}
function isFlightNumber(s) {
  const c = compactId(s);
  return /^[A-Z]{2,3}\d{1,4}[A-Z]?$/.test(c) && !isReg(c);
}
function displayId(a) {
  const cs = compactId(a.flight);
  if (cs && /^[A-Z]{2,3}\d/.test(cs) && !isReg(cs)) return (a.flight || "").trim() || cs;
  if (cs && isReg(cs)) return cs;
  if (a.r && isReg(a.r)) return compactId(a.r);
  const raw = (a.flight || "").trim();
  if (raw && !/^@+$/.test(raw)) return raw;
  if (a.r) return compactId(a.r);
  return a.hex;
}
function airlineOf(cs) {
  const compact = compactId(cs);
  if (!compact || isReg(compact) || !isFlightNumber(compact)) return ["", compact || "GA", "#334155"];
  const L = letters(cs);
  const hit = ALINES.find((a) => a[0] === L.slice(0, 3));
  if (hit) return [hit[1] || hit[0].slice(0, 2), hit[2], "#334155"];
  if (/^[A-Z]{2}\d/.test(compact)) {
    const byIata = ALINES.find((a) => a[1] === L.slice(0, 2));
    if (byIata) return [byIata[1], byIata[2], "#334155"];
  }
  return ["", L.slice(0, 3) || "ACFT", "#334155"];
}
function kindOf(a) {
  const t = (a.t || "").toUpperCase();
  const cs = (a.flight || "").toUpperCase().trim();
  const compact = compactId(cs);
  const L = letters(cs);
  const flightNo = isFlightNumber(compact);
  const known = flightNo && ALINES.some((x) => x[0] === L.slice(0, 3) || (x[1] && x[1].length === 2 && L.startsWith(x[1]) && /^\d/.test(L.slice(2))));
  if (/^(R44|R66|B06|B407|H125|H135)/.test(t) || /^(RSCU|HEMS)/.test(compact)) return "heli";
  if (t === "TWR" || /^SSM\d/.test(compact) || /^SAETTA/.test(compact) || /^@+$/.test(cs)) return "ga";
  if (isReg(compact) || !flightNo) {
    if (/^(LJ|C25|C56|GLF|GLEX)/.test(t)) return "biz";
    if (/^(A3|A2|B7|B38|E1|E19|CRJ)/.test(t)) return "air";
    return "ga";
  }
  if (known || flightNo) return "air";
  if (cs.startsWith("LV") && !known) return "ga";
  return "ga";
}
function fmtAlt(alt) {
  if (alt === "ground" || alt == null || alt === 0) return "GND";
  const n = Number(alt);
  if (!n) return "-";
  return n >= 1000 ? (n / 1000).toFixed(1) + "k ft" : Math.round(n) + " ft";
}
function fmtGs(gs) { return gs == null ? "-" : Math.round(gs * 1.15078) + " mph"; }
function fmtHdg(track) {
  if (track == null || isNaN(Number(track))) return "—";
  const t = ((Math.round(Number(track)) % 360) + 360) % 360;
  const dirs = ["N", "NE", "E", "SE", "S", "SO", "O", "NO"];
  const dir = dirs[Math.round(t / 45) % 8];
  return String(t).padStart(3, "0") + "° " + dir;
}
function isGnd(a) {
  if (a.alt === "ground" || a.alt === 0) return true;
  const n = Number(a.alt);
  if (isFinite(n) && n < 0) return true;
  return a.gs != null && a.gs < 30 && (Number(a.alt) || 0) < 800;
}
function distNmOf(a) {
  if (a.dst != null && isFinite(Number(a.dst))) return Number(a.dst);
  if (a.lat == null) return 99;
  return kmLL([a.lat, a.lon], center) / 1.852;
}
function looksAir(a) {
  const cs = compactId(a.flight);
  const t = (a.t || "").toUpperCase();
  if (t === "TWR" || /^(RSCU|HEMS|SSM|SAETTA)/.test(cs) || isReg(cs) || !isFlightNumber(cs)) return false;
  return kindOf(a) === "air";
}
function phaseOf(a) {
  if (isGnd(a)) return "EN TIERRA";
  const alt = Number(a.alt) || 0;
  const gs = Number(a.gs) || 0;
  const destLL = aptCoord(a.dest);
  if (destLL && gs > 40) {
    const rem = kmLL([a.lat, a.lon], destLL);
    if (rem < 90 && alt < 16000) return "APROXIMANDO";
  } else if (alt > 0 && alt < 10000 && gs > 40 && gs < 280) return "APROXIMANDO";
  return "EN VUELO";
}
function pillClass(st) {
  if (st === "EN TIERRA") return "pill gnd";
  if (st === "APROXIMANDO") return "pill aprox";
  return "pill";
}
function matchAirline(flight, al) {
  const L = letters(flight);
  if (!al || !L || isReg(flight) || !isFlightNumber(flight)) return false;
  if (al[0] && L.startsWith(al[0])) return true;
  if (al[1] && al[1].length === 2 && L.startsWith(al[1]) && /^\d/.test(L.slice(2))) return true;
  return false;
}
function findAirline(q) {
  const s = (q || "").trim();
  if (s.length < 2) return null;
  const up = s.toUpperCase();
  const exact = ALINES.find((a) => a[0] === up || a[1] === up);
  if (exact) return exact;
  const toks = up.replace(/[^A-Z0-9]+/g, " ").trim().split(/\s+/);
  for (let i = toks.length - 1; i >= 0; i--) {
    const tok = toks[i];
    const hit = ALINES.find((a) => a[0] === tok || (a[1] && a[1] === tok));
    if (hit) return hit;
  }
  const f = rewrite(s);
  if (!f) return null;
  return ALINES.find((a) => fold(a.join(" ")).includes(f) || fold(a[2]).startsWith(f)) || null;
}
function hhmm(ts) {
  if (!ts) return "--:--";
  const d = new Date(ts);
  return String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
}
function kmLL(a, b) {
  return Math.hypot((a[0] - b[0]) * 111, (a[1] - b[1]) * 111 * Math.cos((a[0] * Math.PI) / 180));
}
function nearbyRwy() {
  const key = center[0] + "," + center[1] + "," + radiusKm + "," + AIRPORTS.length;
  if (rwyNear.key === key) return rwyNear.list;
  if (radiusKm >= 120) {
    rwyNear = { key: key, list: [] };
    return rwyNear.list;
  }
  const maxD = Math.min(radiusKm * 0.95, 115);
  const out = [];
  for (let i = 0; i < AIRPORTS.length; i++) {
    const a = AIRPORTS[i];
    const rows = RUNWAYS[a[0]];
    if (!rows || !rows.length) continue;
    const d = kmLL(center, [a[2], a[3]]);
    if (d > maxD) continue;
    out.push({ iata: a[0], lat: a[2], lon: a[3], rows: rows, d: d });
  }
  out.sort((a, b) => a.d - b.d);
  rwyNear = { key: key, list: out.slice(0, 10) };
  return rwyNear.list;
}
function aptCoord(code) {
  const a = AIRPORTS.find((x) => x[0] === code);
  return a ? [a[2], a[3]] : null;
}
function resolveRoute(a) {
  let origin = a.origin || "";
  let dest = a.dest || "";
  if (origin && dest) return { origin, dest };
  const apt = currentApt();
  const alt = Number(a.alt) || 0;
  const gs = Number(a.gs) || 0;
  const dSel = kmLL([a.lat, a.lon], [apt[2], apt[3]]);
  function hdgDelta(x, y) { return Math.abs(((x - y + 540) % 360) - 180); }
  function bearing(from, to) {
    const dlon = (to[1] - from[1]) * Math.PI / 180;
    const lat1 = from[0] * Math.PI / 180, lat2 = to[0] * Math.PI / 180;
    const y = Math.sin(dlon) * Math.cos(lat2);
    const x = Math.cos(lat1) * Math.sin(lat2) - Math.sin(lat1) * Math.cos(lat2) * Math.cos(dlon);
    return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
  }
  const toward = a.track != null && hdgDelta(a.track, bearing([a.lat, a.lon], [apt[2], apt[3]])) < 28;
  const away = a.track != null && hdgDelta(a.track, bearing([apt[2], apt[3]], [a.lat, a.lon])) < 32;
  if (isGnd(a) && dSel < 8) origin = origin || apt[0];
  else if (gs > 70 && alt > 200 && alt < 14000 && dSel < 75 && toward) dest = dest || apt[0];
  else if (gs > 90 && alt > 600 && dSel < 36 && away) origin = origin || apt[0];
  return { origin: origin || "—", dest: dest || "—" };
}
function setLoad(show, msg) {
  if (msg) statusEl.textContent = msg;
  if (show) loadEl.textContent = "CARGANDO " + (currentApt()[0] || "");
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
  if ((up === "CASA" || up.startsWith("CASA")) && home) return ["CASA", "Casa", home[0], home[1], "AR", "Casa"];
  const m = up.match(/^([A-Z]{3})(?![A-Z0-9])/);
  if (m) {
    const hit = AIRPORTS.find((a) => a[0] === m[1]);
    if (hit) return hit;
  }
  const s = rewrite(q);
  const scored = AIRPORTS.map((a) => {
    const h = hay(a);
    const city = fold(a[1]);
    let n = 0;
    if (a[0].toLowerCase() === s) n = 100;
    else if (a[0] === "SYD" && s === "sydney") n = 100;
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
  input.addEventListener("keydown", (e) => {
    if (e.key !== "Enter") return;
    const first = listEl.querySelector("button[data-v]");
    if (!first) return;
    e.preventDefault();
    onPick(first.dataset.v, first.textContent);
    listEl.style.display = "none";
  });
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
    bumpedFor = "";
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
    grabForm();
    applyFilters();
    renderWall();
  },
);

function enrich(a) {
  const now = Date.now();
  const key = compactId(a.flight);
  const cached = ROUTECACHE[key];
  if (cached) { a.origin = cached[0]; a.dest = cached[1]; }
  const gnd = isGnd(a);
  if (gnd) {
    a.status = "EN TIERRA"; a.depKind = "EST"; a.arrKind = "EST";
    a.depAt = null; a.arrAt = null; a.pct = 6;
    return a;
  }
  a.status = phaseOf(a); a.depKind = "EST"; a.arrKind = "EST";
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
  const alHit = findAirline(alQ);
  AC = RAW.filter((a) => {
    if (distNmOf(a) > radiusNm * 1.08) return false;
    const k = kindOf(a);
    if (!want[k]) return false;
    if (isGnd(a) && !includeGround) return false;
    if (alHit && !matchAirline(a.flight, alHit)) return false;
    if (followFlt && !compactId(a.flight).includes(compactId(followFlt)) && compactId(a.r || "").indexOf(compactId(followFlt)) < 0) return false;
    return a.lat != null;
  }).sort((a, b) => (looksAir(b) - looksAir(a)) || (distNmOf(a) - distNmOf(b))).slice(0, maxN);
  AC.forEach(enrich);
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
  const urls = [];
  if (SKY_API) urls.push(SKY_API + "/sky?lat=" + lat + "&lon=" + lon + "&dist=" + nm);
  urls.push(
    "https://opendata.adsb.fi/api/v2/lat/" + lat + "/lon/" + lon + "/dist/" + nm,
    "https://api.adsb.lol/v2/lat/" + lat + "/lon/" + lon + "/dist/" + nm,
  );
  return urls;
}
function normList(data) {
  const list = data && (data.ac || data.aircraft);
  return Array.isArray(list) ? list : [];
}
async function fetchSnapshot(lat, lon) {
  let cache = null;
  try { cache = await pullJson("sky-cache.json", 4000); } catch (e) { cache = null; }
  if (!cache) {
    try {
      const live = await pullJson("live.json", 3000);
      if (live && Array.isArray(live.ac)) {
        cache = { EZE: { lat: live.lat || -34.822, lon: live.lon || -58.536, ac: live.ac, ts: live.ts } };
      }
    } catch (e) { cache = null; }
  }
  if (!cache || typeof cache !== "object") throw new Error("sin cielo");
  const apt = currentApt()[0];
  let best = cache[apt] || null;
  let bestD = best ? 0 : 1e9;
  if (!best) {
    Object.keys(cache).forEach((k) => {
      const h = cache[k];
      if (!h || h.lat == null || !Array.isArray(h.ac)) return;
      const d = kmLL([lat, lon], [h.lat, h.lon]);
      if (d < bestD) { bestD = d; best = h; }
    });
  }
  if (!best || !Array.isArray(best.ac)) throw new Error("sin cielo");
  if (bestD > 220) throw new Error("sin cobertura de snapshot");
  return { list: best.ac, src: "snapshot" };
}
async function fetchSky(lat, lon, nm) {
  const byHex = {};
  const sources = [];
  async function ingest(url, src, ms) {
    try {
      const data = await pullJson(url, ms);
      const list = normList(data);
      list.forEach((a) => {
        if (a && a.hex && !byHex[a.hex]) byHex[a.hex] = a;
      });
      if (list.length) sources.push(data.source || src);
      return list.length;
    } catch (e) {
      return -1;
    }
  }
  function commercialN() {
    return Object.keys(byHex).filter((h) => {
      const a = norm(byHex[h]);
      return looksAir(a) && !isGnd(a);
    }).length;
  }
  const urls = skyUrls(lat, lon, nm);
  for (let i = 0; i < urls.length; i++) {
    const u = urls[i];
    const src = u.indexOf("adsb.fi") >= 0 ? "adsb.fi" : u.indexOf("adsb.lol") >= 0 ? "adsb.lol" : u.indexOf("vercel.app") >= 0 ? "proxy" : "vivo";
    await ingest(u, src, src === "vivo" ? 2500 : 9000);
    if (Object.keys(byHex).length >= 8 && commercialN() >= 2) break;
  }
  const wide = Math.min(250, Math.max(nm, 200));
  if ((Object.keys(byHex).length < 8 || commercialN() < 2) && wide > nm + 10) {
    await ingest("https://api.adsb.lol/v2/lat/" + lat + "/lon/" + lon + "/dist/" + wide, "adsb.lol", 8000);
  }
  const list = Object.keys(byHex).map((h) => byHex[h]);
  if (list.length) return { list: list, src: sources.join("+") || "vivo" };
  try {
    return await fetchSnapshot(lat, lon);
  } catch (e) {
    throw e;
  }
}
async function fillRoutes(list) {
  const pending = list.filter((a) => {
    const cs = compactId(a.flight);
    if (isReg(cs) || ROUTECACHE[cs]) return false;
    return a.flight && cs.length >= 4 && /[A-Z]{2,3}\d/.test(cs);
  }).slice(0, 28);
  function keysFor(flight) {
    const cs = compactId(flight);
    const out = [cs];
    const m = cs.match(/^([A-Z]{2,3})(\d+)$/);
    if (!m) return out;
    const prefix = m[1], num = m[2];
    const pad = num.padStart(4, "0");
    if (pad !== num) out.push(prefix + pad);
    const pair = airlineOf(flight);
    const icao = letters(flight).slice(0, 3);
    const hit = ALINES.find((a) => a[0] === icao);
    if (hit && hit[0] !== prefix) out.push(hit[0] + num);
    if (hit && hit[1] && hit[1] !== prefix) out.push(hit[1] + num);
    void pair;
    return out.filter((v, i, arr) => arr.indexOf(v) === i);
  }
  await Promise.all(pending.map(async (a) => {
    const cs = compactId(a.flight);
    const keys = keysFor(a.flight);
    for (let i = 0; i < keys.length; i++) {
      const key = keys[i];
      if (ROUTECACHE[key]) {
        ROUTECACHE[cs] = ROUTECACHE[key];
        a.origin = ROUTECACHE[key][0];
        a.dest = ROUTECACHE[key][1];
        return;
      }
      try {
        const d = await pullJson("https://vrs-standing-data.adsb.lol/routes/" + key.slice(0, 2) + "/" + key + ".json", 3500);
        const iata = (d._airport_codes_iata || "").split("-").map((x) => x.trim().toUpperCase()).filter((x) => x && x.length >= 3 && x.length <= 4);
        if (iata.length >= 2) {
          const pair = [iata[0], iata[iata.length - 1]];
          ROUTECACHE[cs] = pair;
          ROUTECACHE[key] = pair;
          a.origin = pair[0];
          a.dest = pair[1];
          return;
        }
      } catch (e) { /* try next key */ }
    }
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
  const id = ++loadSeq;
  loading = true;
  const apt = currentApt();
  center = [apt[2], apt[3]];
  if (showOverlay) setLoad(true, "Cargando " + apt[0]);
  const nm = Math.min(250, Math.max(180, Math.round(radiusKm / 1.852) + 8));
  try {
    const got = await fetchSky(center[0], center[1], nm);
    srcLabel = got.src;
    lastOk = { list: got.list, src: got.src, at: Date.now() };
    RAW = got.list.map(norm).filter((a) => a.lat != null && a.hex);
    applyFilters();
    const commercial = RAW.filter((a) => looksAir(a) && !isGnd(a));
    const inside = commercial.filter((a) => distNmOf(a) * 1.852 <= radiusKm * 1.05);
    if (inside.length < 1 && commercial.length && bumpedFor !== apt[0] + ":manual") {
      const farthest = Math.max.apply(null, commercial.map((a) => (a.dst || 0) * 1.852));
      const bumped = Math.min(400, Math.ceil(farthest / 10) * 10 + 20);
      if (bumped > radiusKm + 5) {
        bumpedFor = apt[0];
        radiusKm = bumped;
        document.getElementById("radius").value = String(radiusKm);
        document.getElementById("radLbl").textContent = "Radio " + radiusKm + " km";
        applyFilters();
      }
    }
    renderWall();
    fillRoutes(AC).then(() => { applyFilters(); renderWall(); });
    const now = performance.now();
    AC.forEach((a) => { POS[a.hex] = { lat: a.lat, lon: a.lon, t: now, gs: a.gs, track: a.track }; });
    lastSweep = now;
    lastRot = performance.now();
    const alQ = (document.getElementById("followAl").value || "").trim();
    const alHit = findAirline(alQ);
    if (!RAW.length) statusEl.textContent = "0 aeronaves en " + apt[0] + " (radio " + radiusKm + " km). Poca cobertura ADS-B en esta zona ahora.";
    else if (alQ && !AC.length) statusEl.textContent = RAW.length + " aviones en zona, 0 de " + (alHit ? alHit[2] : alQ);
    else {
      const gndN = AC.filter(isGnd).length;
      const airN = AC.filter((a) => looksAir(a) && !isGnd(a)).length;
      statusEl.textContent = RAW.length + " aeronaves · " + apt[0] + " · " + srcLabel + (gndN ? " · " + gndN + " en tierra" : "") + (airN === 0 && RAW.length ? " · poca cobertura comercial" : "");
    }
    foot.textContent = (srcLabel === "snapshot" ? "ADS-B (copia local) · " : "ADS-B en vivo · ") + srcLabel;
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
    if (id === loadSeq) {
      loading = false;
      loadEl.classList.remove("on");
    }
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
  const id = displayId(a);
  const pair = airlineOf(a.flight);
  const iata = isReg(id) ? "" : pair[0];
  const name = isReg(id) ? id : pair[1];
  const fb = '<div class="fb">' + (iata || name || "GA").slice(0, 2) + "</div>";
  if (!iata) return fb;
  return '<img alt="' + iata + '" src="logos/' + iata + '.png" onerror="this.outerHTML=this.dataset.fb" data-fb=\'' + fb + "'>";
}
function cardHTML(a) {
  const pair = airlineOf(a.flight);
  const iata = pair[0];
  const name = pair[1];
  const pct = a.pct != null ? a.pct : 18;
  const route = resolveRoute(a);
  const st = a.status || phaseOf(a);
  const gnd = st === "EN TIERRA";
  const id = displayId(a);
  const sub = isReg(id)
    ? ("Matrícula · " + (a.t || a.r || "—"))
    : (iata ? (a.t || a.r || "—") : (name + " · " + (a.t || a.r || "-")));
  const foot = gnd
    ? "En tierra · preparandose para el despegue"
    : (a.arrAt ? "Llega en " + Math.max(0, Math.round((a.arrAt - Date.now()) / 60000)) + " min" : "En ruta");
  const pill = '<span class="' + pillClass(st) + '">' + st + "</span>";
  return (
    '<div class="fa-card">' +
      '<div class="fa-row"><div class="logo">' + logoHTML(a) + "</div><div>" +
        '<div class="fa-top"><span class="fa-id">' + id + "</span>" + pill + "</div>" +
        '<div class="fa-air">' + sub + "</div>" +
      "</div></div>" +
      '<div class="fa-route"><span>' + route.origin + '</span><span class="bar"><i style="width:' + pct + '%"></i></span><span>' + route.dest + "</span></div>" +
      '<div class="fa-times">' +
        '<div><span class="lbl">Despegue ' + a.depKind + "</span><b>" + hhmm(a.depAt) + "</b></div>" +
        '<div><span class="lbl">Aterrizaje ' + a.arrKind + "</span><b>" + hhmm(a.arrAt) + "</b></div>" +
      "</div>" +
      '<div class="metrics">' +
        '<div><span class="lbl">Altitud</span><b>' + (gnd ? "GND" : fmtAlt(a.alt)) + "</b></div>" +
        '<div><span class="lbl">Velocidad</span><b>' + (gnd ? "—" : fmtGs(a.gs)) + "</b></div>" +
        '<div><span class="lbl">Rumbo</span><b>' + (gnd || a.track == null ? "—" : '<span class="hdg"><span class="arr" style="transform:rotate(' + Number(a.track) + 'deg)"></span> ' + fmtHdg(a.track) + "</span>") + "</b></div>" +
      "</div>" +
      '<div class="fa-foot"><span>' + foot + "</span>" + pill + "</div>" +
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
  if (listStyle === "carousel") {
    const n = Math.max(1, Math.min(carouselN, AC.length));
    const start = (page * n) % AC.length;
    const out = [];
    for (let i = 0; i < n; i++) out.push(AC[(start + i) % AC.length]);
    return out;
  }
  const h = wall.clientHeight || 300;
  const n = listStyle === "fids"
    ? Math.max(1, Math.floor((h - 22) / (view === "wall" ? 48 : 36)))
    : Math.max(1, Math.floor((h - 16) / (view === "wall" ? 250 : 188)));
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
    wall.innerHTML = '<table class="fids"><thead><tr><th></th><th>VUELO</th><th>RUTA</th><th>DEP</th><th>ARR</th><th>RUMBO</th><th>ESTADO</th></tr></thead><tbody>' +
      list.map((a) => {
        const route = resolveRoute(a);
        return '<tr><td><div class="logo" style="width:28px;height:28px;font-size:10px">' + logoHTML(a) + "</div></td><td>" + displayId(a) + "</td><td>" + route.origin + " → " + route.dest + "</td><td>" + hhmm(a.depAt) + "</td><td>" + hhmm(a.arrAt) + "</td><td>" + (isGnd(a) ? "—" : fmtHdg(a.track)) + '</td><td><span class="' + pillClass(a.status || phaseOf(a)) + '">' + (a.status || phaseOf(a)) + "</span></td></tr>";
      }).join("") +
      "</tbody></table>";
  } else if (listStyle === "carousel") {
    const list = visibleList();
    wall.innerHTML = '<div class="carousel-wrap' + (list.length === 1 ? " hero" : list.length <= 3 ? " lg" : "") + '" style="grid-template-rows:repeat(' + list.length + ',minmax(0,1fr))">' +
      list.map(cardHTML).join("") + "</div>";
  } else {
    const hero = view === "wall" && list.length === 1;
    const lg = view === "wall" && list.length <= 3;
    if (view === "wall") {
      wall.innerHTML = '<div class="carousel-wrap' + (hero ? " hero" : lg ? " lg" : "") + '" style="grid-template-rows:repeat(' + list.length + ',minmax(0,1fr))">' +
        list.map(cardHTML).join("") + "</div>";
    } else {
      wall.innerHTML = list.map(cardHTML).join("");
    }
    wall.classList.toggle("hero", hero);
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
  ctx.fillStyle = pair.fg; ctx.font = "12px ui-monospace, monospace";
  ctx.fillText(currentApt()[0], 10, 18);
  ctx.globalAlpha = 0.7; ctx.font = "11px sans-serif";
  ctx.fillText(currentApt()[1] || "", 10, 34);
  ctx.globalAlpha = 1;
  const span = Math.max(0.25, radiusKm / 111);
  const toXY = (lat, lon) => ({ x: cx + ((lon - center[1]) / span) * R, y: cy - ((lat - center[0]) / span) * R });
  if (radiusKm < 120) {
    const sets = nearbyRwy();
    sets.forEach((set) => {
      const rows = set.rows;
      rows.forEach((rw) => {
        const a = toXY(rw[4], rw[5]), b = toXY(rw[6], rw[7]);
        ctx.strokeStyle = pair.fg; ctx.globalAlpha = 0.92;
        const len = Math.hypot(b.x - a.x, b.y - a.y);
        ctx.lineCap = "butt"; ctx.lineJoin = "miter";
        ctx.lineWidth = Math.max(1.15, Math.min(2.4, len * 0.14));
        ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
        if (radiusKm < 90 && len > 14) {
          ctx.globalAlpha = 0.9; ctx.font = "10px ui-monospace, monospace"; ctx.fillStyle = pair.fg;
          ctx.fillText(String(rw[0]), a.x + 6, a.y - 4);
          ctx.fillText(String(rw[1]), b.x + 6, b.y - 4);
        }
      });
      const mid = toXY(set.lat, set.lon);
      ctx.globalAlpha = 0.8; ctx.font = "11px ui-monospace, monospace"; ctx.fillStyle = pair.fg;
      ctx.fillText(set.iata, mid.x + 8, mid.y + 14);
      const scored = {};
      AC.forEach((ac) => {
        if (isGnd(ac) || ac.track == null) return;
        const alt = Number(ac.alt) || 0, gs = Number(ac.gs) || 0;
        if (alt > 9000 || gs < 80) return;
        rows.forEach((rw) => {
          [[rw[0], rw[2], rw[4], rw[5]], [rw[1], rw[3], rw[6], rw[7]]].forEach((end) => {
            const ident = String(end[0]), hdg = Number(end[1]), lat = end[2], lon = end[3];
            if (!ident || !isFinite(hdg)) return;
            let d = ((ac.track - hdg + 540) % 360) - 180;
            if (Math.abs(d) > 14) return;
            const km = kmLL([ac.lat, ac.lon], [lat, lon]);
            if (km < 1.2 || km > 28) return;
            scored[ident] = scored[ident] || { ident, hdg, lat, lon, n: 0 };
            scored[ident].n += 1;
          });
        });
      });
      const ranked = Object.keys(scored).map((k) => scored[k]).sort((a, b) => b.n - a.n);
      const top = ranked.length ? ranked[0].n : 0;
      ranked.filter((x) => x.n >= top).forEach((hit) => {
        const rec = (hit.hdg + 180) % 360;
        const rad = rec * Math.PI / 180;
        const backKm = Math.min(22, radiusKm * 0.5);
        const back = [hit.lat + Math.cos(rad) * backKm / 111, hit.lon + Math.sin(rad) * backKm / (111 * Math.cos(hit.lat * Math.PI / 180))];
        const s = toXY(back[0], back[1]), t = toXY(hit.lat, hit.lon);
        if (radiusKm < 80) {
          ctx.setLineDash([8, 6]); ctx.strokeStyle = pair.fg; ctx.globalAlpha = 0.8; ctx.lineWidth = 1.6;
          ctx.lineCap = "butt";
          ctx.beginPath(); ctx.moveTo(s.x, s.y); ctx.lineTo(t.x, t.y); ctx.stroke();
          ctx.setLineDash([]); ctx.fillStyle = pair.fg; ctx.globalAlpha = 0.95; ctx.font = "11px ui-monospace, monospace";
          ctx.fillText("APX " + hit.ident, s.x + 4, s.y - 4);
        }
      });
    });
    ctx.globalAlpha = 1;
  }
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
  rotateS = Math.max(4, Math.min(30, Number(document.getElementById("rotate").value) || 8));
  carouselN = Math.max(1, Math.min(4, Number(document.getElementById("carN").value) || 1));
  want = {
    air: document.getElementById("fAir").checked,
    ga: document.getElementById("fGa").checked,
    biz: document.getElementById("fBiz").checked,
    heli: document.getElementById("fHeli").checked,
  };
  document.getElementById("radLbl").textContent = "Radio " + radiusKm + " km";
  document.getElementById("maxLbl").textContent = "Max. " + maxN + " vuelos";
  document.getElementById("rotLbl").textContent = "Rotacion " + rotateS + "s";
  document.getElementById("carLbl").textContent = "Carrusel: " + carouselN + (carouselN === 1 ? " vuelo" : " vuelos");
  document.getElementById("carField").hidden = listStyle !== "carousel";
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
["fAir", "fGa", "fBiz", "fHeli", "ground", "house", "followFlt", "max", "rotate", "carN"].forEach((id) => {
  document.getElementById(id).addEventListener("change", () => { grabForm(); applyFilters(); renderWall(); });
});
document.getElementById("followAl").addEventListener("input", () => { grabForm(); applyFilters(); renderWall(); });
document.getElementById("carN").addEventListener("input", () => {
  carouselN = Math.max(1, Math.min(4, Number(document.getElementById("carN").value) || 1));
  document.getElementById("carLbl").textContent = "Carrusel: " + carouselN + (carouselN === 1 ? " vuelo" : " vuelos");
  renderWall();
});
document.getElementById("radius").addEventListener("input", () => {
  bumpedFor = currentApt()[0] + ":manual";
  radiusKm = Number(document.getElementById("radius").value) || 150;
  document.getElementById("radLbl").textContent = "Radio " + radiusKm + " km";
  setMeta();
});
document.getElementById("radius").addEventListener("change", () => { grabForm(); loadLive(true); });
document.getElementById("max").addEventListener("input", () => {
  document.getElementById("maxLbl").textContent = "Max. " + document.getElementById("max").value + " vuelos";
});
document.getElementById("rotate").addEventListener("input", () => {
  rotateS = Math.max(4, Math.min(30, Number(document.getElementById("rotate").value) || 8));
  document.getElementById("rotLbl").textContent = "Rotacion " + rotateS + "s";
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
    try {
      const rw = await pullJson("runways.json", 8000);
      if (rw && typeof rw === "object") RUNWAYS = rw;
    } catch (e) { /* optional */ }
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
