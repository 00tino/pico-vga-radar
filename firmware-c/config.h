// Las pantallas del cliente, guardadas en la flash. Ver config.c.
#ifndef CONFIG_H
#define CONFIG_H
#include "pantallas.h"
#include <stdbool.h>

// Llena pantallas[] y pantallas_n con lo que haya guardado. Devuelve false si
// no hay nada, si quedo a medias o si es de un formato viejo: en ese caso el
// que manda es quien la llamo, con sus valores de fabrica.
bool config_leer(void);

// La red del cliente, cargada desde el portal. Si nunca guardo ninguna, los
// dos quedan vacios y manda lo que diga instalacion.h.
// Cuantos aviones se dibujan como punto en el circulo. La lista de tarjetas
// no se toca: pasa por todos los vuelos que haya.
#define CONFIG_MAX_PUNTOS_DEF 16
extern int config_max_puntos;

extern char config_ssid[33];
extern char config_pass[64];

// Guarda la red y nada mas: es lo primero que hace el cliente, antes de tener
// ninguna pantalla armada.
bool config_guardar_wifi(const char *ssid, const char *pass);

// Si hay una red para intentar, sea del portal o de instalacion.h.
bool config_hay_wifi(void);

// Guarda lo que hay ahora en pantallas[]. Va a ser lo que llame el portal.
bool config_guardar(void);

// Deja la flash como salida de fabrica.
bool config_borrar(void);

#endif
