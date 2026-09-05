// Las pantallas que el cliente arma y entre las que el equipo va rotando.
// Ver pantallas.c.
#ifndef PANTALLAS_H
#define PANTALLAS_H
#include "radar.h"

#define PANTALLAS_MAX 8

// Cuanto puede durar una pantalla, en segundos. Los elige el equipo, no el
// cliente: cada cambio de pantalla limpia la imagen entera y eso se come un
// cuadro, o sea un parpadeo. Cada quince segundos no se nota; cada cinco, si.
// El maximo es para que el equipo no parezca colgado en una sola vista.
#define PANTALLA_SEGUNDOS_MIN 15
#define PANTALLA_SEGUNDOS_MAX 120

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
