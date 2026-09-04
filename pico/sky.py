# Trafico ADS-B. Pega al proxy propio, que ya devuelve el JSON masticado y
# ahorra el TLS pesado y los campos que no usamos.

import json
import urequests

PROXY = "https://pico-vga-radar-sky.vercel.app/api/sky"

_KEEP = ("hex", "flight", "r", "t", "lat", "lon", "gs", "track", "dst")


def _alt_of(a):
    v = a.get("alt_baro")
    if v is None:
        v = a.get("alt")
    if v == "ground":
        return 0
    try:
        return int(v)
    except Exception:
        return None


def fetch(lat, lon, radius_km, limit=40):
    """Devuelve una lista de dicts chicos, o None si no se pudo."""
    nm = int(max(15, min(250, radius_km / 1.852)))
    url = "%s?lat=%.4f&lon=%.4f&dist=%d" % (PROXY, lat, lon, nm)
    r = None
    try:
        r = urequests.get(url, timeout=12)
        if r.status_code != 200:
            return None
        data = json.loads(r.text)
    except Exception:
        return None
    finally:
        if r is not None:
            try:
                r.close()
            except Exception:
                pass

    raw = data.get("ac") or data.get("aircraft") or []
    out = []
    for a in raw:
        if a.get("lat") is None:
            continue
        ac = {}
        for k in _KEEP:
            ac[k] = a.get(k)
        ac["alt"] = _alt_of(a)
        flight = (ac.get("flight") or "").strip()
        ac["flight"] = flight or (ac.get("r") or ac.get("hex") or "")
        out.append(ac)
        if len(out) >= limit:
            break
    return out
