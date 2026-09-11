// El avion tal como lo manda el proxy, y el parser que lo saca del texto.
// Esta aparte de sky.h para poder probarlo fuera de la placa; ver sky_parse.c.
#ifndef SKY_PARSE_H
#define SKY_PARSE_H
#include <stdint.h>

#define SKY_MAX 32       // lo mismo que RADAR_MAX_AVIONES

// Un avion como lo manda el proxy: sin rastro y sin nada calculado. Lo
// calculado (ciudades, distancia, avance, hora de llegada) lo hace vivo.c,
// porque es ahi donde estan los aeropuertos y los logos.
typedef struct {
    char    hex[8];
    char    vuelo[9];
    char    matricula[9];
    char    tipo[8];
    int32_t lat, lon;      // grados x 10000
    int32_t alt;           // pies
    int16_t gs;            // nudos
    int16_t track;         // grados, -1 si no lo manda
    char    origen[4], destino[4];
} sky_avion_t;

// Convierte la respuesta entera del proxy en aviones. Devuelve cuantos entro
// y, si se le pasa, deja en hora_min la hora local en minutos desde
// medianoche (-1 si el proxy no la mando).
int sky_parse(const char *texto, sky_avion_t *destino, int tope, int *hora_min);

#endif
