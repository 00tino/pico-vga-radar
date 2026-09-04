# Todo lo que se dibuja en el monitor. La libreria VGA da 8 colores.

from VGA.VGA_800x600 import screen_800x600
import qr

BLACK = 0b000
BLUE = 0b100
GREEN = 0b010
CYAN = 0b110
RED = 0b001
MAGENTA = 0b101
YELLOW = 0b011
WHITE = 0b111

W = 800
H = 600

# Area util: lo que el monitor realmente muestra. Los margenes salen de
# install.json, porque cada monitor recorta distinto. En el ViewSonic VA1703wb
# de prueba se pierden las ultimas ~20 filas.
def _margen():
    try:
        import json
        m = (json.load(open("install.json")).get("margen") or {})
    except Exception:
        m = {}
    return (int(m.get("arriba", 0)), int(m.get("abajo", 0)),
            int(m.get("izquierda", 0)), int(m.get("derecha", 0)))

_T, _B, _L, _R = _margen()
X0 = _L                 # primera columna visible
Y0 = _T                 # primera fila visible
X1 = W - 1 - _R         # ultima columna visible
Y1 = H - 1 - _B         # ultima fila visible
AW = X1 - X0 + 1        # ancho util
AH = Y1 - Y0 + 1        # alto util

THEMES = {
    "crt_amber": YELLOW,
    "crt_green": GREEN,
    "crt_white": WHITE,
    "crt_cyan": CYAN,
}

_scr = None


def screen():
    global _scr
    if _scr is None:
        _scr = screen_800x600()
        _scr.VGA_init()
        _scr.fill_screen(BLACK)
    return _scr


def clear(col=BLACK):
    s = screen()
    s.fill_screen(col)
    s.background_color = col


def text(x, y, msg, col=WHITE):
    s = screen()
    s.settextcursor(x, y)
    s.settextcolor(col)
    s.printh(msg)


def center(y, msg, col=WHITE, char_w=10):
    text(max(X0 + 2, X0 + (AW - len(msg) * char_w) // 2), y, msg, col)


def draw_qr(text_url, x, y, scale=8, col=WHITE):
    """Dibuja el QR con un marco blanco alrededor: sin marco no lo lee el celular."""
    s = screen()
    m = qr.encode(text_url)
    n = len(m)
    quiet = 4 * scale
    side = n * scale + 2 * quiet
    s.fill_rect(x, y, x + side, y + side, WHITE)
    for j in range(n):
        row = m[j]
        py = y + quiet + j * scale
        for i in range(n):
            if row[i]:
                px = x + quiet + i * scale
                s.fill_rect(px, py, px + scale - 1, py + scale - 1, BLACK)
    return side


def screen_boot(msg="INICIANDO"):
    clear()
    center(H // 2 - 20, msg, YELLOW)


def screen_ap(ap_name, ap_pass, url):
    """Sin WiFi: el cliente se cuelga de la red del equipo y carga la config."""
    clear()
    center(60, "CONFIGURACION INICIAL", YELLOW)
    side = draw_qr(url, (W - (29 * 8 + 64)) // 2, 110, 8)
    y = 110 + side + 30
    center(y, "1. En el celular, entra al WiFi:", WHITE)
    center(y + 26, ap_name + "   clave: " + ap_pass, YELLOW)
    center(y + 60, "2. Escanea el codigo o abre:", WHITE)
    center(y + 86, url, YELLOW)


def screen_setup(url, ip):
    """Con WiFi: el QR lleva a la pagina completa de configuracion."""
    clear()
    center(60, "LISTO PARA CONFIGURAR", YELLOW)
    side = draw_qr(url, (W - (29 * 8 + 64)) // 2, 110, 8)
    y = 110 + side + 30
    center(y, "Escanea con el celular para elegir", WHITE)
    center(y + 26, "aeropuerto, vista y filtros.", WHITE)
    center(y + 60, "Equipo en " + ip, YELLOW)


def screen_error(title, detail=""):
    clear()
    center(H // 2 - 40, title, RED)
    if detail:
        center(H // 2, detail, WHITE)
