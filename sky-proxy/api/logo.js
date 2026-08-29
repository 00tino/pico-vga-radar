export default async function handler(req, res) {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  if (req.method === "OPTIONS") {
    res.status(204).end();
    return;
  }
  const code = String(req.query.code || "")
    .toUpperCase()
    .replace(/[^A-Z0-9]/g, "")
    .slice(0, 3);
  if (code.length < 2) {
    res.status(400).end();
    return;
  }
  try {
    const r = await fetch("https://pics.avs.io/64/64/" + code + ".png", {
      headers: { "user-agent": "pico-radar/1.0", accept: "image/png,image/*" },
      signal: AbortSignal.timeout(6000),
    });
    if (!r.ok) {
      res.status(404).end();
      return;
    }
    const buf = Buffer.from(await r.arrayBuffer());
    res.setHeader("Content-Type", r.headers.get("content-type") || "image/png");
    res.setHeader("Cache-Control", "public, max-age=86400, s-maxage=86400");
    res.status(200).send(buf);
  } catch (e) {
    res.status(502).end();
  }
}
