// Aerolineas, generado por herramientas/aerolineas_a_c.py. NO EDITAR.
// 797 codigos OACI con su equivalente IATA.
#ifndef AEROLINEAS_H
#define AEROLINEAS_H

#define AEROLINEAS_CANT 797

typedef struct {
    char icao[4];   // el que viene en el indicativo de ADS-B
    char iata[3];   // el que usan los logos
} aerolinea_t;

extern const aerolinea_t aerolineas[AEROLINEAS_CANT];

// Saca el IATA del principio de un indicativo ("ARG1885" -> "AR"). Devuelve
// NULL si esa aerolinea no esta en la tabla.
const char *aerolinea_de_indicativo(const char *indicativo);

#endif
