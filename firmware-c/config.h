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

// Mostrar solo vuelos de aerolinea. Arriba de un aeropuerto grande la mitad
// de lo que vuela son avionetas y aviones de instruccion: no tienen ruta ni
// horario y sus tarjetas salen vacias. Lo aplica el proxy, no la placa, para
// que los 32 lugares se llenen con vuelos que sirvan.
extern bool config_solo_aerolineas;

// La casa del cliente: una ubicacion propia, aparte del aeropuerto. Cuando
// pasa un avion cerca se lo marca en el scope y sale su ficha abajo. En
// grados por 10000, igual que todo lo demas.
// Minutos de diferencia con UTC para los horarios de las fichas. Cero es
// hora Zulu, que es como la mira un piloto. Lo manda la pagina, que sabe en
// que huso esta el telefono, y hasta entonces vale lo de instalacion.h.
extern int config_tz_min;

extern bool    config_casa_on;
extern int32_t config_casa_lat, config_casa_lon;
extern int     config_casa_km;

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
