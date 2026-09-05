#!/usr/bin/env python3
"""Pide el framebuffer a la Pico por el puerto serie y lo guarda como PNG.

Es el reemplazo de la camara cuando hace falta ver exactamente lo que dibuja
la Pico, sin el monitor ni la exposicion de la GoPro en el medio.

  python3 herramientas/mirar.py vista.png          # la escena que este
  python3 herramientas/mirar.py vista.png -n 3     # salta 3 escenas antes
"""
import sys, time, glob, base64, zlib, struct, argparse
sys.path.insert(0, "/Users/valentino/Library/Python/3.9/lib/python/site-packages")
import serial

def png(ancho, alto, rgb):
    crudo = b"".join(b"\x00" + rgb[y*ancho*3:(y+1)*ancho*3] for y in range(alto))
    def trozo(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c))
    return (b"\x89PNG\r\n\x1a\n"
            + trozo(b"IHDR", struct.pack(">IIBBBBB", ancho, alto, 8, 2, 0, 0, 0))
            + trozo(b"IDAT", zlib.compress(crudo, 6))
            + trozo(b"IEND", b""))

ap = argparse.ArgumentParser()
ap.add_argument("salida")
ap.add_argument("-n", "--saltar", type=int, default=0, help="escenas a saltar")
ap.add_argument("-e", "--escena", type=int, help="saltar hasta esta escena")
ap.add_argument("-p", "--quedarse", action="store_true", help="reiniciar el reloj de la escena")
a = ap.parse_args()

s = serial.Serial(glob.glob("/dev/cu.usbmodem*")[0], 115200, timeout=2)
for _ in range(a.saltar):
    s.write(b"n"); time.sleep(0.4)
if a.escena is not None:
    # Salta escenas hasta que la consola anuncie la pedida. Hay que vaciar el
    # buffer antes: la linea de la escena anterior sigue ahi y daba un falso
    # positivo, y se volcaba una vista que no era la pedida.
    s.reset_input_buffer()
    for _ in range(60):
        s.write(b"n")
        t = time.time()
        visto = None
        while time.time() - t < 1.5:
            l = s.readline().decode("ascii", "replace")
            if l.startswith("ESCENA "):
                visto = int(l.split()[1].rstrip(":")); break
        if visto == a.escena:
            break
    else:
        sys.exit("no aparecio la escena %d" % a.escena)
    s.write(b"p"); time.sleep(0.3)

s.reset_input_buffer()
s.write(b"v")

# Descarta lo que venga hasta la cabecera: la escena sigue imprimiendo.
ancho = alto = 0
t0 = time.time()
while time.time() - t0 < 10:
    l = s.readline().decode("ascii", "replace").strip()
    if l.startswith("FB "):
        _, ancho, alto = l.split(); ancho, alto = int(ancho), int(alto); break
if not ancho:
    sys.exit("la Pico no contesto el volcado")

partes = []
while True:
    l = s.readline().decode("ascii", "replace").strip()
    if l == "FBFIN": break
    if not l: sys.exit("se corto el volcado")
    partes.append(l)
s.close()

fb = base64.b64decode("".join(partes))
if len(fb) < ancho * alto:
    sys.exit("faltan bytes: %d de %d" % (len(fb), ancho * alto))

# Un pixel es RRRGGGBB. Se expande a 8 bits por canal igual que lo hace el
# conversor digital-analogico de resistencias del cable VGA.
tr = bytes(range(8))
rgb = bytearray(ancho * alto * 3)
for i in range(ancho * alto):
    v = fb[i]
    rgb[i*3]   = ((v >> 5) & 7) * 255 // 7
    rgb[i*3+1] = ((v >> 2) & 7) * 255 // 7
    rgb[i*3+2] = (v & 3) * 255 // 3
open(a.salida, "wb").write(png(ancho, alto, bytes(rgb)))
print("guardado", a.salida)
