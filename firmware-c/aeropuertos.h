// Aeropuertos, generado por herramientas/aeropuertos_a_c.py. NO EDITAR.
// 5334 aeropuertos con ciudad y coordenadas en grados x 10000.
#ifndef AEROPUERTOS_H
#define AEROPUERTOS_H
#include <stdint.h>

#define AEROPUERTOS_CANT 5334
#define AEROPUERTO_CIUDAD 22

typedef struct {
    char iata[4];
    char ciudad[AEROPUERTO_CIUDAD];
    int32_t lat, lon;
} aeropuerto_dato_t;

extern const aeropuerto_dato_t aeropuertos[AEROPUERTOS_CANT];

// Busca por codigo IATA. NULL si no esta.
const aeropuerto_dato_t *aeropuerto_buscar(const char *iata);

#endif
