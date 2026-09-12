#!/usr/bin/env python3
"""Pasa docs/airlines.json a una tabla en C.

Los indicativos que manda ADS-B vienen con el codigo OACI de tres letras
(ARG1885), pero los logos del firmware estan guardados por el codigo IATA de
dos (AR). Esta tabla es el puente entre los dos.

Se genera igual que aeropuertos.c: ordenada, para buscar por mitades.

    python3 herramientas/aerolineas_a_c.py
"""

import json
import pathlib

# docs/airlines.json viene de una base publica que quedo vieja: le faltan las
# aerolineas que nacieron despues y tiene codigos que cambiaron de dueno. Lo
# que se corrige aca gana sobre lo que diga el archivo.
#
# Para agregar una: mirar en la consola del equipo el indicativo que llega
# (son las tres letras del principio, ITY680 -> ITY) y poner al lado el codigo
# de dos letras cuyo logo esta en docs/logos.
AGREGADOS = {
    "ITY": "AZ",   # ITA Airways, que heredo el codigo de Alitalia
    "JES": "JA",   # JetSmart
    "JAT": "JA",   # JetSmart Chile
    "SKX": "H2",   # Sky Airline Peru; el archivo lo daba como Skyways Express
    "SKU": "H2",   # Sky Airline Chile
    "ANS": "OY",   # Andes Lineas Aereas; el logo ya estaba en docs/logos
}

RAIZ = pathlib.Path(__file__).resolve().parent.parent
ENTRADA = RAIZ / "docs" / "airlines.json"
SALIDA_C = RAIZ / "firmware-c" / "aerolineas.c"
SALIDA_H = RAIZ / "firmware-c" / "aerolineas.h"


def main():
    crudo = json.loads(ENTRADA.read_text())

    vistos = {}
    for fila in crudo:
        if not isinstance(fila, list) or len(fila) < 2:
            continue
        icao = (fila[0] or "").strip().upper()
        iata = (fila[1] or "").strip().upper()
        # Sin las dos cosas no sirve de nada: no se podria ni buscar ni
        # devolver un logo.
        if len(icao) != 3 or len(iata) != 2:
            continue
        if not icao.isalpha() or not iata.isalnum():
            continue
        vistos.setdefault(icao, iata)

    # Los agregados pisan lo que haya: son correcciones, no alternativas.
    vistos.update(AGREGADOS)

    filas = sorted(vistos.items())

    with SALIDA_H.open("w") as f:
        f.write(f"""// Aerolineas, generado por herramientas/aerolineas_a_c.py. NO EDITAR.
// {len(filas)} codigos OACI con su equivalente IATA.
#ifndef AEROLINEAS_H
#define AEROLINEAS_H

#define AEROLINEAS_CANT {len(filas)}

typedef struct {{
    char icao[4];   // el que viene en el indicativo de ADS-B
    char iata[3];   // el que usan los logos
}} aerolinea_t;

extern const aerolinea_t aerolineas[AEROLINEAS_CANT];

// Saca el IATA del principio de un indicativo ("ARG1885" -> "AR"). Devuelve
// NULL si esa aerolinea no esta en la tabla.
const char *aerolinea_de_indicativo(const char *indicativo);

#endif
""")

    with SALIDA_C.open("w") as f:
        f.write(f"""// Aerolineas, generado por herramientas/aerolineas_a_c.py. NO EDITAR.
#include "aerolineas.h"
#include <string.h>

const aerolinea_t aerolineas[AEROLINEAS_CANT] = {{
""")
        for icao, iata in filas:
            f.write('    {"%s", "%s"},\n' % (icao, iata))
        f.write("""};

const char *aerolinea_de_indicativo(const char *indicativo) {
    if (!indicativo) return 0;
    // Las tres primeras letras, si es que las hay. Una matricula (LVS100) va
    // a dar con algo o no, pero no hace dano: el logo simplemente no sale.
    char clave[4];
    for (int i = 0; i < 3; i++) {
        char c = indicativo[i];
        if (c < 'A' || c > 'Z') return 0;
        clave[i] = c;
    }
    clave[3] = 0;

    int lo = 0, hi = AEROLINEAS_CANT - 1;
    while (lo <= hi) {
        int medio = (lo + hi) / 2;
        int cmp = strcmp(clave, aerolineas[medio].icao);
        if (cmp == 0) return aerolineas[medio].iata;
        if (cmp < 0) hi = medio - 1;
        else lo = medio + 1;
    }
    return 0;
}
""")

    print(f"{len(filas)} aerolineas -> {SALIDA_C.name} y {SALIDA_H.name}")


if __name__ == "__main__":
    main()
