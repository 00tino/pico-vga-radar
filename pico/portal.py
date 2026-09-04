# Servidor HTTP minimo. Lo pesado (elegir aeropuerto, ver mapas, logos) vive en
# la web de GitHub Pages; el equipo solo recibe el resultado, que son ~300 bytes.

import socket

PAGE_URL = "https://00tino.github.io/pico-vga-radar/sim.html"

_WIFI_FORM = """<!doctype html><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Radar</title>
<style>
body{background:#0a0a0b;color:#eceae4;font:16px system-ui;margin:0;padding:28px}
h1{font-size:20px;margin:0 0 4px}p{color:#9a958b;margin:0 0 22px;font-size:14px}
label{display:block;font-size:12px;text-transform:uppercase;color:#6f6a62;margin:16px 0 6px}
input{width:100%;box-sizing:border-box;height:46px;border-radius:10px;border:1px solid #2a2926;
background:#000;color:#fff;padding:0 12px;font-size:16px}
button{width:100%;height:46px;margin-top:24px;border:0;border-radius:10px;
background:#f2542d;color:#fff;font-size:16px}
</style>
<h1>Conectar el radar</h1>
<p>Elegi tu red de casa. El equipo se reinicia y arranca solo.</p>
<form action="/wifi">
<label>Red WiFi (2.4 GHz)</label><input name="ssid" autocapitalize=off autocorrect=off required>
<label>Contrase&ntilde;a</label><input name="pass" type="password">
<button>Guardar y reiniciar</button>
</form>
"""


def _ok(body, ctype="text/html"):
    return ("HTTP/1.0 200 OK\r\nContent-Type: " + ctype +
            "; charset=utf-8\r\nConnection: close\r\n\r\n" + body)


def _msg(title, detail):
    return _ok("<!doctype html><meta charset=utf-8>"
               "<meta name=viewport content='width=device-width,initial-scale=1'>"
               "<body style='background:#0a0a0b;color:#eceae4;font:17px system-ui;"
               "padding:40px;text-align:center'>"
               "<h1 style='color:#f2542d'>" + title + "</h1><p>" + detail + "</p>")


def _unquote(s):
    s = s.replace("+", " ")
    parts = s.split("%")
    out = parts[0]
    for p in parts[1:]:
        try:
            out += chr(int(p[:2], 16)) + p[2:]
        except Exception:
            out += "%" + p
    return out


def parse_query(path):
    q = {}
    if "?" not in path:
        return path, q
    route, _, rest = path.partition("?")
    for pair in rest.split("&"):
        if not pair:
            continue
        k, _, v = pair.partition("=")
        q[_unquote(k)] = _unquote(v)
    return route, q


class Portal:
    """Se atiende de a un pedido por vuelta, sin bloquear el dibujado."""

    def __init__(self, ip, port=80):
        self.ip = ip
        self.sock = socket.socket()
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("0.0.0.0", port))
        self.sock.listen(2)
        self.sock.settimeout(0.2)

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass

    def poll(self, on_wifi, on_config, ap_mode):
        """Devuelve 'wifi' o 'config' cuando el cliente guardo algo."""
        try:
            cli, _ = self.sock.accept()
        except Exception:
            return None
        result = None
        try:
            cli.settimeout(3)
            req = cli.recv(1024)
            if not req:
                return None
            line = req.split(b"\r\n")[0].decode()
            parts = line.split(" ")
            if len(parts) < 2:
                return None
            route, q = parse_query(parts[1])

            if route == "/wifi" and q.get("ssid"):
                on_wifi(q.get("ssid", ""), q.get("pass", ""))
                cli.send(_msg("Guardado", "Reiniciando y conectando a " +
                              q.get("ssid", "") + "..."))
                result = "wifi"
            elif route == "/save":
                on_config(q)
                cli.send(_msg("Listo", "El monitor ya se esta actualizando."))
                result = "config"
            elif ap_mode:
                cli.send(_ok(_WIFI_FORM))
            else:
                # Con internet, el celular se va a la pagina completa.
                target = PAGE_URL + "?pico=" + self.ip
                cli.send(_ok("<!doctype html><meta charset=utf-8>"
                             "<meta http-equiv=refresh content='0;url=" + target + "'>"
                             "<a href='" + target + "'>Abrir configuracion</a>"))
        except Exception:
            pass
        finally:
            try:
                cli.close()
            except Exception:
                pass
        return result
