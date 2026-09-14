#!/usr/bin/env python3
"""Arma hojas con los 824 logos TAL COMO LOS DIBUJA EL EQUIPO.

No lee los PNG de docs/logos: lee firmware-c/logos.c, que es lo que
realmente va en la flash y se ve en el monitor. Es la unica forma de
enterarse de que un logo se rompio al convertirlo sin tener que mirar el
monitor logo por logo.

Asi se descubrio que el relleno de transparencia tapaba los logos chicos y
que el papel casi blanco moteaba el fondo de todos.

Uso:  python3 herramientas/ver_logos.py [carpeta de salida]
"""
import os
import re
import sys

sys.path.insert(0, os.path.expanduser(
    "~/Library/Python/3.9/lib/python/site-packages"))
from PIL import Image, ImageDraw

LADO = 36
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = sys.argv[1] if len(sys.argv) > 1 else "/tmp"

# Las que se ven por aca, para que salgan en la primera hoja.
PRIMERAS = ["AR", "LA", "JA", "H2", "OY", "AZ", "JJ", "CM", "AA", "DL", "UA",
            "AF", "KL", "IB", "BA", "LH", "EK", "QR", "TK", "AV", "G3", "AD",
            "WJ", "AM", "CX", "NZ", "QF", "ET", "SA", "TP", "AC", "AS", "B6"]


# El fondo del radar, para verlos como se ven en el monitor y no sobre blanco.
FONDO = (0x0a, 0x08, 0x05)
MASCARA_BYTES = (LADO * LADO + 7) // 8


def leer():
    s = open(os.path.join(RAIZ, "firmware-c", "logos.c")).read()
    idx = dict((m.group(1), int(m.group(2)))
               for m in re.finditer(r'\{"(\w\w)",\s*(\d+)\}', s))

    def arreglo(nombre):
        marca = "const uint8_t %s[] = {" % nombre
        ini = s.index(marca) + len(marca)
        return [int(x) for x in re.findall(r"\d+", s[ini:s.index("};", ini)])]

    return idx, arreglo("logos_datos"), arreglo("logos_mascaras")


def imagen(idx, datos, mascaras, cod):
    """El logo tal cual queda en el monitor: con su mascara y sobre el fondo
    del radar. Sobre blanco no se ve el problema que importa, que es el
    recuadro que algunos logos meten encima del radar."""
    off = idx[cod]
    px = datos[off:off + LADO * LADO]
    mo = (off // (LADO * LADO)) * MASCARA_BYTES
    im = Image.new("RGB", (LADO, LADO), FONDO)
    p = im.load()
    for y in range(LADO):
        for x in range(LADO):
            n = y * LADO + x
            if not (mascaras[mo + (n >> 3)] & (1 << (n & 7))):
                continue                      # fondo: se ve el radar
            b = px[n]
            # El mismo 3-3-2 que dibuja la placa.
            p[x, y] = ((b >> 5) * 255 // 7, ((b >> 2) & 7) * 255 // 7, (b & 3) * 255 // 3)
    return im


def main():
    idx, datos, mascaras = leer()
    orden = [c for c in PRIMERAS if c in idx]
    orden += sorted(c for c in idx if c not in orden)

    cols, esc = 20, 2
    paso = LADO * esc + 18
    por_hoja = 200
    hojas = (len(orden) + por_hoja - 1) // por_hoja
    for n in range(hojas):
        trozo = orden[n * por_hoja:(n + 1) * por_hoja]
        filas = (len(trozo) + cols - 1) // cols
        out = Image.new("RGB", (cols * paso, filas * paso + 24), FONDO)
        d = ImageDraw.Draw(out)
        d.text((6, 7), "LOGOS %d de %d, sobre el fondo del radar" % (n + 1, hojas),
               fill=(232, 184, 109))
        for i, c in enumerate(trozo):
            x, y = (i % cols) * paso, (i // cols) * paso + 24
            out.paste(imagen(idx, datos, mascaras, c)
                      .resize((LADO * esc, LADO * esc), Image.NEAREST), (x + 2, y + 2))
            d.text((x + 4, y + LADO * esc + 3), c, fill=(180, 150, 100))
        ruta = os.path.join(SALIDA, "logos_%d.png" % (n + 1))
        out.save(ruta)
        print(ruta)


if __name__ == "__main__":
    main()
