/* Pico Radar v30 — typeahead de vuelo, mapa continental, flyover sin recortes. */
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
  ["IAH", "Houston", 29.984, -95.341, "United States", "George Bush"],
  ["MEX", "Ciudad de Mexico", 19.436, -99.072, "Mexico", "Benito Juarez"],
  ["BOG", "Bogota", 4.701, -74.147, "Colombia", "El Dorado"],
  ["LIM", "Lima", -12.022, -77.114, "Peru", "Jorge Chavez"],
  ["PTY", "Panama", 9.071, -79.383, "Panama", "Tocumen"],
  ["COR", "Cordoba", -31.31, -64.208, "Argentina", "Cordoba"],
  ["MDZ", "Mendoza", -32.832, -68.793, "Argentina", "Mendoza"],
  ["BRC", "Bariloche", -41.151, -71.158, "Argentina", "Bariloche"],
  ["IGR", "Iguazu", -25.737, -54.473, "Argentina", "Iguazu"],
  ["USH", "Ushuaia", -54.843, -68.296, "Argentina", "Ushuaia"],
  ["MDQ", "Mar del Plata", -37.934, -57.573, "Argentina", "Mar del Plata"],
  ["SLA", "Salta", -24.856, -65.486, "Argentina", "Salta"],
  ["GIG", "Rio de Janeiro", -22.81, -43.251, "Brazil", "Galeao"],
  ["MVD", "Montevideo", -34.838, -56.031, "Uruguay", "Carrasco"],
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
const TRAIL = {};
let LAND = [];
let home = null, demoOver = false, srcLabel = "";
let RAW = [], AC = [], SKY = [], running = false, looping = false;
let view = "hybrid", theme = "crt_amber", listStyle = "fa";
let spec = { inches: 15, w: 640, h: 480 };
let runwaysOn = true;
let installLocked = false;
const LS_INSTALL = "pico-radar-install-v1";
const PRESETS = [
  { id: "2", spec: { inches: 2, w: 320, h: 240 } },
  { id: "3", spec: { inches: 3, w: 320, h: 240 } },
  { id: "7", spec: { inches: 7, w: 800, h: 480 } },
  { id: "10", spec: { inches: 10, w: 800, h: 600 } },
  { id: "15", spec: { inches: 15, w: 640, h: 480 } },
  { id: "21", spec: { inches: 21, w: 1024, h: 768 } },
];
const DEFAULT_SPEC = { inches: 15, w: 640, h: 480 };
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
function clampSpec(s) {
  const inches = Math.min(32, Math.max(1.5, Math.round((Number(s && s.inches) || DEFAULT_SPEC.inches) * 10) / 10));
  const w = Math.round(Math.min(1920, Math.max(160, Number(s && s.w) || DEFAULT_SPEC.w)));
  const h = Math.round(Math.min(1200, Math.max(120, Number(s && s.h) || DEFAULT_SPEC.h)));
  return { inches, w, h };
}
function guessVga(inches) {
  if (inches < 4.5) return { w: 320, h: 240 };
  if (inches < 8) return { w: 800, h: 480 };
  if (inches < 12) return { w: 800, h: 600 };
  if (inches < 18) return { w: 640, h: 480 };
  return { w: 1024, h: 768 };
}
function guessInches(w, h) {
  const min = Math.min(w, h);
  if (min <= 240) return 3;
  if (min <= 320) return 5;
  if (min <= 480) return 7;
  if (min <= 600) return 10;
  if (min <= 768) return 15;
  return 21;
}
function layoutOf(s) {
  s = clampSpec(s);
  const minPx = Math.min(s.w, s.h);
  let band = "large";
  if (s.inches < 5 || minPx < 360) band = "tiny";
  else if (s.inches < 12) band = "mid";
  const hybridOk = band !== "tiny";
  const cards = band === "tiny" ? 1 : Math.max(1, Math.min(8, Math.floor(s.h / (band === "large" ? 280 : 200))));
  const carouselN0 = band === "tiny" ? 1 : band === "mid" ? 2 : Math.min(4, Math.max(2, Math.floor(s.h / 240)));
  const t = (s.inches - 2) / 19;
  const typeScale = Math.round((0.72 + Math.max(0, Math.min(1, t)) * 0.4) * 100) / 100;
  const frame = band !== "large";
  const inch = Number.isInteger(s.inches) ? String(s.inches) : s.inches.toFixed(1);
  const label = inch + '" ' + s.w + "×" + s.h;
  const mode = hybridOk ? "híbrido" : "radar o tarjetas";
  const cardTxt = cards === 1 ? "1 tarjeta" : cards + " tarjetas";
  return { spec: s, band, hybridOk, cards, carouselN: carouselN0, typeScale, frame, label, summary: label + " · " + mode + " · " + cardTxt };
}
function matchPreset(s) {
  s = clampSpec(s);
  const hit = PRESETS.find((p) => p.spec.inches === s.inches && p.spec.w === s.w && p.spec.h === s.h);
  return hit ? hit.id : "custom";
}
function parseVga(raw) {
  const m = String(raw || "").match(/(\d{2,4})\s*[x×]\s*(\d{2,4})/i);
  return m ? { w: Number(m[1]), h: Number(m[2]) } : null;
}
function parseInches(raw) {
  const s = String(raw || "").toLowerCase().replace(/["'\s]/g, "").replace(/pulgadas|inch|in$/g, "");
  const n = Number(s);
  if (!Number.isFinite(n) || n < 1.5 || n > 32) return null;
  return Math.round(n * 10) / 10;
}
function parseLegacyScreen(raw) {
  const s = String(raw || "").toLowerCase().replace(/["'\s]/g, "").replace(/pulgadas|inch|in$/g, "");
  if (s === "2") return "2";
  if (s === "3" || s === "tiny" || s === "small" || s === "qvga") return "3";
  if (s === "7" || s === "mid" || s === "wvga") return "7";
  if (s === "10") return "10";
  if (s === "15" || s === "19" || s === "big" || s === "vga" || s === "auto") return "15";
  if (s === "21") return "21";
  return null;
}
function specFromQuery(q) {
  const inches = parseInches(q.get("in") || q.get("inch") || q.get("pulgadas"));
  const vga = parseVga(q.get("vga") || q.get("res") || q.get("px"));
  const legacyId = parseLegacyScreen(q.get("disp") || q.get("screen"));
  const legacy = legacyId ? PRESETS.find((p) => p.id === legacyId) : null;
  if (inches == null && !vga && !legacy) return null;
  const base = (legacy && legacy.spec) || DEFAULT_SPEC;
  const inch = inches != null ? inches : (vga ? guessInches(vga.w, vga.h) : base.inches);
  const px = vga || (inches != null ? guessVga(inches) : { w: base.w, h: base.h });
  return clampSpec({ inches: inch, w: px.w, h: px.h });
}
function fitFrame(w, h, maxW, maxH) {
  const mw = Math.max(80, maxW);
  const mh = Math.max(60, maxH);
  const scale = Math.min(1, mw / w, mh / h);
  return { width: Math.max(80, Math.round(w * scale)), height: Math.max(60, Math.round(h * scale)), scale };
}
function fillSpecInputs(s) {
  s = clampSpec(s);
  spec = s;
  const a = document.getElementById("inIn");
  const b = document.getElementById("vgaW");
  const c = document.getElementById("vgaH");
  if (a) a.value = String(s.inches);
  if (b) b.value = String(s.w);
  if (c) c.value = String(s.h);
  document.querySelectorAll("[data-preset]").forEach((btn) => btn.classList.toggle("on", btn.dataset.preset === matchPreset(s)));
}
function shownView() {
  const L = layoutOf(spec);
  if (!L.hybridOk) return view === "radar" ? "radar" : "wall";
  return view;
}
function viewLabel() {
  const v = shownView();
  return v === "hybrid" ? "Hibrido" : v === "radar" ? "Radar" : "Pared";
}
const screenMeta = document.getElementById("screenMeta");
function setMeta() {
  const L = layoutOf(spec);
  const hit = followAcOf();
  const ezeN = SKY.filter(relatedToAirport).length;
  const line = hit
    ? ("Siguiendo " + displayId(hit) + " · " + viewLabel() + " · " + L.label)
    : (SKY.length + " en radar · " + ezeN + " de " + currentApt()[0] + " · " + radiusKm + " km · " + viewLabel() + " · " + L.label);
  if (screenMeta) screenMeta.textContent = line;
}
function cfgQuery() {
  const p = new URLSearchParams();
  p.set("apt", currentApt()[0]);
  p.set("view", shownView());
  p.set("list", listStyle);
  p.set("theme", theme);
  p.set("r", String(radiusKm));
  if (!runwaysOn) p.set("rwy", "0");
  return p.toString();
}
function installQuery() {
  const L = layoutOf(spec);
  const p = new URLSearchParams(cfgQuery());
  p.set("in", String(L.spec.inches));
  p.set("vga", L.spec.w + "x" + L.spec.h);
  p.set("tab", "install");
  return p.toString();
}
function saveInstall() {
  try { localStorage.setItem(LS_INSTALL, JSON.stringify({ spec: spec, locked: installLocked })); } catch (e) {}
}
function loadInstall() {
  try {
    const raw = localStorage.getItem(LS_INSTALL);
    if (!raw) return;
    const s = JSON.parse(raw);
    if (s && s.spec) spec = clampSpec(s.spec);
    installLocked = !!(s && s.locked);
  } catch (e) {}
}
function applyInstallLock() {
  const fields = document.getElementById("installFields");
  const nums = document.getElementById("installNums");
  if (fields) fields.classList.toggle("install-lock", installLocked);
  if (nums) nums.classList.toggle("install-lock", installLocked);
  const btn = document.getElementById("installLock");
  const state = document.getElementById("installState");
  const L = layoutOf(spec);
  if (btn) btn.textContent = installLocked ? "Cambiar instalación" : "Guardar en este Pico";
  if (state) state.textContent = installLocked
    ? ("Grabado en este Pico: " + L.label + ". El cliente elige vista y formato, no el tamaño.")
    : "Todavía no está grabado. Probá el tamaño en el CRT y cuando cierre, guardalo.";
}
function showPane(which) {
  const tabs = { tabMon: "paneMon", tabCfg: "paneCfg", tabInst: "paneInst" };
  Object.keys(tabs).forEach((id) => {
    const t = document.getElementById(id);
    const pane = document.getElementById(tabs[id]);
    if (t) t.classList.toggle("on", id === which);
    if (pane) pane.hidden = id !== which;
  });
}
function updateQr() {
  const pico = "http://192.168.4.1/?" + cfgQuery();
  const img = document.getElementById("qrImg");
  const url = document.getElementById("qrUrl");
  if (img) img.src = "https://api.qrserver.com/v1/create-qr-code/?size=180x180&margin=8&data=" + encodeURIComponent(pico);
  if (url) url.textContent = pico;
}
function applyDisp() {
  const L = layoutOf(spec);
  const app = document.querySelector(".app");
  const stage = document.querySelector(".stage");
  const mon = document.getElementById("monitor");
  if (app) {
    app.classList.toggle("band-tiny", L.band === "tiny");
    app.classList.toggle("band-mid", L.band === "mid");
    app.classList.toggle("band-large", L.band === "large");
  }
  if (stage) stage.classList.toggle("framed", L.frame);
  if (mon) {
    mon.classList.toggle("framed", L.frame);
    mon.classList.toggle("band-tiny", L.band === "tiny");
    if (L.frame && stage) {
      const r = stage.getBoundingClientRect();
      const box = fitFrame(L.spec.w, L.spec.h, Math.max(40, r.width - 8), Math.max(40, r.height - 8));
      mon.style.width = box.width + "px";
      mon.style.height = box.height + "px";
    } else {
      mon.style.width = "";
      mon.style.height = "";
    }
    mon.style.setProperty("--type-scale", String(L.typeScale));
  }
  const note = document.getElementById("dispNote");
  if (note) note.textContent = L.summary;
  const viewSeg = document.getElementById("viewSeg");
  if (viewSeg) viewSeg.style.gridTemplateColumns = L.hybridOk ? "1fr 1fr 1fr" : "1fr 1fr";
  document.querySelectorAll("[data-preset]").forEach((b) => b.classList.toggle("on", b.dataset.preset === matchPreset(spec)));
  applyInstallLock();
  updateQr();
}
function applyQuery() {
  const q = new URLSearchParams(location.search);
  const fromQ = specFromQuery(q);
  if (fromQ) fillSpecInputs(fromQ);
  document.querySelectorAll("[data-preset]").forEach((x) => x.classList.toggle("on", x.dataset.preset === matchPreset(spec)));
  let v = (q.get("view") || q.get("face") || "").toLowerCase();
  if (v === "cards" || v === "board") v = "wall";
  if (v === "scope") v = "radar";
  if (v === "radar" || v === "wall" || v === "hybrid") {
    document.querySelectorAll("[data-view]").forEach((x) => x.classList.toggle("on", x.dataset.view === v));
    view = v;
  }
  const list = q.get("list");
  if (list === "fa" || list === "fids" || list === "carousel") {
    document.querySelectorAll("[data-list]").forEach((x) => x.classList.toggle("on", x.dataset.list === list));
    listStyle = list;
  }
  const th = q.get("theme");
  if (th && THEMES[th]) {
    document.querySelectorAll("[data-theme]").forEach((x) => x.classList.toggle("on", x.dataset.theme === th));
    theme = th;
  }
  const apt = (q.get("apt") || q.get("iata") || "").toUpperCase();
  if (/^[A-Z]{3}$/.test(apt)) {
    const a = (typeof AIRPORTS !== "undefined" ? AIRPORTS : SEED_AP).find((x) => x[0] === apt) || SEED_AP.find((x) => x[0] === apt);
    if (a) document.getElementById("preset").value = a[0] + " — " + a[1] + " · " + (a[4] || "");
  }
  const r = Number(q.get("r") || q.get("radius"));
  if (Number.isFinite(r) && r >= 40 && r <= 400) {
    document.getElementById("radius").value = String(r);
    radiusKm = r;
  }
  const rwy = (q.get("rwy") || q.get("pistas") || "").toLowerCase();
  if (rwy === "0" || rwy === "off" || rwy === "no") {
    runwaysOn = false;
    const el = document.getElementById("rwyOn");
    if (el) el.checked = false;
  } else if (rwy === "1" || rwy === "on" || rwy === "yes") {
    runwaysOn = true;
    const el = document.getElementById("rwyOn");
    if (el) el.checked = true;
  }
  if (q.get("install") === "1" || q.get("tab") === "install") showPane("tabInst");
  else if (q.get("tab") === "client") showPane("tabCfg");
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
function lonDelta(a, b) {
  return ((b - a + 540) % 360) - 180;
}
function wrapLon(lon, origin) {
  return origin + lonDelta(origin, lon);
}
function gcKm(a, b) {
  const lat1 = (a[0] * Math.PI) / 180;
  const lat2 = (b[0] * Math.PI) / 180;
  const dlon = ((b[1] - a[1]) * Math.PI) / 180;
  const dlat = lat2 - lat1;
  const h = Math.sin(dlat / 2) ** 2 + Math.cos(lat1) * Math.cos(lat2) * Math.sin(dlon / 2) ** 2;
  return 2 * 6371 * Math.asin(Math.min(1, Math.sqrt(h)));
}
function gcInterpolate(a, b, f) {
  const lat1 = (a[0] * Math.PI) / 180, lon1 = (a[1] * Math.PI) / 180;
  const lat2 = (b[0] * Math.PI) / 180, lon2 = (b[1] * Math.PI) / 180;
  const d = 2 * Math.asin(Math.sqrt(Math.sin((lat2 - lat1) / 2) ** 2 + Math.cos(lat1) * Math.cos(lat2) * Math.sin((lon2 - lon1) / 2) ** 2));
  if (!isFinite(d) || d < 1e-8) return [a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f];
  const A = Math.sin((1 - f) * d) / Math.sin(d);
  const B = Math.sin(f * d) / Math.sin(d);
  const x = A * Math.cos(lat1) * Math.cos(lon1) + B * Math.cos(lat2) * Math.cos(lon2);
  const y = A * Math.cos(lat1) * Math.sin(lon1) + B * Math.cos(lat2) * Math.sin(lon2);
  const z = A * Math.sin(lat1) + B * Math.sin(lat2);
  return [(Math.atan2(z, Math.hypot(x, y)) * 180) / Math.PI, (Math.atan2(y, x) * 180) / Math.PI];
}
function gcPath(a, b, n) {
  const steps = Math.max(8, n || 48);
  const out = [];
  for (let i = 0; i <= steps; i++) out.push(gcInterpolate(a, b, i / steps));
  return out;
}
function projectKm(lat, lon, track, km) {
  const rad = (track * Math.PI) / 180;
  return [lat + (Math.cos(rad) * km) / 111, lon + (Math.sin(rad) * km) / (111 * Math.cos((lat * Math.PI) / 180))];
}
function approachKm(r) {
  return Math.min(32, Math.max(18, r * 0.35));
}
function routeLookupKeys(flight) {
  const cs = compactId(flight);
  if (!cs || isReg(cs) || cs.length < 4 || !/[A-Z]{2,3}\d/.test(cs)) return [];
  const keys = [cs];
  const m = cs.match(/^([A-Z]{2,3})(\d+)$/);
  if (!m) return keys;
  const prefix = m[1], num = m[2];
  const pad = num.padStart(4, "0");
  if (pad !== num) keys.push(prefix + pad);
  const slim = String(Number(num));
  if (slim !== num) keys.push(prefix + slim);
  const al = ALINES.find((x) => x[0] === prefix || (x[1] && x[1] === prefix));
  if (al) {
    if (al[0] && al[0].length === 3 && al[0] !== prefix) keys.push(al[0] + num);
    if (al[1] && al[1].length === 2 && al[1] !== prefix) keys.push(al[1] + num);
  }
  return keys.filter((v, i, arr) => arr.indexOf(v) === i);
}
function matchesFollowFlight(ac, needle) {
  const n = compactId(needle);
  if (!n || n.length < 2) return false;
  const cs = compactId(ac.flight);
  const reg = compactId(ac.r || "");
  if ((cs && (cs.includes(n) || n.includes(cs))) || (reg && (reg.includes(n) || n.includes(reg)))) return true;
  const nNum = n.match(/^([A-Z]{2,3})(\d+[A-Z]?)$/);
  const csNum = cs.match(/^([A-Z]{2,3})(\d+[A-Z]?)$/);
  if (nNum && csNum && nNum[2] === csNum[2]) {
    if (nNum[1] === csNum[1]) return true;
    if (nNum[1].length === 2 && csNum[1].startsWith(nNum[1])) return true;
    if (csNum[1].length === 2 && nNum[1].startsWith(csNum[1])) return true;
  }
  const keys = routeLookupKeys(ac.flight);
  const nKeys = routeLookupKeys(needle);
  if (keys.some((k) => k.includes(n) || n.includes(k))) return true;
  if (nKeys.some((k) => k === cs || cs.includes(k))) return true;
  return false;
}
function followAcOf() {
  const n = compactId(followFlt);
  if (!n || n.length < 2 || !/\d/.test(n)) return null;
  return RAW.find((a) => matchesFollowFlight(a, followFlt)) || SKY.find((a) => matchesFollowFlight(a, followFlt)) || null;
}
function rivalAirports() {
  const apt = currentApt();
  return AIRPORTS.filter((a) => a[0] !== apt[0] && kmLL([a[2], a[3]], [apt[2], apt[3]]) < 45);
}
function nearbyRwy() {
  if (!runwaysOn) return [];
  const homeIata = currentApt()[0];
  const key = center[0] + "," + center[1] + "," + radiusKm + "," + AIRPORTS.length + "," + homeIata;
  if (rwyNear.key === key) return rwyNear.list;
  const includeNearby = radiusKm <= 110;
  const maxD = includeNearby ? Math.min(radiusKm * 0.95, 110) : 0;
  const skip = { EPA: 1, FDO: 1 };
  const out = [];
  for (let i = 0; i < AIRPORTS.length; i++) {
    const a = AIRPORTS[i];
    if (skip[a[0]]) continue;
    const rows = RUNWAYS[a[0]];
    if (!rows || !rows.length) continue;
    const d = kmLL(center, [a[2], a[3]]);
    const isHome = a[0] === homeIata;
    if (!isHome && (!includeNearby || d > maxD)) continue;
    out.push({ iata: a[0], lat: a[2], lon: a[3], rows: rows, d: d });
  }
  out.sort((a, b) => {
    if (a.iata === homeIata) return -1;
    if (b.iata === homeIata) return 1;
    return a.d - b.d;
  });
  rwyNear = { key: key, list: out.slice(0, 10) };
  return rwyNear.list;
}
function aptCity(code) {
  const a = AIRPORTS.find((x) => x[0] === code);
  return a ? (a[1] || "") : "";
}
function hdgDelta(x, y) { return Math.abs(((x - y + 540) % 360) - 180); }
function bearingLL(from, to) {
  const dlon = (to[1] - from[1]) * Math.PI / 180;
  const lat1 = from[0] * Math.PI / 180, lat2 = to[0] * Math.PI / 180;
  const y = Math.sin(dlon) * Math.cos(lat2);
  const x = Math.cos(lat1) * Math.sin(lat2) - Math.sin(lat1) * Math.cos(lat2) * Math.cos(dlon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
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
  const toward = a.track != null && hdgDelta(a.track, bearingLL([a.lat, a.lon], [apt[2], apt[3]])) < 28;
  const away = a.track != null && hdgDelta(a.track, bearingLL([apt[2], apt[3]], [a.lat, a.lon])) < 32;
  if (isGnd(a) && dSel < 8) origin = origin || apt[0];
  else if (gs > 70 && alt > 200 && alt < 14000 && dSel < 75 && toward) dest = dest || apt[0];
  else if (gs > 90 && alt > 600 && dSel < 36 && away) origin = origin || apt[0];
  return { origin: origin || "—", dest: dest || "—" };
}
function publishedRoute(a) {
  const o = (a.origin || "").toUpperCase();
  const d = (a.dest || "").toUpperCase();
  if (!o || !d || o === "—" || d === "—" || o.length < 3 || d.length < 3) return null;
  return { origin: o, dest: d };
}
function relatedToAirport(a) {
  const apt = currentApt();
  const pub = publishedRoute(a);
  if (pub) return pub.origin === apt[0] || pub.dest === apt[0];
  const here = [a.lat, a.lon];
  const dSel = kmLL(here, [apt[2], apt[3]]);
  const rivals = rivalAirports();
  const dRival = rivals.reduce((m, r) => Math.min(m, kmLL(here, [r[2], r[3]])), Infinity);
  if (dSel < 50 && isFinite(dRival) && dRival + 6 < dSel) return false;
  if (dSel < 6) {
    if (isGnd(a)) return true;
    const alt = Number(a.alt) || 0;
    const gs = Number(a.gs) || 0;
    if (alt < 2500 && gs < 200) return true;
  }
  const alt = Number(a.alt) || 0;
  const gs = Number(a.gs) || 0;
  if (dSel < 28 && alt > 0 && alt < 7000 && gs > 50 && a.track != null) {
    if (hdgDelta(a.track, bearingLL(here, [apt[2], apt[3]])) < 25) return true;
  }
  return false;
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
function flightSuggestRows() {
  const seen = {};
  const out = [];
  function add(a) {
    const id = compactId(a.flight);
    if (!id || seen[id]) return;
    if (isReg(id) && !isFlightNumber(id)) return;
    seen[id] = 1;
    const route = resolveRoute(a);
    const al = airlineOf(a.flight);
    out.push([id, displayId(a), (route.origin || "—") + " → " + (route.dest || "—"), al[1] || "", al[0] || "", a.hex]);
  }
  AC.forEach(add);
  SKY.forEach(add);
  RAW.forEach(add);
  return out;
}
(function bindFlightSuggest() {
  const input = document.getElementById("followFlt");
  const listEl = document.getElementById("sugFlt");
  if (!input || !listEl) return;
  function hits(q) {
    const rows = flightSuggestRows();
    const s = compactId(q);
    const f = fold(q);
    if (!s && !f) return rows.slice(0, 8);
    const ranked = [];
    rows.forEach((r) => {
      const id = compactId(r[0]);
      const al = fold(r[3] || "");
      const iata = compactId(r[4] || "");
      let n = 0;
      if (s && id === s) n = 100;
      else if (s && id.startsWith(s)) n = 90;
      else if (s && iata && (s === iata || id.startsWith(iata) && s.startsWith(iata))) n = 80;
      else if (s && id.includes(s)) n = 60;
      else if (f && al.includes(f)) n = 70;
      else if (f && fold(r[2]).includes(f)) n = 40;
      if (n) ranked.push({ r: r, n: n });
    });
    ranked.sort((a, b) => b.n - a.n);
    return ranked.slice(0, 12).map((x) => x.r);
  }
  function show(q) {
    const found = hits(q);
    listEl.innerHTML = found.map((r) =>
      '<button type="button" data-v="' + r[0] + '"><b>' + r[1] + "</b> " + r[2] + (r[3] ? " · " + r[3] : "") + "</button>"
    ).join("") || '<button type="button" disabled>Sin vuelos que coincidan</button>';
    listEl.style.display = "block";
  }
  function pick(code) {
    input.value = code;
    followFlt = code;
    listEl.style.display = "none";
    grabForm();
    applyFilters();
    renderWall();
    running = true;
    loadLive(false);
    if (shownView() !== "wall" && !looping) loop();
  }
  input.addEventListener("input", () => show(input.value));
  input.addEventListener("focus", () => show(input.value));
  input.addEventListener("keydown", (e) => {
    if (e.key !== "Enter") return;
    const first = listEl.querySelector("button[data-v]");
    if (!first) return;
    e.preventDefault();
    pick(first.dataset.v);
  });
  listEl.addEventListener("mousedown", (e) => {
    const b = e.target.closest("button");
    if (!b || !b.dataset.v) return;
    pick(b.dataset.v);
  });
  document.addEventListener("click", (e) => {
    if (!listEl.contains(e.target) && e.target !== input) listEl.style.display = "none";
  });
})();

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
  const pool = RAW.filter((a) => {
    const k = kindOf(a);
    if (!want[k]) return false;
    if (isGnd(a) && !includeGround) return false;
    if (alHit && !matchAirline(a.flight, alHit)) return false;
    return a.lat != null;
  });
  const zone = pool.filter((a) => distNmOf(a) <= radiusNm * 1.08 || relatedToAirport(a));
  zone.sort((a, b) => (relatedToAirport(b) - relatedToAirport(a)) || (looksAir(b) - looksAir(a)) || (distNmOf(a) - distNmOf(b)));
  SKY = zone.slice(0, 80);
  SKY.forEach(enrich);
  const hit = followAcOf();
  if (hit) {
    enrich(hit);
    AC = [hit];
  } else {
    AC = SKY.slice(0, maxN);
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
  return list.length ? { list: list, src: sources.join("+") || "vivo" } : await fetchSnapshot(lat, lon);
}
async function pullCallsign(cs) {
  const n = compactId(cs);
  if (!n) return [];
  const keys = [n];
  const slim = n.replace(/^([A-Z]{2,3})0+(\d)/, "$1$2");
  if (slim !== n) keys.push(slim);
  const byHex = {};
  for (let k = 0; k < keys.length && k < 3; k++) {
    const key = keys[k];
    const urls = [
      "https://opendata.adsb.fi/api/v2/callsign/" + encodeURIComponent(key),
      "https://api.adsb.lol/v2/callsign/" + encodeURIComponent(key),
    ];
    for (let i = 0; i < urls.length; i++) {
      try {
        const data = await pullJson(urls[i], 6000);
        normList(data).forEach((a) => { if (a && a.hex) byHex[a.hex] = a; });
        if (Object.keys(byHex).length) break;
      } catch (e) { /* next */ }
    }
    if (Object.keys(byHex).length) break;
  }
  return Object.keys(byHex).map((h) => byHex[h]);
}
async function fillRoutes(list) {
  const pending = list.filter((a) => {
    const cs = compactId(a.flight);
    if (isReg(cs) || ROUTECACHE[cs]) return false;
    return a.flight && cs.length >= 4 && /[A-Z]{2,3}\d/.test(cs);
  }).slice(0, 56);
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
  const nm = Math.min(250, Math.max(200, Math.round(radiusKm / 1.852) + 8));
  try {
    const got = await fetchSky(center[0], center[1], nm);
    srcLabel = got.src;
    lastOk = { list: got.list, src: got.src, at: Date.now() };
    RAW = got.list.map(norm).filter((a) => a.lat != null && a.hex);
    if (compactId(followFlt)) {
      try {
        const extra = await pullCallsign(followFlt);
        extra.forEach((row) => {
          const a = norm(row);
          if (!a.hex || a.lat == null) return;
          if (!RAW.some((x) => x.hex === a.hex)) RAW.push(a);
        });
      } catch (e) { /* seguir igual con el radar local */ }
    }
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
    fillRoutes(RAW).then(() => { applyFilters(); renderWall(); });
    const now = performance.now();
    RAW.forEach((a) => {
      POS[a.hex] = { lat: a.lat, lon: a.lon, t: now, gs: a.gs, track: a.track };
      const trail = TRAIL[a.hex] || [];
      const last = trail[trail.length - 1];
      if (!last || gcKm(last, [a.lat, a.lon]) > 1.4) {
        trail.push([a.lat, a.lon]);
        if (trail.length > 140) trail.shift();
        TRAIL[a.hex] = trail;
      }
    });
    lastSweep = now;
    lastRot = performance.now();
    const alQ = (document.getElementById("followAl").value || "").trim();
    const alHit = findAirline(alQ);
    if (!RAW.length) statusEl.textContent = "0 aeronaves en " + apt[0] + " (radio " + radiusKm + " km). Poca cobertura ADS-B en esta zona ahora.";
    else if (alQ && !AC.length) statusEl.textContent = RAW.length + " aviones en zona, 0 de " + (alHit ? alHit[2] : alQ);
    else {
      const gndN = AC.filter(isGnd).length;
      const airN = AC.filter((a) => looksAir(a) && !isGnd(a)).length;
      const ezeN = SKY.filter(relatedToAirport).length;
      const hit = followAcOf();
      if (hit) statusEl.textContent = "Siguiendo " + displayId(hit) + " · " + srcLabel;
      else statusEl.textContent = RAW.length + " aeronaves · " + SKY.length + " en radar · " + ezeN + " de " + apt[0] + " · " + srcLabel + (gndN ? " · " + gndN + " en tierra" : "") + (airN === 0 && RAW.length ? " · poca cobertura comercial" : "");
    }
    foot.textContent = (srcLabel === "snapshot" ? "ADS-B (copia local) · " : "ADS-B en vivo · ") + srcLabel;
  } catch (e) {
    if (lastOk && Date.now() - lastOk.at < 15 * 60 * 1000) {
      srcLabel = lastOk.src + " (cache)";
      RAW = lastOk.list.map(norm).filter((a) => a.lat != null && a.hex);
      applyFilters(); renderWall();
      statusEl.textContent = RAW.length + " aeronaves · " + apt[0] + " · " + srcLabel;
    } else {
      RAW = []; AC = []; SKY = []; renderWall();
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
  const distKm = a.dst != null && isFinite(Number(a.dst)) ? Math.round(Number(a.dst) * 1.852) : (a.dstNm != null ? Math.round(a.dstNm * 1.852) : null);
  const extraBits = [];
  if (a.t) extraBits.push("Tipo " + a.t);
  if (!isReg(id) && a.r) extraBits.push("Matrícula " + String(a.r).replace(/[^A-Z0-9]/gi, "").toUpperCase());
  if (distKm != null) extraBits.push(distKm + " km al centro");
  const destLL = aptCoord(route.dest);
  if (!gnd && destLL) extraBits.push(Math.round(kmLL([a.lat, a.lon], destLL)) + " km a " + route.dest);
  const pill = '<span class="' + pillClass(st) + '">' + st + "</span>";
  const following = compactId(followFlt) && matchesFollowFlight(a, followFlt);
  const hint = following ? "Siguiendo · toca para soltar" : "Toca para seguir";
  return (
    '<div class="fa-card' + (following ? " following" : "") + '" data-hex="' + a.hex + '" role="button" tabindex="0">' +
      '<div class="fa-row"><div class="logo">' + logoHTML(a) + '</div><div class="fa-meta">' +
        '<div class="fa-top"><span class="fa-id">' + id + "</span>" + pill + "</div>" +
        '<div class="fa-air">' + sub + "</div>" +
      "</div></div>" +
      '<div class="fa-route"><span>' + route.origin + '</span><span class="bar"><i style="width:' + pct + '%"></i></span><span>' + route.dest + "</span></div>" +
      '<div class="fa-cities"><span>' + aptCity(route.origin) + "</span><span>" + aptCity(route.dest) + "</span></div>" +
      '<div class="fa-times">' +
        '<div><span class="lbl">Despegue ' + a.depKind + "</span><b>" + hhmm(a.depAt) + "</b></div>" +
        '<div><span class="lbl">Aterrizaje ' + a.arrKind + "</span><b>" + hhmm(a.arrAt) + "</b></div>" +
      "</div>" +
      '<div class="metrics">' +
        '<div><span class="lbl">Altitud</span><b>' + (gnd ? "GND" : fmtAlt(a.alt)) + "</b></div>" +
        '<div><span class="lbl">Velocidad</span><b>' + (gnd ? "—" : fmtGs(a.gs)) + "</b></div>" +
        '<div><span class="lbl">Rumbo</span><b>' + (gnd || a.track == null ? "—" : '<span class="hdg"><span class="arr" style="transform:rotate(' + Number(a.track) + 'deg)"></span> ' + fmtHdg(a.track) + "</span>") + "</b></div>" +
      "</div>" +
      (extraBits.length ? '<div class="fa-extra">' + extraBits.join(" · ") + "</div>" : "") +
      '<div class="fa-foot"><span>' + foot + '</span><span class="fa-follow">' + hint + "</span>" + pill + "</div>" +
    "</div>"
  );
}
function clipCards() {
  if (wall.querySelector(".carousel-wrap")) return;
  if (listStyle === "carousel" || listStyle === "fids") return;
  const box = wall.getBoundingClientRect();
  [].slice.call(wall.querySelectorAll(".fa-card")).forEach((c) => {
    const r = c.getBoundingClientRect();
    if (r.bottom > box.bottom + 2 || r.top < box.top - 2) c.style.display = "none";
  });
}
function visibleList() {
  if (!AC.length) return [];
  const L = layoutOf(spec);
  if (L.band === "tiny" && listStyle !== "fids") {
    return [AC[page % AC.length]];
  }
  if (listStyle === "carousel") {
    const n = Math.max(1, Math.min(carouselN, AC.length));
    const start = (page * n) % AC.length;
    const out = [];
    for (let i = 0; i < n; i++) out.push(AC[(start + i) % AC.length]);
    return out;
  }
  const h = wall.clientHeight || 300;
  const v = shownView();
  const minH = listStyle === "fids" ? (v === "wall" ? 64 : 36) : (v === "wall" ? 210 : 168);
  const n = Math.max(1, Math.floor((h - 16) / minH));
  const start = (page * n) % Math.max(1, AC.length);
  const out = [];
  for (let i = 0; i < Math.min(n, AC.length); i++) out.push(AC[(start + i) % AC.length]);
  return out;
}
function overhead() {
  if (layoutOf(spec).band === "tiny") return null;
  if (demoOver) return enrich({ hex: "demo", flight: "ARG1716", t: "A320", lat: center[0], lon: center[1], alt: 4200, gs: 210, track: 180, origin: "AEP", dest: "COR" });
  if (!document.getElementById("house").checked || !home) return null;
  let best = null, bestD = 9;
  AC.forEach((a) => { const d = kmLL([displayPos(a).lat, displayPos(a).lon], home); if (d < bestD) { bestD = d; best = a; } });
  return best;
}
let overHex = "";
let overAt = 0;
function updateOver() {
  const el = document.getElementById("overCard");
  const a = overhead();
  if (!a || shownView() === "wall") { el.style.display = "none"; return; }
  if (a.hex !== overHex) { overHex = a.hex; overAt = performance.now(); }
  if (performance.now() - overAt > 7000) {
    el.style.display = "none";
    if (demoOver) demoOver = false;
    return;
  }
  el.style.display = "block";
  el.innerHTML = cardHTML(a);
}
function renderWall() {
  const view = shownView();
  applyTheme();
  applyDisp();
  updateOver();
  radar.style.display = view === "wall" ? "none" : "block";
  wall.classList.toggle("on", view !== "radar");
  wall.classList.toggle("hybrid", view === "hybrid");
  wall.classList.toggle("full", view === "wall");
  if (!AC.length) {
    wall.innerHTML = compactId(followFlt)
      ? '<div class="empty"><div>Buscando ' + followFlt + "…</div><p class=\"fa-air\">Lo busco por indicativo en el ADS-B, aunque esté lejos del aeropuerto.</p></div>"
      : SKY.length
      ? '<div class="empty"><div>Sin vuelos de ' + currentApt()[0] + ' ahora</div><p class="fa-air">Hay tráfico en la zona, ninguno entra en esta radio.</p></div>'
      : '<div class="empty"><div>' + (loading ? "Cargando el cielo…" : "Sin vuelos en este radio") + '</div><p class="fa-air">' + (statusEl.textContent || "") + "</p></div>";
    return;
  }
  const followOne = !!followAcOf() && AC.length === 1;
  const list = followOne ? AC : visibleList();
  if (followOne || listStyle === "carousel" || listStyle === "fa") {
    const cls = list.length === 1 ? " hero" : list.length <= 2 ? " lg" : list.length === 3 ? " md" : "";
    wall.innerHTML = '<div class="carousel-wrap' + cls + '" style="grid-template-rows:repeat(' + list.length + ',minmax(0,1fr))">' +
      list.map(cardHTML).join("") + "</div>";
    wall.classList.toggle("hero", list.length === 1);
    wall.classList.toggle("lg", list.length > 1 && list.length <= 2);
  } else if (listStyle === "fids") {
    const compact = view === "hybrid";
    const shortSt = (st) => st === "APROXIMANDO" ? "APROX" : st === "EN TIERRA" ? "TIERRA" : st === "EN VUELO" ? "VUELO" : st;
    wall.innerHTML = '<table class="fids"><thead><tr><th></th><th>VUELO</th><th>RUTA</th>' +
      (compact ? "" : "<th>TIPO</th>") +
      "<th>DEP</th><th>ARR</th>" +
      (compact ? "" : "<th>LLEGA</th>") +
      "<th>ESTADO</th></tr></thead><tbody>" +
      list.map((a) => {
        const route = resolveRoute(a);
        const st = a.status || phaseOf(a);
        const id = displayId(a);
        const pair = airlineOf(a.flight);
        const eta = (st === "EN TIERRA" || !a.arrAt) ? "—" : (Math.max(0, Math.round((a.arrAt - Date.now()) / 60000)) + " min");
        const cities = compact ? "" : '<div class="fid-sub">' + (aptCity(route.origin) || route.origin) + " → " + (aptCity(route.dest) || route.dest) + "</div>";
        const sub = compact ? "" : '<div class="fid-sub">' + (isReg(id) ? "Matrícula" : pair[1]) + (a.r && !isReg(id) ? " · " + String(a.r).replace(/[^A-Z0-9]/gi, "").toUpperCase() : "") + "</div>";
        return '<tr data-hex="' + a.hex + '" role="button" tabindex="0"><td><div class="logo" style="width:26px;height:26px;font-size:9px">' + logoHTML(a) + "</div></td>" +
          "<td>" + id + sub + "</td>" +
          '<td class="' + (compact ? "" : "wrap") + '">' + route.origin + " → " + route.dest + cities + "</td>" +
          (compact ? "" : "<td>" + (a.t || "—") + "</td>") +
          "<td>" + hhmm(a.depAt) + "</td><td>" + hhmm(a.arrAt) + "</td>" +
          (compact ? "" : "<td>" + eta + "</td>") +
          '<td><span class="' + pillClass(st) + '">' + (compact ? shortSt(st) : st) + "</span></td></tr>";
      }).join("") +
      "</tbody></table>";
  } else {
    wall.innerHTML = '<div class="carousel-wrap' + (list.length === 1 ? " hero" : list.length <= 2 ? " lg" : "") + '" style="grid-template-rows:repeat(' + list.length + ',minmax(0,1fr))">' +
      list.map(cardHTML).join("") + "</div>";
    wall.classList.toggle("hero", list.length === 1);
    wall.classList.toggle("lg", list.length > 1 && list.length <= 2);
  }
  requestAnimationFrame(clipCards);
}
function angDiff(a, b) {
  let d = a - b;
  while (d > Math.PI) d -= Math.PI * 2;
  while (d < -Math.PI) d += Math.PI * 2;
  return d;
}
function drawJourney(ctx, W, H, left, fg, ac) {
  const x0 = 18, y0 = 36;
  const x1 = W * left - 16, y1 = H - 18;
  const boxW = Math.max(80, x1 - x0), boxH = Math.max(80, y1 - y0);
  const pNow = displayPos(ac);
  const route = resolveRoute(ac);
  const orig = AIRPORTS.find((x) => x[0] === route.origin) || null;
  const dest = AIRPORTS.find((x) => x[0] === route.dest) || null;
  const oApt = orig || ((!dest) ? currentApt() : null);
  const dApt = dest;
  const flown = TRAIL[ac.hex] || [];
  const pts = [[pNow.lat, pNow.lon]].concat(flown);
  if (oApt) pts.push([oApt[2], oApt[3]]);
  if (dApt) pts.push([dApt[2], dApt[3]]);
  if (pts.length < 2 && ac.track != null) {
    pts.push(projectKm(pNow.lat, pNow.lon, ac.track, 320));
    pts.push(projectKm(pNow.lat, pNow.lon, (ac.track + 180) % 360, 80));
  }
  const anchorLon = oApt ? oApt[3] : (dApt ? dApt[3] : pNow.lon);
  const lons = pts.map((p) => wrapLon(p[1], anchorLon));
  const lats = pts.map((p) => p[0]);
  let minLat = Math.min.apply(null, lats), maxLat = Math.max.apply(null, lats);
  let minLon = Math.min.apply(null, lons), maxLon = Math.max.apply(null, lons);
  const latPad = Math.max(4, (maxLat - minLat) * 0.18 + 2.4);
  const lonPad = Math.max(4, (maxLon - minLon) * 0.18 + 2.4);
  minLat -= latPad; maxLat += latPad; minLon -= lonPad; maxLon += lonPad;
  const latSpan = Math.max(6, maxLat - minLat), lonSpan = Math.max(6, maxLon - minLon);
  const scale = Math.min(boxW / lonSpan, boxH / latSpan);
  const midLat = (minLat + maxLat) / 2, midLon = (minLon + maxLon) / 2;
  const toXY = (lat, lon) => ({
    x: x0 + boxW / 2 + (wrapLon(lon, anchorLon) - midLon) * scale,
    y: y0 + boxH / 2 - (lat - midLat) * scale,
  });
  ctx.save();
  ctx.beginPath();
  ctx.rect(x0, y0, boxW, boxH);
  ctx.clip();
  if (LAND.length) {
    ctx.fillStyle = fg;
    ctx.strokeStyle = fg;
    for (let i = 0; i < LAND.length; i++) {
      const ring = LAND[i];
      if (!ring || ring.length < 4) continue;
      let a = 90, b = -90, c = 1e9, d = -1e9;
      for (let j = 0; j < ring.length; j++) {
        const lat = ring[j][0], lon = wrapLon(ring[j][1], anchorLon);
        if (lat < a) a = lat; if (lat > b) b = lat;
        if (lon < c) c = lon; if (lon > d) d = lon;
      }
      if (b < minLat || a > maxLat || d < minLon || c > maxLon) continue;
      ctx.beginPath();
      for (let j = 0; j < ring.length; j++) {
        const p = toXY(ring[j][0], ring[j][1]);
        if (j === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y);
      }
      ctx.closePath();
      ctx.globalAlpha = 0.16;
      ctx.fill();
      ctx.globalAlpha = 0.5;
      ctx.lineWidth = 1.15;
      ctx.stroke();
    }
  }
  ctx.strokeStyle = fg; ctx.globalAlpha = 0.22; ctx.lineWidth = 1;
  ctx.strokeRect(x0, y0, boxW, boxH);
  const latStep = latSpan > 40 ? 10 : latSpan > 18 ? 5 : 2;
  const lonStep = lonSpan > 40 ? 10 : lonSpan > 18 ? 5 : 2;
  ctx.font = "10px ui-monospace, monospace"; ctx.fillStyle = fg;
  for (let lat = Math.ceil(minLat / latStep) * latStep; lat <= maxLat; lat += latStep) {
    const p = toXY(lat, midLon);
    ctx.globalAlpha = 0.12; ctx.beginPath(); ctx.moveTo(x0, p.y); ctx.lineTo(x0 + boxW, p.y); ctx.stroke();
    ctx.globalAlpha = 0.4; ctx.fillText(lat.toFixed(0) + "°", x0 + 4, p.y - 3);
  }
  for (let lon = Math.ceil(minLon / lonStep) * lonStep; lon <= maxLon; lon += lonStep) {
    const p = toXY(midLat, lon);
    ctx.globalAlpha = 0.12; ctx.beginPath(); ctx.moveTo(p.x, y0); ctx.lineTo(p.x, y0 + boxH); ctx.stroke();
  }
  ctx.restore();
  ctx.save();
  ctx.beginPath();
  ctx.rect(x0, y0, boxW, boxH);
  ctx.clip();
  const oLL = oApt ? [oApt[2], oApt[3]] : null;
  const dLL = dApt ? [dApt[2], dApt[3]] : null;
  const fullRoute = oLL && dLL ? gcPath(oLL, dLL, 56) : [];
  const remain = dLL ? gcPath([pNow.lat, pNow.lon], dLL, 40) : [];
  const path = remain.length ? remain : fullRoute;
  if (fullRoute.length) {
    ctx.globalAlpha = 0.35; ctx.strokeStyle = fg; ctx.lineWidth = 1.2; ctx.setLineDash([4, 8]);
    ctx.beginPath();
    fullRoute.forEach((pt, i) => { const p = toXY(pt[0], pt[1]); if (i === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
    ctx.stroke(); ctx.setLineDash([]);
  }
  if (remain.length) {
    ctx.globalAlpha = 0.95; ctx.strokeStyle = fg; ctx.lineWidth = 2; ctx.setLineDash([8, 5]);
    ctx.beginPath();
    remain.forEach((pt, i) => { const p = toXY(pt[0], pt[1]); if (i === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
    ctx.stroke(); ctx.setLineDash([]);
  }
  if (flown.length > 1) {
    ctx.globalAlpha = 0.9; ctx.strokeStyle = fg; ctx.lineWidth = 2.2;
    ctx.beginPath();
    flown.forEach((pt, i) => { const p = toXY(pt[0], pt[1]); if (i === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
    ctx.stroke();
  }
  const used = {};
  if (oApt) used[oApt[0]] = 1;
  if (dApt) used[dApt[0]] = 1;
  if (path.length) {
    [0.18, 0.36, 0.54, 0.72, 0.88].forEach((f) => {
      const pt = path[Math.round(f * (path.length - 1))];
      let best = null, bestD = 220;
      for (let i = 0; i < AIRPORTS.length; i++) {
        const a = AIRPORTS[i];
        if (used[a[0]]) continue;
        const d = gcKm([a[2], a[3]], pt);
        if (d < bestD) { bestD = d; best = a; }
      }
      ctx.fillStyle = fg; ctx.font = "10px ui-monospace, monospace";
      if (best) {
        used[best[0]] = 1;
        const p = toXY(best[2], best[3]);
        ctx.globalAlpha = 0.85; ctx.beginPath(); ctx.arc(p.x, p.y, 3.2, 0, Math.PI * 2); ctx.fill();
        ctx.globalAlpha = 0.7; ctx.fillText(best[0], p.x + 6, p.y - 5);
      } else {
        const p = toXY(pt[0], pt[1]);
        ctx.globalAlpha = 0.85; ctx.beginPath(); ctx.arc(p.x, p.y, 3.2, 0, Math.PI * 2); ctx.fill();
        ctx.globalAlpha = 0.7; ctx.fillText(Math.round(f * 100) + "%", p.x + 6, p.y - 5);
      }
    });
  }
  [[oApt, "ORIGEN"], [dApt, "DESTINO"]].forEach((pair) => {
    const apt = pair[0], tag = pair[1];
    if (!apt) return;
    const p = toXY(apt[2], apt[3]);
    ctx.globalAlpha = 1; ctx.strokeStyle = fg; ctx.lineWidth = 1.6;
    ctx.beginPath(); ctx.arc(p.x, p.y, 7, 0, Math.PI * 2); ctx.stroke();
    ctx.fillStyle = fg; ctx.font = "13px ui-monospace, monospace";
    ctx.fillText(apt[0], p.x + 10, p.y - 6);
    ctx.globalAlpha = 0.7; ctx.font = "10px sans-serif";
    ctx.fillText(tag + " · " + (apt[1] || apt[5] || ""), p.x + 10, p.y + 8);
  });
  const plane = toXY(pNow.lat, pNow.lon);
  ctx.globalAlpha = 0.9; ctx.strokeStyle = fg;
  ctx.beginPath(); ctx.arc(plane.x, plane.y, 14, 0, Math.PI * 2); ctx.stroke();
  ctx.fillStyle = fg;
  ctx.save(); ctx.translate(plane.x, plane.y); ctx.rotate(((ac.track || 0) * Math.PI) / 180);
  ctx.beginPath(); ctx.moveTo(0, -9); ctx.lineTo(5.5, 8); ctx.lineTo(-5.5, 8); ctx.closePath(); ctx.fill();
  ctx.restore();
  ctx.font = "12px ui-monospace, monospace";
  ctx.fillText((ac.flight || "").trim() || ac.hex, plane.x + 16, plane.y - 8);
  ctx.restore();
  const rem = dLL && ac.alt !== "ground" ? Math.round(gcKm([pNow.lat, pNow.lon], dLL)) : null;
  ctx.globalAlpha = 1; ctx.fillStyle = fg; ctx.font = "12px ui-monospace, monospace";
  ctx.fillText(((ac.flight || "").trim() || currentApt()[0]) + "  " + route.origin + " → " + route.dest, 10, 18);
  ctx.globalAlpha = 0.7; ctx.font = "11px sans-serif";
  ctx.fillText(rem != null ? ("Ruta del viaje · " + rem + " km al destino") : "Ruta del viaje", 10, 32);
  ctx.globalAlpha = 1;
}
function loop() {
  const view = shownView();
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
  const tripAc = followAcOf();
  if (performance.now() - lastSweep > (tripAc ? 20000 : 55000)) { lastSweep = performance.now(); loadLive(false); }
  if (performance.now() - lastRot > rotateS * 1000) { lastRot = performance.now(); page++; renderWall(); }
  if (tripAc) {
    drawJourney(ctx, w, h, left, pair.fg, tripAc);
    requestAnimationFrame(loop);
    return;
  }
  const cx = w * left * 0.5, cy = h * 0.52;
  const R = Math.min(cx, cy) * 0.86;
  ctx.strokeStyle = pair.fg + "28"; ctx.lineWidth = 1;
  [0.25, 0.5, 0.75, 1].forEach((r) => { ctx.beginPath(); ctx.arc(cx, cy, R * r, 0, Math.PI * 2); ctx.stroke(); });
  ctx.beginPath(); ctx.moveTo(cx, cy - R); ctx.lineTo(cx, cy + R); ctx.moveTo(cx - R, cy); ctx.lineTo(cx + R, cy); ctx.stroke();
  beam += 0.012; if (beam > Math.PI * 3) beam -= Math.PI * 2;
  ctx.strokeStyle = pair.fg; ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(cx + Math.cos(beam) * R, cy + Math.sin(beam) * R); ctx.stroke();
  ctx.fillStyle = pair.fg; ctx.font = "12px ui-monospace, monospace";
  ctx.fillText(currentApt()[0], 10, 18);
  ctx.globalAlpha = 0.7; ctx.font = "11px sans-serif";
  ctx.fillText(currentApt()[1] || "", 10, 34);
  ctx.globalAlpha = 1;
  const span = Math.max(0.25, radiusKm / 111);
  const toXY = (lat, lon) => ({ x: cx + ((lon - center[1]) / span) * R, y: cy - ((lat - center[0]) / span) * R });
  const sets = nearbyRwy();
  const apxKm = approachKm(radiusKm);
  const homeIata = currentApt()[0];
  sets.forEach((set) => {
    const rows = set.rows;
    const isHome = set.iata === homeIata;
    rows.forEach((rw) => {
      const a = toXY(rw[4], rw[5]), b = toXY(rw[6], rw[7]);
      ctx.strokeStyle = pair.fg; ctx.globalAlpha = isHome ? 0.95 : 0.72;
      const len = Math.hypot(b.x - a.x, b.y - a.y);
      ctx.lineCap = "butt"; ctx.lineJoin = "miter";
      ctx.lineWidth = Math.max(1.15, Math.min(2.4, len * 0.14));
      ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
      if ((isHome || radiusKm < 90) && len > 10) {
        ctx.globalAlpha = 0.9; ctx.font = "10px ui-monospace, monospace"; ctx.fillStyle = pair.fg;
        ctx.fillText(String(rw[0]), a.x + 6, a.y - 4);
        ctx.fillText(String(rw[1]), b.x + 6, b.y - 4);
      }
    });
    const mid = toXY(set.lat, set.lon);
    ctx.globalAlpha = isHome ? 0.95 : 0.7; ctx.font = "11px ui-monospace, monospace"; ctx.fillStyle = pair.fg;
    ctx.fillText(set.iata, mid.x + 8, mid.y + 14);
    const scored = {};
    SKY.forEach((ac) => {
      if (!looksAir(ac) || isGnd(ac) || ac.track == null) return;
      const alt = Number(ac.alt) || 0, gs = Number(ac.gs) || 0;
      if (alt > 12000 || gs < 80) return;
      rows.forEach((rw) => {
        [[rw[0], rw[2], rw[4], rw[5]], [rw[1], rw[3], rw[6], rw[7]]].forEach((end) => {
          const ident = String(end[0]), hdg = Number(end[1]), lat = end[2], lon = end[3];
          if (!ident || !isFinite(hdg)) return;
          if (hdgDelta(ac.track, hdg) > 12) return;
          const km = kmLL([ac.lat, ac.lon], [lat, lon]);
          const along = Math.cos((ac.track - bearingLL([ac.lat, ac.lon], [lat, lon])) * Math.PI / 180) * km;
          if (along <= 1.5 || along >= 40) return;
          scored[ident] = scored[ident] || { ident, hdg, lat, lon, n: 0, minAlong: 99 };
          scored[ident].n += 1;
          if (along < scored[ident].minAlong) scored[ident].minAlong = along;
        });
      });
    });
    const ranked = Object.keys(scored).map((k) => scored[k]).sort((a, b) => b.n - a.n || a.minAlong - b.minAlong);
    const hitIds = {};
    if (ranked[0]) hitIds[ranked[0].ident] = 1;
    function drawApx(lat, lon, hdg, ident, strong) {
      const back = projectKm(lat, lon, (hdg + 180) % 360, apxKm);
      let s = toXY(back[0], back[1]);
      const t = toXY(lat, lon);
      const dx = s.x - t.x, dy = s.y - t.y;
      const plen = Math.hypot(dx, dy);
      if (isHome && plen < 56 && plen > 0.2) {
        const k = 56 / plen;
        s = { x: t.x + dx * k, y: t.y + dy * k };
      }
      ctx.setLineDash(strong ? [10, 5] : [7, 7]);
      ctx.strokeStyle = pair.fg;
      ctx.globalAlpha = strong ? 0.95 : isHome ? 0.62 : 0.28;
      ctx.lineCap = "butt";
      ctx.lineWidth = strong ? 2.2 : isHome ? 1.5 : 1.2;
      ctx.beginPath(); ctx.moveTo(s.x, s.y); ctx.lineTo(t.x, t.y); ctx.stroke();
      ctx.setLineDash([]);
      if (isHome || strong) {
        ctx.fillStyle = pair.fg;
        ctx.globalAlpha = strong ? 0.95 : 0.7;
        ctx.font = "11px ui-monospace, monospace";
        ctx.fillText(strong ? "APX " + ident : ident, s.x + 4, s.y - 4);
      }
    }
    if (isHome) {
      rows.forEach((rw) => {
        if (rw[0] && isFinite(Number(rw[2]))) drawApx(rw[4], rw[5], Number(rw[2]), String(rw[0]), !!hitIds[rw[0]]);
        if (rw[1] && isFinite(Number(rw[3]))) drawApx(rw[6], rw[7], Number(rw[3]), String(rw[1]), !!hitIds[rw[1]]);
      });
    } else if (radiusKm <= 70 && ranked[0]) {
      drawApx(ranked[0].lat, ranked[0].lon, ranked[0].hdg, ranked[0].ident, true);
    }
  });
  ctx.globalAlpha = 1;
  const labeled = new Set(visibleList().map((a) => a.hex));
  SKY.forEach((a) => {
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
  spec = clampSpec({
    inches: Number(document.getElementById("inIn") && document.getElementById("inIn").value),
    w: Number(document.getElementById("vgaW") && document.getElementById("vgaW").value),
    h: Number(document.getElementById("vgaH") && document.getElementById("vgaH").value),
  });
  document.querySelectorAll("[data-preset]").forEach((b) => b.classList.toggle("on", b.dataset.preset === matchPreset(spec)));
  const apt = currentApt();
  center = [apt[2], apt[3]];
  followAl = document.getElementById("followAl").value.trim();
  followFlt = document.getElementById("followFlt").value.trim();
  includeGround = document.getElementById("ground").checked;
  runwaysOn = !!(document.getElementById("rwyOn") && document.getElementById("rwyOn").checked);
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
  const rwyLive = document.getElementById("rwyLive");
  if (rwyLive) rwyLive.textContent = runwaysOn
    ? "Las pistas de " + currentApt()[0] + " se dibujan siempre. La aproximación punteada se alarga con el radio (hasta ~32 km)."
    : "Pistas apagadas. El cliente las puede volver a prender.";
  const rwyClient = document.getElementById("rwyClientNote");
  if (rwyClient) rwyClient.hidden = runwaysOn;
  applyTheme();
  applyDisp();
  saveInstall();
  setMeta();
}
document.getElementById("tabMon").onclick = function () { showPane("tabMon"); };
document.getElementById("tabCfg").onclick = function () { showPane("tabCfg"); };
document.getElementById("tabInst").onclick = function () { showPane("tabInst"); };
document.querySelectorAll("[data-view],[data-list],[data-theme]").forEach((b) => {
  b.addEventListener("click", () => {
    const key = b.dataset.view ? "view" : b.dataset.list ? "list" : "theme";
    document.querySelectorAll("[data-" + key + "]").forEach((x) => x.classList.remove("on"));
    b.classList.add("on");
    grabForm(); applyFilters(); renderWall();
    running = true; if (shownView() !== "wall" && !looping) loop();
  });
});
document.querySelectorAll("[data-preset]").forEach((b) => {
  b.addEventListener("click", () => {
    if (installLocked) return;
    const id = b.dataset.preset;
    if (id && id !== "custom") {
      const p = PRESETS.find((x) => x.id === id);
      if (p) fillSpecInputs(p.spec);
    } else {
      document.querySelectorAll("[data-preset]").forEach((x) => x.classList.remove("on"));
      b.classList.add("on");
    }
    grabForm(); applyFilters(); renderWall();
    running = true; if (shownView() !== "wall" && !looping) loop();
  });
});
["inIn", "vgaW", "vgaH"].forEach((id) => {
  const el = document.getElementById(id);
  if (!el) return;
  el.addEventListener("change", () => {
    if (installLocked) return;
    fillSpecInputs(clampSpec({
      inches: Number(document.getElementById("inIn").value),
      w: Number(document.getElementById("vgaW").value),
      h: Number(document.getElementById("vgaH").value),
    }));
    grabForm(); applyFilters(); renderWall();
    running = true; if (shownView() !== "wall" && !looping) loop();
  });
  el.addEventListener("input", () => {
    spec = {
      inches: Number(document.getElementById("inIn").value) || spec.inches,
      w: Number(document.getElementById("vgaW").value) || spec.w,
      h: Number(document.getElementById("vgaH").value) || spec.h,
    };
    applyDisp();
    setMeta();
  });
});
["fAir", "fGa", "fBiz", "fHeli", "ground", "house", "followFlt", "max", "rotate", "carN", "rwyOn"].forEach((id) => {
  document.getElementById(id).addEventListener("change", () => { grabForm(); applyFilters(); renderWall(); });
});
document.getElementById("followFlt").addEventListener("input", () => { grabForm(); applyFilters(); renderWall(); });
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
  grabForm(); loadLive(true); running = true; if (shownView() !== "wall" && !looping) loop();
};
document.getElementById("qrTry").onclick = function () {
  grabForm();
  const qs = cfgQuery();
  history.replaceState({}, "", location.pathname + "?" + qs);
};
document.getElementById("installLock").onclick = function () {
  installLocked = !installLocked;
  if (installLocked) spec = clampSpec(spec);
  saveInstall();
  applyInstallLock();
};
document.getElementById("installTry").onclick = function () {
  grabForm();
  history.replaceState({}, "", location.pathname + "?" + installQuery());
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

function toggleFollow(a) {
  const id = compactId(a.flight) || compactId(a.r || "") || a.hex;
  const inp = document.getElementById("followFlt");
  if (followFlt && matchesFollowFlight(a, followFlt)) {
    followFlt = "";
    if (inp) inp.value = "";
  } else {
    followFlt = id;
    if (inp) inp.value = id;
  }
  applyFilters();
  renderWall();
  running = true;
  if (followFlt) loadLive(false);
  if (shownView() !== "wall" && !looping) loop();
}
function pickFromEvent(e) {
  const el = e.target.closest("[data-hex]");
  if (!el) return;
  const hex = el.dataset.hex;
  const a = RAW.find((x) => x.hex === hex) || AC.find((x) => x.hex === hex) || SKY.find((x) => x.hex === hex);
  if (a) toggleFollow(a);
}
wall.addEventListener("click", pickFromEvent);
wall.addEventListener("keydown", (e) => {
  if (e.key !== "Enter" && e.key !== " ") return;
  e.preventDefault();
  pickFromEvent(e);
});
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
  try {
    const land = await pullJson("land.json", 8000);
    if (Array.isArray(land) && land.length) LAND = land;
  } catch (e) { /* optional */ }
}

async function boot() {
  loadInstall();
  fillSpecInputs(spec);
  applyQuery();
  grabForm();
  applyDisp();
  running = true; if (shownView() !== "wall") loop();
  loadLive(true);
  await loadCatalogs();
  applyQuery();
  grabForm();
  applyDisp();
}
boot();
setInterval(() => { if (shownView() !== "wall") return; if (performance.now() - lastRot > rotateS * 1000) { lastRot = performance.now(); page++; renderWall(); } }, 400);
window.addEventListener("resize", () => renderWall());
