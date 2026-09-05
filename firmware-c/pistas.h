// Pistas de aterrizaje, generado por herramientas/pistas_a_c.py. NO EDITAR.
// 5600 pistas de 4040 aeropuertos.
#ifndef PISTAS_H
#define PISTAS_H
#include <stdint.h>

typedef struct {
    char ident_a[4], ident_b[4];   // cabeceras, por ejemplo "11" y "29"
    int16_t hdg_a, hdg_b;          // rumbo de cada cabecera, en grados
    int32_t lat_a, lon_a;          // una punta, grados x 10000
    int32_t lat_b, lon_b;          // la otra punta
} pista_t;

typedef struct {
    char iata[4];
    uint16_t desde;                // indice de su primera pista
    uint8_t cantidad;
} apt_pistas_t;

#define PISTAS_CANT 5600
#define APTS_PISTAS_CANT 4040

extern const pista_t pistas[PISTAS_CANT];
extern const apt_pistas_t pistas_indice[APTS_PISTAS_CANT];

// Devuelve las pistas de un aeropuerto y cuantas son. NULL si no tiene.
const pista_t *pistas_de(const char *iata, int *cuantas);

#endif
