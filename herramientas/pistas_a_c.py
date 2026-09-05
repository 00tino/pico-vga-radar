#!/usr/bin/env python3
"""Convierte docs/runways.json a tablas en C para el firmware.

Cada pista trae los dos identificadores de cabecera, los dos rumbos y las
coordenadas de las dos puntas. Son 5600 pistas de 4040 aeropuertos y ocupan
unos 190 KB, asi que entran todas: el cliente puede elegir cualquier
aeropuerto sin que le falten las pistas, igual que con los logos.

Las coordenadas van en grados por 10000, que es como las maneja el radar.

Uso:  python3 herramientas/pistas_a_c.py
Sale: firmware-c/pistas.c y firmware-c/pistas.h
"""
import json, os

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = os.path.join(RAIZ, "firmware-c")


def main():
    datos = json.load(open(os.path.join(RAIZ, "docs", "runways.json")))
    aptos = sorted(datos.keys())

    pistas, indice = [], []
    for iata in aptos:
        filas = datos[iata]
        indice.append((iata, len(pistas), len(filas)))
        for f in filas:
            ia, ib, ha, hb, la, lo, lb, ob = (f + [None] * 8)[:8]
            pistas.append((
                str(ia or "")[:3], str(ib or "")[:3],
                int(ha or 0), int(hb or 0),
                round(float(la) * 10000), round(float(lo) * 10000),
                round(float(lb) * 10000), round(float(ob) * 10000),
            ))

    with open(os.path.join(SALIDA, "pistas.h"), "w") as h:
        h.write(f"""// Pistas de aterrizaje, generado por herramientas/pistas_a_c.py. NO EDITAR.
// {len(pistas)} pistas de {len(indice)} aeropuertos.
#ifndef PISTAS_H
#define PISTAS_H
#include <stdint.h>

typedef struct {{
    char ident_a[4], ident_b[4];   // cabeceras, por ejemplo "11" y "29"
    int16_t hdg_a, hdg_b;          // rumbo de cada cabecera, en grados
    int32_t lat_a, lon_a;          // una punta, grados x 10000
    int32_t lat_b, lon_b;          // la otra punta
}} pista_t;

typedef struct {{
    char iata[4];
    uint16_t desde;                // indice de su primera pista
    uint8_t cantidad;
}} apt_pistas_t;

#define PISTAS_CANT {len(pistas)}
#define APTS_PISTAS_CANT {len(indice)}

extern const pista_t pistas[PISTAS_CANT];
extern const apt_pistas_t pistas_indice[APTS_PISTAS_CANT];

// Devuelve las pistas de un aeropuerto y cuantas son. NULL si no tiene.
const pista_t *pistas_de(const char *iata, int *cuantas);

#endif
""")

    with open(os.path.join(SALIDA, "pistas.c"), "w") as c:
        c.write('// Generado por herramientas/pistas_a_c.py. NO EDITAR.\n')
        c.write('#include "pistas.h"\n#include <string.h>\n\n')
        c.write("const pista_t pistas[PISTAS_CANT] = {\n")
        for ia, ib, ha, hb, la, lo, lb, ob in pistas:
            c.write('    {"%s","%s",%d,%d,%d,%d,%d,%d},\n' % (ia, ib, ha, hb, la, lo, lb, ob))
        c.write("};\n\nconst apt_pistas_t pistas_indice[APTS_PISTAS_CANT] = {\n")
        for iata, desde, cant in indice:
            c.write('    {"%s",%d,%d},\n' % (iata, desde, cant))
        c.write("};\n\n")
        c.write("""// El indice esta ordenado por codigo, asi que busqueda binaria.
const pista_t *pistas_de(const char *iata, int *cuantas) {
    int lo = 0, hi = APTS_PISTAS_CANT - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        int cmp = strncmp(iata, pistas_indice[m].iata, 3);
        if (cmp == 0) {
            if (cuantas) *cuantas = pistas_indice[m].cantidad;
            return &pistas[pistas_indice[m].desde];
        }
        if (cmp < 0) hi = m - 1; else lo = m + 1;
    }
    if (cuantas) *cuantas = 0;
    return 0;
}
""")
    print(f"{len(pistas)} pistas de {len(indice)} aeropuertos")


if __name__ == "__main__":
    main()
