#!/usr/bin/env python3
"""Convierte docs/airports.json a una tabla en C.

Son 5334 aeropuertos con codigo, ciudad y coordenadas. Se usan para el
encabezado del radar, para las tarjetas y sobre todo para la vista de
seguimiento, que dibuja el origen, el destino y los aeropuertos por los que
pasa la ruta.

Uso:  python3 herramientas/aeropuertos_a_c.py
Sale: firmware-c/aeropuertos.c y firmware-c/aeropuertos.h
"""
import json, os

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = os.path.join(RAIZ, "firmware-c")
CIUDAD = 22


def limpiar(s):
    """Solo ASCII imprimible: la fuente del firmware no tiene acentos."""
    tabla = {"á": "a", "é": "e", "í": "i", "ó": "o", "ú": "u", "ñ": "n",
             "Á": "A", "É": "E", "Í": "I", "Ó": "O", "Ú": "U", "Ñ": "N",
             "ü": "u", "Ü": "U", "ç": "c", "Ç": "C"}
    out = "".join(tabla.get(c, c) for c in (s or ""))
    out = "".join(c if 32 <= ord(c) < 127 else "" for c in out)
    return out.replace('"', "").replace("\\", "")


def main():
    datos = json.load(open(os.path.join(RAIZ, "docs", "airports.json")))
    filas = []
    for fila in datos:
        iata = limpiar(fila[0])[:3]
        if len(iata) != 3:
            continue
        filas.append((iata, limpiar(fila[1])[:CIUDAD - 1],
                      round(float(fila[2]) * 10000), round(float(fila[3]) * 10000)))
    filas.sort(key=lambda f: f[0])

    with open(os.path.join(SALIDA, "aeropuertos.h"), "w") as h:
        h.write(f"""// Aeropuertos, generado por herramientas/aeropuertos_a_c.py. NO EDITAR.
// {len(filas)} aeropuertos con ciudad y coordenadas en grados x 10000.
#ifndef AEROPUERTOS_H
#define AEROPUERTOS_H
#include <stdint.h>

#define AEROPUERTOS_CANT {len(filas)}
#define AEROPUERTO_CIUDAD {CIUDAD}

typedef struct {{
    char iata[4];
    char ciudad[AEROPUERTO_CIUDAD];
    int32_t lat, lon;
}} aeropuerto_dato_t;

extern const aeropuerto_dato_t aeropuertos[AEROPUERTOS_CANT];

// Busca por codigo IATA. NULL si no esta.
const aeropuerto_dato_t *aeropuerto_buscar(const char *iata);

#endif
""")

    with open(os.path.join(SALIDA, "aeropuertos.c"), "w") as c:
        c.write('// Generado por herramientas/aeropuertos_a_c.py. NO EDITAR.\n')
        c.write('#include "aeropuertos.h"\n#include <string.h>\n\n')
        c.write("const aeropuerto_dato_t aeropuertos[AEROPUERTOS_CANT] = {\n")
        for iata, ciudad, lat, lon in filas:
            c.write('    {"%s","%s",%d,%d},\n' % (iata, ciudad, lat, lon))
        c.write("};\n\n")
        c.write("""const aeropuerto_dato_t *aeropuerto_buscar(const char *iata) {
    int lo = 0, hi = AEROPUERTOS_CANT - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        int cmp = strncmp(iata, aeropuertos[m].iata, 3);
        if (cmp == 0) return &aeropuertos[m];
        if (cmp < 0) hi = m - 1; else lo = m + 1;
    }
    return 0;
}
""")
    print(f"{len(filas)} aeropuertos")


if __name__ == "__main__":
    main()
