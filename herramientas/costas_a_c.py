#!/usr/bin/env python3
"""Convierte docs/land.json y docs/borders.json a tablas en C.

Son las costas del mundo simplificadas: 53 contornos y unos 2700 puntos, que
en el firmware ocupan 11 KB. Se usan en la vista de seguimiento, para que el
vuelo se vea sobre el mapa y no sobre un fondo vacio, igual que en la web.

Las coordenadas van en grados por 100, que con esta resolucion alcanza y
entra en un entero de 16 bits.

Las fronteras salen de Natural Earth (dominio publico), simplificadas con
Douglas-Peucker a 0,35 grados: 333 tramos y 1374 puntos, unos 5 KB.

Uso:  python3 herramientas/costas_a_c.py
Sale: firmware-c/costas.c y firmware-c/costas.h
"""
import json, os

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = os.path.join(RAIZ, "firmware-c")


def main():
    anillos = [r for r in json.load(open(os.path.join(RAIZ, "docs", "land.json")))
               if r and len(r) >= 4]
    fronteras = [l for l in json.load(open(os.path.join(RAIZ, "docs", "borders.json")))
                 if l and len(l) >= 2]
    fpuntos, findice = [], []
    for l in fronteras:
        findice.append((len(fpuntos), len(l)))
        for lat, lon in l:
            fpuntos.append((round(lat * 100), round(lon * 100)))
    puntos, indice = [], []
    for r in anillos:
        indice.append((len(puntos), len(r)))
        for lat, lon in r:
            puntos.append((round(lat * 100), round(lon * 100)))

    with open(os.path.join(SALIDA, "costas.h"), "w") as h:
        h.write(f"""// Costas del mundo, generado por herramientas/costas_a_c.py. NO EDITAR.
// {len(indice)} contornos, {len(puntos)} puntos, en grados x 100.
#ifndef COSTAS_H
#define COSTAS_H
#include <stdint.h>

#define COSTAS_ANILLOS {len(indice)}
#define COSTAS_PUNTOS {len(puntos)}

typedef struct {{ uint16_t desde, cantidad; }} anillo_t;

extern const anillo_t costas_anillos[COSTAS_ANILLOS];
extern const int16_t costas_lat[COSTAS_PUNTOS];
extern const int16_t costas_lon[COSTAS_PUNTOS];

// Limites entre paises, como tramos sueltos (no cerrados).
#define FRONTERAS_TRAMOS {len(findice)}
#define FRONTERAS_PUNTOS {len(fpuntos)}
extern const anillo_t fronteras_tramos[FRONTERAS_TRAMOS];
extern const int16_t fronteras_lat[FRONTERAS_PUNTOS];
extern const int16_t fronteras_lon[FRONTERAS_PUNTOS];

#endif
""")

    with open(os.path.join(SALIDA, "costas.c"), "w") as c:
        c.write('// Generado por herramientas/costas_a_c.py. NO EDITAR.\n#include "costas.h"\n\n')
        c.write("const anillo_t costas_anillos[COSTAS_ANILLOS] = {\n")
        for desde, cant in indice:
            c.write("    {%d,%d},\n" % (desde, cant))
        c.write("};\n\nconst int16_t costas_lat[COSTAS_PUNTOS] = {\n")
        for i in range(0, len(puntos), 20):
            c.write("    " + ",".join(str(p[0]) for p in puntos[i:i + 20]) + ",\n")
        c.write("};\n\nconst int16_t costas_lon[COSTAS_PUNTOS] = {\n")
        for i in range(0, len(puntos), 20):
            c.write("    " + ",".join(str(p[1]) for p in puntos[i:i + 20]) + ",\n")
        c.write("};\n\nconst anillo_t fronteras_tramos[FRONTERAS_TRAMOS] = {\n")
        for desde, cant in findice:
            c.write("    {%d,%d},\n" % (desde, cant))
        c.write("};\n\nconst int16_t fronteras_lat[FRONTERAS_PUNTOS] = {\n")
        for i in range(0, len(fpuntos), 20):
            c.write("    " + ",".join(str(p[0]) for p in fpuntos[i:i + 20]) + ",\n")
        c.write("};\n\nconst int16_t fronteras_lon[FRONTERAS_PUNTOS] = {\n")
        for i in range(0, len(fpuntos), 20):
            c.write("    " + ",".join(str(p[1]) for p in fpuntos[i:i + 20]) + ",\n")
        c.write("};\n")
    print(f"{len(indice)} contornos, {len(puntos)} puntos; "
          f"{len(findice)} tramos de frontera, {len(fpuntos)} puntos")


if __name__ == "__main__":
    main()
