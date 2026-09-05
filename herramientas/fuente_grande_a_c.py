#!/usr/bin/env python3
"""Genera las fuentes grandes del firmware, rasterizadas de verdad.

Agrandar la fuente chica repitiendo pixeles deja las letras escalonadas: a 7x14
cada pixel es un rasgo entero de la letra, y multiplicarlo por dos o por tres
solo agranda los escalones. Asi que los tamanos grandes se rasterizan de nuevo
desde una fuente vectorial, cada uno al alto que le toca.

Se alinean contra la chica: para cada caracter se mide donde arranca y termina
la 'A' mayuscula, y se elige el tamano en puntos y el desplazamiento para que
la version grande caiga en el mismo lugar multiplicado por la escala. Asi
ningun texto del firmware se corre de lugar, y el avance de cada caracter es
exactamente el de la fuente chica por la escala.

Salida: firmware-c/fuente_grande.c
"""
import sys, re
sys.path.insert(0, "/Users/valentino/Library/Python/3.9/lib/python/site-packages")
from PIL import Image, ImageFont, ImageDraw

FUENTE = "/Users/valentino/Library/Fonts/Inconsolata.ttf"
ESCALAS = [2, 3]
ALTO_CHICO, ANCHO_CHICO = 14, 7

def leer_fuente_chica():
    """Saca del .c los avances y los bitmaps de la fuente de 7x14."""
    txt = open("firmware-c/fuente.c").read()
    cuerpo = txt[txt.index("{", txt.index("gfx_fuente")):]
    nums = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", cuerpo)]
    chars = []
    for i in range(96):
        b = nums[i * 15:(i + 1) * 15]
        avance = b[0]
        cols = [b[1 + c * 2] | (b[2 + c * 2] << 8) for c in range(ANCHO_CHICO)]
        chars.append((avance, cols))
    return chars

def caja_A(cols_por_char):
    """Filas que ocupa la 'A' mayuscula en la fuente chica."""
    _, cols = cols_por_char[ord('A') - 32]
    filas = [f for f in range(ALTO_CHICO)
             if any(c & (1 << f) for c in cols)]
    return min(filas), max(filas)

def elegir_tamano(escala, arriba_chico, abajo_chico):
    """Tamano en puntos y desplazamiento para que la 'A' caiga donde debe."""
    alto_objetivo = (abajo_chico - arriba_chico + 1) * escala
    y_objetivo = arriba_chico * escala
    mejor = None
    for pts in range(escala * 8, escala * 26):
        f = ImageFont.truetype(FUENTE, pts)
        im = Image.new("L", (pts * 2, pts * 3), 0)
        ImageDraw.Draw(im).text((pts // 2, pts // 2), "A", fill=255, font=f)
        bb = im.point(lambda v: 255 if v >= 128 else 0).getbbox()
        if not bb:
            continue
        alto = bb[3] - bb[1]
        if mejor is None or abs(alto - alto_objetivo) < abs(mejor[1] - alto_objetivo):
            mejor = (pts, alto, bb[1] - pts // 2)
    pts, _, top_rel = mejor
    return pts, y_objetivo - top_rel

def rasterizar(escala, chicos):
    ancho, alto = ANCHO_CHICO * escala, ALTO_CHICO * escala
    arriba, abajo = caja_A(chicos)
    pts, dy = elegir_tamano(escala, arriba, abajo)
    f = ImageFont.truetype(FUENTE, pts)
    bytes_fila = (ancho + 7) // 8
    salida = []
    for i in range(96):
        avance_chico, _ = chicos[i]
        ch = chr(32 + i)
        im = Image.new("L", (ancho, alto), 0)
        d = ImageDraw.Draw(im)
        # Centrado en el ancho de avance, igual que la chica: el glifo va
        # dentro de su caja y el avance manda el paso al siguiente.
        an_glifo = d.textlength(ch, font=f)
        x = (avance_chico * escala - an_glifo) / 2
        d.text((x, dy), ch, fill=255, font=f)
        px = im.point(lambda v: 255 if v >= 110 else 0).load()
        filas = []
        for y in range(alto):
            v = 0
            for xx in range(ancho):
                if px[xx, y]:
                    v |= 1 << xx
            filas.append(v)
        salida.append((avance_chico * escala, filas, bytes_fila))
    return ancho, alto, bytes_fila, salida, pts

chicos = leer_fuente_chica()
partes = []
cab = []
for escala in ESCALAS:
    ancho, alto, bf, datos, pts = rasterizar(escala, chicos)
    nombre = "gfx_fuente_x%d" % escala
    por_char = 1 + alto * bf
    cab.append((escala, nombre, ancho, alto, bf, por_char))
    lineas = []
    for i, (avance, filas, _) in enumerate(datos):
        vals = ["0x%02X" % avance]
        for v in filas:
            for k in range(bf):
                vals.append("0x%02X" % ((v >> (8 * k)) & 0xFF))
        ch = chr(32 + i)
        etiq = "'%s'" % (ch if ch != "'" and ch != "\\" else "\\" + ch)
        lineas.append("    " + ", ".join(vals) + ",  // " + etiq)
    partes.append("const uint8_t %s[96 * %d] = {\n%s\n};\n" % (nombre, por_char, "\n".join(lineas)))
    print("escala %d: %dx%d, %d bytes por caracter, %d pts" % (escala, ancho, alto, por_char, pts))

with open("firmware-c/fuente_grande.c", "w") as fh:
    fh.write("// Fuentes grandes, generadas por herramientas/fuente_grande_a_c.py. NO EDITAR.\n")
    fh.write("//\n// Rasterizadas de Inconsolata a cada tamano, no agrandadas de la de 7x14:\n")
    fh.write("// repetir pixeles solo agranda los escalones. Cada caracter lleva 1 byte de\n")
    fh.write("// avance y despues una fila por linea, bit 0 = columna de la izquierda.\n")
    fh.write('#include "gfx.h"\n\n')
    fh.write("\n".join(partes))
print("escrito firmware-c/fuente_grande.c")
