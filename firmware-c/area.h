// Area util: la parte del framebuffer que el monitor realmente muestra.
//
// Cada monitor recorta distinto. El ViewSonic VA1703wb de prueba se come el
// borde de arriba y el de la derecha. Como el equipo se vende armado, esto lo
// define Valentino al instalar, igual que "margen" en install.json de la
// version MicroPython, y el cliente no lo ve.
//
// Todo el radar se dibuja relativo a esta area, nunca a 0,0.
#ifndef AREA_H
#define AREA_H
#include "vga.h"

typedef struct {
    int x, y;        // primera columna y fila visibles
    int an, al;      // ancho y alto utiles
} area_t;

extern area_t area;

// Margenes en pixeles, lo que hay que descontar de cada borde.
void area_set(int arriba, int abajo, int izquierda, int derecha);

// Centro del area, que es donde va el radar.
static inline int area_cx(void) { return area.x + area.an / 2; }
static inline int area_cy(void) { return area.y + area.al / 2; }

// Patron de calibracion: reglas numeradas contra los cuatro bordes para leer
// cuanto recorta el monitor y cargar los margenes.
void area_calibrar(uint8_t fondo, uint8_t trazo, uint8_t acento);

#endif
