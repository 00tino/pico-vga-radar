export default async function handler(req, res) {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "*");
  if (req.method === "OPTIONS") {
    res.status(204).end();
    return;
  }
  const lat = Number(req.query.lat);
  const lon = Number(req.query.lon);
  const dist = Math.min(250, Math.max(15, Math.round(Number(req.query.dist) || 80)));
  if (!Number.isFinite(lat) || !Number.isFinite(lon)) {
    res.status(400).json({ error: "bad coords", ac: [] });
    return;
  }
  const urls = [
    ["https://opendata.adsb.fi/api/v2/lat/" + lat + "/lon/" + lon + "/dist/" + dist, "adsb.fi"],
    ["https://api.adsb.lol/v2/lat/" + lat + "/lon/" + lon + "/dist/" + dist, "adsb.lol"],
  ];
  for (const [url, source] of urls) {
    try {
      const r = await fetch(url, {
        headers: { accept: "application/json", "user-agent": "pico-radar/1.0" },
        signal: AbortSignal.timeout(source === "adsb.fi" ? 8000 : 7000),
      });
      if (!r.ok) continue;
      const data = await r.json();
      const list = (data && (data.ac || data.aircraft)) || [];
      if (!Array.isArray(list)) continue;
      res.setHeader("Cache-Control", "s-maxage=12, stale-while-revalidate=45");
      res.status(200).json({ ac: list, aircraft: list, source, now: Date.now() / 1000 });
      return;
    } catch (e) {
      /* try next */
    }
  }
  res.status(502).json({ error: "sky down", ac: [] });
}
