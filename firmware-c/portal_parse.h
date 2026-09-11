// Lectura de lo que manda la web: las direcciones con sus parametros y las
// pantallas. Esta aparte de portal.c para poder probarlo fuera de la placa,
// que es donde se rompe el contrato con docs/setup.html. Ver portal_parse.c.
#ifndef PORTAL_PARSE_H
#define PORTAL_PARSE_H
#include "pantallas.h"
#include <stdbool.h>

// Deshace el %20 y el + de una direccion, sobre el mismo texto.
void portal_destapar(char *s);

// Saca el valor de un parametro. false si no esta.
bool portal_parametro(const char *consulta, const char *cual, char *destino, int tam);
int  portal_numero(const char *consulta, const char *cual, int si_no_esta);

// Lee una pantalla de su forma "nombre~vista~lista~apt~km~tarjetas~tema~seguir~segundos"
// y la deja acotada a lo que el dibujo soporta. false si vino incompleta.
bool portal_leer_pantalla(const char *crudo, pantalla_t *p);

// Lee la tanda entera de una consulta de /save. Devuelve cuantas entraron.
int portal_leer_pantallas(const char *consulta, pantalla_t *destino, int tope);

#endif
