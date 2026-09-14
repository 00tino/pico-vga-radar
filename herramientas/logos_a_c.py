#!/usr/bin/env python3
"""Convierte los logos de aerolineas al formato que dibuja la Pico.

Los PNG de docs/logos/ son 64x64 RGBA y pesan 4,2 MB entre los 824. Ese peso
es del formato, no de la informacion: la pantalla de la Pico no muestra ni
64x64 ni color verdadero. Aca se pasan al tamano y al formato reales de la
pantalla —36x36, un byte RRRGGGBB por pixel— y entran todos en la flash.

Los PNG vienen con las esquinas transparentes, porque en la web el logo va
dentro de una caja redondeada. Antes de escalarlos se les extiende el color
hacia esas zonas, asi ninguno queda con puntas blancas. Ver
rellenar_transparente().

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
# Blanco puro y no un blanco casi blanco. Con 0xfb el dither lo partia entre
# dos niveles y el fondo de TODOS los logos salia moteado, con puntitos
# grises; 255 cae justo en un nivel y queda plano.
PAPEL = (255, 255, 255)

# Cuantos pixeles se empuja el color hacia las zonas transparentes. Dos
# alcanzan para matar el halo del borde; de ahi para arriba se empieza a
# tapar el logo. Ver rellenar_transparente().
EXPANDIR = 2

BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5]


def canal(v, maximo, umbral):
    t = v * maximo * 16 // 255
    n = t // 16 + (1 if (t % 16) > umbral else 0)
    return min(n, maximo)


def a_byte(x, y, r, g, b):
    """Mismo resultado que gfx_rgb_dither(): el azul no se dithera."""
    u = BAYER[(y & 3) * 4 + (x & 3)]
    return (canal(r, 7, u) << 5) | (canal(g, 7, u) << 2) | ((b * 3 + 127) // 255)


def rellenar_transparente(im):
    """Extiende el color de los pixeles opacos hacia los transparentes.

    Los PNG traen las esquinas transparentes porque en la web el logo va en
    una caja redondeada. Componerlos sobre un color fijo dejaba puntas: sobre
    blanco se veian en los logos de fondo oscuro, y elegir "el color del
    borde" no sirve cuando el fondo es un degradado, como el de British
    Airways o el de Emirates.

    Esto empuja el color hacia afuera UNOS POCOS pixeles, apenas los que hacen
    falta para que el borde no quede con un halo claro.

    Empujaba veinticuatro, y veinticuatro pixeles sobre un PNG de 64 llenan el
    cuadro entero: un logo chico rodeado de transparencia, como el de
    JetSmart, se derramaba hasta quedar en mitad azul y mitad rojo, sin la J
    ni la S. El de Sky terminaba con el fondo manchado de morado y verde por
    las flechas. Lo que sobra despues de estos pocos pixeles va sobre el papel
    blanco, que es la misma caja que se ve en la web.
    """
    px = im.load()
    w, h = im.size
    for _ in range(EXPANDIR):
        faltan = []
        for y in range(h):
            for x in range(w):
                if px[x, y][3] > 200:
                    continue
                r = g = b = n = 0
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1),
                               (1, 1), (1, -1), (-1, 1), (-1, -1)):
                    vx, vy = x + dx, y + dy
                    if 0 <= vx < w and 0 <= vy < h and px[vx, vy][3] > 200:
                        r += px[vx, vy][0]; g += px[vx, vy][1]; b += px[vx, vy][2]; n += 1
                if n:
                    faltan.append((x, y, (r // n, g // n, b // n, 255)))
        if not faltan:
            break
        for x, y, c in faltan:
            px[x, y] = c
    return im


def convertir(ruta):
    crudo = Image.open(ruta).convert("RGBA")
    orig = rellenar_transparente(crudo.copy())
    im = orig.resize((LADO, LADO), Image.LANCZOS)
    # Lo que siga transparente despues del relleno va sobre el papel blanco.
    base = Image.new("RGBA", im.size, PAPEL + (255,))
    im = Image.alpha_composite(base, im).convert("RGB")
    px = im.load()
    pixeles = bytes(a_byte(x, y, *px[x, y]) for y in range(LADO) for x in range(LADO))

    # La mascara dice que pixeles son del logo y cuales son el fondo de la
    # caja. Sale de la transparencia del PNG original, no del relleno: el
    # relleno es para que el borde no tenga halo, pero esas zonas siguen
    # siendo fondo.
    #
    # Sin esto el logo se dibuja como un cuadrado blanco sobre el radar, que
    # es de fondo oscuro, y queda un parche feo alrededor de cada logo.
    alfa = crudo.resize((LADO, LADO), Image.LANCZOS).split()[3].load()
    es_fondo = [[alfa[x, y] <= 128 for x in range(LADO)] for y in range(LADO)]

    # Se probo tambien sacar el fondo blanco de los logos que no vienen
    # recortados, rellenando desde los bordes. No quedo: con el umbral flojo
    # el relleno se cuela por los bordes suavizados y se come las letras (ITA
    # quedaba agujereada), y con el umbral estricto no alcanza a sacar el
    # marco de los que lo tienen (American, Iberia). Esos siguen con su caja
    # blanca, que es como se ven en la web.

    mascara = bytearray((LADO * LADO + 7) // 8)
    for y in range(LADO):
        for x in range(LADO):
            if not es_fondo[y][x]:
                i = y * LADO + x
                mascara[i >> 3] |= 1 << (i & 7)
    return pixeles, bytes(mascara)


def main():
    nombres = {}
    for _icao, iata, nombre, _pais, _x in json.load(
            open(os.path.join(RAIZ, "docs", "airlines.json"))):
        if iata:
            nombres.setdefault(iata, nombre)

    archivos = sorted(f for f in os.listdir(LOGOS) if f.endswith(".png"))
    datos, mascaras, indice = bytearray(), bytearray(), []
    for f in archivos:
        codigo = os.path.splitext(f)[0]
        indice.append((codigo, len(datos), nombres.get(codigo, "")))
        px, msk = convertir(os.path.join(LOGOS, f))
        datos += px
        mascaras += msk

    with open(os.path.join(SALIDA, "logos.h"), "w") as h:
        h.write(f"""// Logos de aerolineas, generado por herramientas/logos_a_c.py. NO EDITAR.
// {len(indice)} logos de {LADO}x{LADO}, un byte RRRGGGBB por pixel.
#ifndef LOGOS_H
#define LOGOS_H
#include <stdint.h>

#define LOGO_LADO {LADO}
#define LOGO_BYTES ({LADO} * {LADO})
#define LOGO_MASCARA_BYTES (({LADO} * {LADO} + 7) / 8)
#define LOGOS_CANT {len(indice)}

typedef struct {{
    char codigo[3];       // IATA de dos letras
    uint32_t offset;      // donde empiezan sus pixeles
}} logo_t;

extern const logo_t logos_indice[LOGOS_CANT];
extern const uint8_t logos_datos[];

// Un bit por pixel: 1 es logo, 0 es el fondo de la caja. Sin esto el logo se
// dibuja como un cuadrado blanco sobre el radar, que es oscuro, y queda un
// parche alrededor de cada uno.
extern const uint8_t logos_mascaras[];

// Devuelve los pixeles del logo, o NULL si esa aerolinea no tiene.
const uint8_t *logo_buscar(const char *codigo_iata);

// La mascara del mismo logo, para dibujarlo sin su fondo.
const uint8_t *logo_mascara(const char *codigo_iata);

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
        c.write("};\n\nconst uint8_t logos_mascaras[] = {\n")
        for i in range(0, len(mascaras), 32):
            c.write("    " + ",".join(str(b) for b in mascaras[i:i + 32]) + ",\n")
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

// La mascara vive en su propio arreglo, con el mismo orden que los pixeles:
// el logo numero n empieza en n * LOGO_MASCARA_BYTES.
const uint8_t *logo_mascara(const char *codigo_iata) {
    int lo = 0, hi = LOGOS_CANT - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        int cmp = strncmp(codigo_iata, logos_indice[m].codigo, 2);
        if (cmp == 0)
            return &logos_mascaras[(logos_indice[m].offset / LOGO_BYTES)
                                   * LOGO_MASCARA_BYTES];
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
