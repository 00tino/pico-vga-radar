#!/usr/bin/env python3
"""Convierte los logos de aerolineas al formato que dibuja la Pico.

Los PNG de docs/logos/ son 64x64 RGBA y pesan 4,2 MB entre los 824. Ese peso
es del formato, no de la informacion: la pantalla de la Pico no muestra ni
64x64 ni color verdadero. Aca se pasan al tamano y al formato reales de la
pantalla —36x36, un byte RRRGGGBB por pixel— y entran todos en la flash.

Los PNG vienen con las esquinas transparentes, porque en la web el logo va
dentro de una caja redondeada. Componerlos sobre blanco dejaba puntas blancas
en los logos de fondo de color, como el de Aerolineas. Por eso cada logo se
compone sobre SU PROPIO color de fondo, que se saca de los pixeles opacos del
borde: el de Aerolineas queda celeste entero y el de LATAM blanco entero.

El dither es el mismo Bayer 4x4 que usa firmware-c/gfx.c. Si se cambia alla,
hay que cambiarlo aca: los dos tienen que cuantizar igual.

Uso:  python3 herramientas/logos_a_c.py
Sale: firmware-c/logos.c y firmware-c/logos.h
"""
import json, os, sys
sys.path.insert(0, "/Users/valentino/Library/Python/3.9/lib/python/site-packages")
from PIL import Image

LADO = 36
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOGOS = os.path.join(RAIZ, "docs", "logos")
SALIDA = os.path.join(RAIZ, "firmware-c")
PAPEL = (0xfb, 0xfa, 0xf7)

BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5]


def canal(v, maximo, umbral):
    t = v * maximo * 16 // 255
    n = t // 16 + (1 if (t % 16) > umbral else 0)
    return min(n, maximo)


def a_byte(x, y, r, g, b):
    """Mismo resultado que gfx_rgb_dither(): el azul no se dithera."""
    u = BAYER[(y & 3) * 4 + (x & 3)]
    return (canal(r, 7, u) << 5) | (canal(g, 7, u) << 2) | ((b * 3 + 127) // 255)


def color_de_fondo(im):
    """El color mas repetido entre los pixeles opacos del borde."""
    px = im.load()
    w, h = im.size
    cuenta = {}
    for x in range(w):
        for y in (0, 1, h - 2, h - 1):
            r, g, b, a = px[x, y]
            if a > 200:
                cuenta[(r, g, b)] = cuenta.get((r, g, b), 0) + 1
    for y in range(h):
        for x in (0, 1, w - 2, w - 1):
            r, g, b, a = px[x, y]
            if a > 200:
                cuenta[(r, g, b)] = cuenta.get((r, g, b), 0) + 1
    if not cuenta:
        return PAPEL
    color, n = max(cuenta.items(), key=lambda kv: kv[1])
    # Si el borde es un revoltijo, no hay fondo: va el papel blanco.
    return color if n >= (w + h) // 2 else PAPEL


def convertir(ruta):
    orig = Image.open(ruta).convert("RGBA")
    fondo = color_de_fondo(orig)
    im = orig.resize((LADO, LADO), Image.LANCZOS)
    base = Image.new("RGBA", im.size, fondo + (255,))
    im = Image.alpha_composite(base, im).convert("RGB")
    px = im.load()
    return bytes(a_byte(x, y, *px[x, y]) for y in range(LADO) for x in range(LADO))


def main():
    nombres = {}
    for _icao, iata, nombre, _pais, _x in json.load(
            open(os.path.join(RAIZ, "docs", "airlines.json"))):
        if iata:
            nombres.setdefault(iata, nombre)

    archivos = sorted(f for f in os.listdir(LOGOS) if f.endswith(".png"))
    datos, indice = bytearray(), []
    for f in archivos:
        codigo = os.path.splitext(f)[0]
        indice.append((codigo, len(datos), nombres.get(codigo, "")))
        datos += convertir(os.path.join(LOGOS, f))

    with open(os.path.join(SALIDA, "logos.h"), "w") as h:
        h.write(f"""// Logos de aerolineas, generado por herramientas/logos_a_c.py. NO EDITAR.
// {len(indice)} logos de {LADO}x{LADO}, un byte RRRGGGBB por pixel.
#ifndef LOGOS_H
#define LOGOS_H
#include <stdint.h>

#define LOGO_LADO {LADO}
#define LOGO_BYTES ({LADO} * {LADO})
#define LOGOS_CANT {len(indice)}

typedef struct {{
    char codigo[3];       // IATA de dos letras
    uint32_t offset;      // donde empiezan sus pixeles
}} logo_t;

extern const logo_t logos_indice[LOGOS_CANT];
extern const uint8_t logos_datos[];

// Devuelve los pixeles del logo, o NULL si esa aerolinea no tiene.
const uint8_t *logo_buscar(const char *codigo_iata);

#endif
""")

    with open(os.path.join(SALIDA, "logos.c"), "w") as c:
        c.write('// Generado por herramientas/logos_a_c.py. NO EDITAR.\n')
        c.write('#include "logos.h"\n#include <string.h>\n\n')
        c.write("const logo_t logos_indice[LOGOS_CANT] = {\n")
        for codigo, off, nombre in indice:
            com = ("  // " + nombre) if nombre else ""
            c.write('    {"%s", %d},%s\n' % (codigo, off, com))
        c.write("};\n\nconst uint8_t logos_datos[] = {\n")
        for i in range(0, len(datos), 32):
            c.write("    " + ",".join(str(b) for b in datos[i:i + 32]) + ",\n")
        c.write("};\n\n")
        c.write("""// El indice esta ordenado por codigo, asi que busqueda binaria.
const uint8_t *logo_buscar(const char *codigo_iata) {
    int lo = 0, hi = LOGOS_CANT - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        int cmp = strncmp(codigo_iata, logos_indice[m].codigo, 2);
        if (cmp == 0) return &logos_datos[logos_indice[m].offset];
        if (cmp < 0) hi = m - 1; else lo = m + 1;
    }
    return 0;
}
""")

    con_nombre = sum(1 for _, _, n in indice if n)
    print(f"{len(indice)} logos, {len(datos)} bytes de pixeles "
          f"({len(datos)/1024/1024:.2f} MB), {con_nombre} con nombre conocido")


if __name__ == "__main__":
    main()
