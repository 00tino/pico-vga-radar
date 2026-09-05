// Las pantallas que el cliente arma y entre las que el equipo va rotando.
// Ver pantallas.c.
#ifndef PANTALLAS_H
#define PANTALLAS_H
#include "radar.h"

#define PANTALLAS_MAX 8

typedef struct {
    char     nombre[24];   // solo para la consola y la web
    vista_t  vista;
    lista_t  lista;
    char     apt[4];       // aeropuerto, en IATA
    int      radio_km;
    int      tarjetas;     // cuantos vuelos por vez
    char     tema[16];
    char     seguir[9];    // indicativo, para las vistas de seguimiento
    int      segundos;     // cuanto se queda en esta pantalla
} pantalla_t;

extern pantalla_t pantallas[PANTALLAS_MAX];
extern int        pantallas_n;

void pantallas_init(void);      // deja puesta la primera
void pantallas_avanzar(void);   // una vez por cuadro
int  pantallas_actual(void);
void pantallas_forzar_siguiente(void);

#endif
