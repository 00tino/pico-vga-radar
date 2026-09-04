# Dos archivos de configuracion separados a proposito:
#
#   install.json  lo dejas vos antes de vender el equipo (monitor, pulgadas).
#                 El cliente no lo ve ni lo puede tocar.
#   config.json   lo escribe el cliente desde el celular. Es lo unico que
#                 viaja por la red, y pesa unos 300 bytes.

import json

INSTALL = "install.json"
CONFIG = "config.json"

INSTALL_DEF = {
    "inches": 15,
    "w": 800,
    "h": 600,
}

CONFIG_DEF = {
    "wifi": {"ssid": "", "pass": ""},
    "apt": "EZE",
    "lat": -34.822,
    "lon": -58.536,
    "radius_km": 80,
    "view": "hybrid",
    "list": "fa",
    "theme": "crt_amber",
    "runways": True,
    "rotate_s": 7,
    "max_n": 8,
    "follow_al": "",
    "follow_flt": "",
    "want": {"air": True, "ga": False, "biz": False, "heli": False},
    "ground": True,
    "poll_s": 25,
}


def _read(path, default):
    try:
        with open(path) as f:
            data = json.load(f)
    except Exception:
        return dict(default)
    out = dict(default)
    for k, v in data.items():
        out[k] = v
    return out


def _write(path, data):
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(data, f)
    try:
        import os
        os.remove(path)
    except Exception:
        pass
    import os
    os.rename(tmp, path)


def load_install():
    return _read(INSTALL, INSTALL_DEF)


def save_install(data):
    _write(INSTALL, data)


def load_config():
    return _read(CONFIG, CONFIG_DEF)


def save_config(data):
    _write(CONFIG, data)


_BOOL_TRUE = ("1", "true", "on", "yes", "si")


def apply_query(cfg, q):
    """Vuelca un querystring de la web sobre el config. Solo claves conocidas."""
    def s(key, dest=None):
        if key in q:
            cfg[dest or key] = q[key]

    def f(key, dest=None, lo=None, hi=None):
        if key not in q:
            return
        try:
            v = float(q[key])
        except Exception:
            return
        if lo is not None and v < lo:
            return
        if hi is not None and v > hi:
            return
        cfg[dest or key] = v

    def b(key, dest=None):
        if key in q:
            cfg[dest or key] = str(q[key]).lower() in _BOOL_TRUE

    s("apt")
    s("view")
    s("list")
    s("theme")
    s("al", "follow_al")
    s("flt", "follow_flt")
    f("lat", lo=-90, hi=90)
    f("lon", lo=-180, hi=180)
    f("r", "radius_km", 20, 400)
    f("rot", "rotate_s", 3, 120)
    f("n", "max_n", 1, 24)
    f("poll", "poll_s", 10, 300)
    b("rwy", "runways")
    b("gnd", "ground")
    want = dict(cfg.get("want") or CONFIG_DEF["want"])
    for k in ("air", "ga", "biz", "heli"):
        if k in q:
            want[k] = str(q[k]).lower() in _BOOL_TRUE
    cfg["want"] = want
    for key in ("radius_km", "rotate_s", "max_n", "poll_s"):
        cfg[key] = int(cfg[key])
    return cfg
