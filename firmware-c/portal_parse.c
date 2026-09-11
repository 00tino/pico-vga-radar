// Lectura de lo que manda la web. Ver portal_parse.h.
//
// Esta aparte de portal.c porque portal.c no se puede compilar fuera de la
// placa (usa lwIP y la radio) y esto si. El contrato con docs/setup.html se
// prueba en la Mac con herramientas/probar_portal.sh: si alguien cambia el
// formato de un lado, ahi salta.
#include "portal_parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- lectura del pedido ---------------------------------------------------
// Deshace el %20 y el + de las direcciones. Trabaja sobre el mismo texto.
void portal_destapar(char *s) {
    char *lee = s, *escribe = s;
    while (*lee) {
        if (*lee == '+') { *escribe++ = ' '; lee++; }
        else if (*lee == '%' && lee[1] && lee[2]) {
            char hex[3] = {lee[1], lee[2], 0};
            *escribe++ = (char)strtol(hex, NULL, 16);
            lee += 3;
        } else *escribe++ = *lee++;
    }
    *escribe = 0;
}

// Saca el valor de un parametro de la direccion. Devuelve false si no esta.
bool portal_parametro(const char *consulta, const char *cual, char *destino, int tam) {
    char busco[24];
    const int n = snprintf(busco, sizeof busco, "%s=", cual);
    const char *p = consulta;
    while ((p = strstr(p, busco))) {
        // Tiene que arrancar el parametro, no estar en el medio de otro.
        if (p == consulta || p[-1] == '&' || p[-1] == '?') {
            p += n;
            int i = 0;
            while (*p && *p != '&' && *p != ' ' && i < tam - 1) destino[i++] = *p++;
            destino[i] = 0;
            portal_destapar(destino);
            return true;
        }
        p += n;
    }
    destino[0] = 0;
    return false;
}

int portal_numero(const char *consulta, const char *cual, int si_no_esta) {
    char v[16];
    if (!portal_parametro(consulta, cual, v, sizeof v) || !v[0]) return si_no_esta;
    return (int)strtol(v, NULL, 10);
}

// Una pantalla por parametro, con los campos separados por "~". Se eligio asi
// y no JSON porque del otro lado lo arma la web, que puede armar cualquier
// cosa, y de este lado lo lee la placa, que con un JSON tendria que cargar un
// parser entero para leer siete campos.
//
//   p0=nombre~vista~lista~apt~km~tarjetas~tema~seguir~segundos
bool portal_leer_pantalla(const char *crudo, pantalla_t *p) {
    char copia[160];
    snprintf(copia, sizeof copia, "%s", crudo);
    memset(p, 0, sizeof *p);

    char *campos[9] = {0};
    int n = 0;
    char *t = copia;
    campos[n++] = t;
    while (*t && n < 9) {
        if (*t == '~') { *t = 0; campos[n++] = t + 1; }
        t++;
    }
    if (n < 9) return false;

    snprintf(p->nombre, sizeof p->nombre, "%s", campos[0]);
    p->vista    = (vista_t)strtol(campos[1], NULL, 10);
    p->lista    = (lista_t)strtol(campos[2], NULL, 10);
    snprintf(p->apt, sizeof p->apt, "%s", campos[3]);
    p->radio_km = (int)strtol(campos[4], NULL, 10);
    p->tarjetas = (int)strtol(campos[5], NULL, 10);
    snprintf(p->tema, sizeof p->tema, "%s", campos[6]);
    snprintf(p->seguir, sizeof p->seguir, "%s", campos[7]);
    p->segundos = (int)strtol(campos[8], NULL, 10);

    // Nada de lo que llega de afuera se cree sin acotarlo: una pantalla con
    // cero tarjetas o un radio de un millon de km rompe el dibujo.
    if (p->vista > VISTA_LOGOS) p->vista = VISTA_HIBRIDA;
    if (p->lista > LISTA_FIDS)  p->lista = LISTA_TARJETAS;
    if (p->radio_km < 5)   p->radio_km = 5;
    if (p->radio_km > 400) p->radio_km = 400;
    if (p->tarjetas < 1)   p->tarjetas = 1;
    if (p->tarjetas > 6)   p->tarjetas = 6;
    if (p->segundos < PANTALLA_SEGUNDOS_MIN) p->segundos = PANTALLA_SEGUNDOS_MIN;
    if (p->segundos > PANTALLA_SEGUNDOS_MAX) p->segundos = PANTALLA_SEGUNDOS_MAX;
    if (!p->apt[0]) return false;
    return true;
}


int portal_leer_pantallas(const char *consulta, pantalla_t *destino, int tope) {
    const int cuantas = portal_numero(consulta, "n", 0);
    if (cuantas < 1) return 0;
    int puestas = 0;
    for (int i = 0; i < cuantas && puestas < tope; i++) {
        char clave[8], crudo[160];
        snprintf(clave, sizeof clave, "p%d", i);
        if (!portal_parametro(consulta, clave, crudo, sizeof crudo)) continue;
        if (portal_leer_pantalla(crudo, &destino[puestas])) puestas++;
    }
    return puestas;
}
