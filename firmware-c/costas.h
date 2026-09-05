// Costas del mundo, generado por herramientas/costas_a_c.py. NO EDITAR.
// 53 contornos, 2744 puntos, en grados x 100.
#ifndef COSTAS_H
#define COSTAS_H
#include <stdint.h>

#define COSTAS_ANILLOS 53
#define COSTAS_PUNTOS 2744

typedef struct { uint16_t desde, cantidad; } anillo_t;

extern const anillo_t costas_anillos[COSTAS_ANILLOS];
extern const int16_t costas_lat[COSTAS_PUNTOS];
extern const int16_t costas_lon[COSTAS_PUNTOS];

#endif
