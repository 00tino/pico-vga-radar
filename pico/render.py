# Pintado del radar y de la lista. Es la primera version: puntos, anillos,
# barrido y renglones de texto. Las tarjetas con logo de la web todavia no.

import math
import time
import ui

_beam = -math.pi / 2


def _kind_air(ac):
    t = (ac.get("t") or "").upper()
    if not t:
        return True
    return t[:2] in ("A3", "A2", "A1", "B7", "B3", "E1", "E2", "AT", "DH",
                     "BC", "MD", "CR", "SU", "RJ")


def filtered(sky, cfg):
    want = cfg.get("want") or {}
    out = []
    for ac in sky:
        if not want.get("air", True) and _kind_air(ac):
            continue
        if not cfg.get("ground", True) and (ac.get("alt") or 0) == 0:
            continue
        al = (cfg.get("follow_al") or "").upper()
        if al and not (ac.get("flight") or "").upper().startswith(al):
            continue
        flt = (cfg.get("follow_flt") or "").upper().replace(" ", "")
        if flt and flt not in (ac.get("flight") or "").upper().replace(" ", ""):
            continue
        out.append(ac)
    return out


def _fmt_alt(alt):
    if alt is None:
        return "---"
    if alt <= 0:
        return "GND"
    if alt >= 10000:
        return "FL%03d" % (alt // 100)
    return "%d" % alt


def draw_radar(cfg, sky, x0, y0, w, h, col):
    global _beam
    s = ui.screen()
    cx = x0 + w // 2
    cy = y0 + h // 2
    R = int(min(w, h) * 0.43)

    for f in (0.25, 0.5, 0.75, 1.0):
        s.draw_circle(cx, cy, int(R * f), col)
    s.draw_line(cx, cy - R, cx, cy + R, col)
    s.draw_line(cx - R, cy, cx + R, cy, col)

    # Sin barrido giratorio: dibujar un cuadro completo en MicroPython tarda
    # demasiado como para animarlo. Vuelve cuando el render pase a C.

    span = max(0.25, cfg["radius_km"] / 111.0)
    lat0 = cfg["lat"]
    lon0 = cfg["lon"]
    for ac in sky:
        lat = ac.get("lat")
        lon = ac.get("lon")
        if lat is None or lon is None:
            continue
        px = int(cx + ((lon - lon0) / span) * R)
        py = int(cy - ((lat - lat0) / span) * R)
        if not (x0 <= px < x0 + w and y0 <= py < y0 + h):
            continue
        s.fill_disk(px, py, 3, col)

    s.settextcolor(col)
    s.settextcursor(x0 + 6, y0 + 14)
    s.printh(cfg.get("apt", "---"))


def draw_list(cfg, sky, x0, y0, w, h, col, page=0):
    s = ui.screen()
    s.settextcolor(col)
    line_h = 22
    rows = max(1, (h - 30) // line_h)
    n = cfg.get("max_n", 8)
    view = sky[:n]
    if not view:
        s.settextcursor(x0 + 8, y0 + 34)
        s.printh("SIN VUELOS EN ESTE RADIO")
        return
    start = (page * rows) % len(view)

    s.settextcursor(x0 + 8, y0 + 16)
    s.printh("VUELO      TIPO   ALT     VEL")
    s.draw_fastHline(x0 + 8, x0 + w - 8, y0 + 22, col)

    y = y0 + 42
    for i in range(min(rows, len(view))):
        ac = view[(start + i) % len(view)]
        # MicroPython no tiene ljust/rjust: el ancho se pone en el formato.
        flight = (ac.get("flight") or "")[:9]
        typ = (ac.get("t") or "--")[:5]
        alt = _fmt_alt(ac.get("alt"))
        gs = ac.get("gs")
        vel = ("%d kt" % gs) if gs else "--"
        s.settextcursor(x0 + 8, y)
        s.printh("%-9s  %-5s %6s  %7s" % (flight, typ, alt, vel))
        y += line_h


def draw(cfg, sky, page=0):
    col = ui.THEMES.get(cfg.get("theme"), ui.YELLOW)
    ui.clear()
    view = cfg.get("view", "hybrid")
    # Todo se dibuja dentro del area que el monitor realmente muestra.
    x0, y0, w, h = ui.X0, ui.Y0, ui.AW, ui.AH
    if view == "radar":
        draw_radar(cfg, sky, x0, y0, w, h, col)
    elif view == "wall":
        draw_list(cfg, sky, x0, y0, w, h, col, page)
    else:
        split = x0 + int(w * 0.54)
        draw_radar(cfg, sky, x0, y0, split - x0, h, col)
        ui.screen().draw_fastVline(split, y0, y0 + h - 1, col)
        draw_list(cfg, sky, split + 1, y0, x0 + w - split - 1, h, col, page)
