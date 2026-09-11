// Trafico real: WiFi, el pedido al proxy y el lote de aviones que deja para
// que lo levante el radar. Ver sky.c.
#ifndef SKY_H
#define SKY_H
#include <stdint.h>
#include <stdbool.h>
#include "sky_parse.h"

// En que anda la radio. Se muestra en pantalla mientras no haya datos.
typedef enum {
    SKY_APAGADO,      // no hay red configurada: se ve el trafico simulado
    SKY_CONECTANDO,
    SKY_SIN_RED,      // no encontro el wifi o lo perdio
    SKY_PIDIENDO,
    SKY_ANDANDO,
    SKY_PROXY_CAIDO,
    SKY_PORTAL,       // haciendo de router, con el QR en pantalla
} sky_estado_t;

// Arranca la radio y el nucleo 1. Se llama una sola vez, desde el nucleo 0,
// antes de encender el video: conectarse tarda segundos y hace ruido.
// pedir_portal viene del gesto de los tres cortes: manda al portal sin
// siquiera intentar conectarse a la red guardada.
void sky_init(bool pedir_portal);

// Lo que el nucleo 0 necesita para dibujar la pantalla del QR.
bool        sky_en_portal(void);
const char *sky_ap_nombre(void);
const char *sky_ap_clave(void);
const char *sky_portal_url(void);

// El nucleo 1 lo prende cuando llegan pantallas nuevas desde la web.
extern volatile bool pantallas_rehacer_pedido;

// Que aeropuerto mirar. El nucleo 1 lo va a usar en el proximo pedido; se
// puede llamar cada vez que cambia la pantalla, es barato.
void sky_mirar(int32_t lat, int32_t lon, int radio_km);

// Si hay un lote nuevo sin leer, lo copia y devuelve cuantos aviones trajo.
// Devuelve -1 si no hay nada nuevo. Lo llama el nucleo 0, una vez por cuadro.
int sky_tomar(sky_avion_t *destino, int tope);

sky_estado_t sky_estado(void);
const char  *sky_estado_texto(void);

// Hora local en minutos desde medianoche, o -1 si todavia no se sabe. Sale
// del reloj del proxy, que es la unica hora que tiene el equipo.
int sky_hora_local(void);

// Segundos desde que llego el ultimo lote. Sirve para avisar en pantalla que
// lo que se ve quedo viejo.
uint32_t sky_segundos_desde_el_ultimo(void);

#endif
