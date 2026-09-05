// Cuentas de geografia en enteros. Las coordenadas van en grados por 10000.
#ifndef GEO_H
#define GEO_H
#include <stdint.h>

// Distancia aproximada en kilometros. Alcanza para el alcance de un radar
// de unos cientos de km: no vale para vuelos largos.
int geo_km(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);

// Rumbo de 1 a 2, en grados, 0 = norte.
int geo_rumbo(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);

// Diferencia entre dos rumbos, siempre de 0 a 180.
int geo_dif_rumbo(int a, int b);

// Proyecta un punto a tantos km en un rumbo dado.
void geo_proyectar(int32_t lat, int32_t lon, int rumbo, int km,
                   int32_t *lat_out, int32_t *lon_out);

#endif
