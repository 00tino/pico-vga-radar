// Decidir si un avion es el que el cliente pidio seguir.
//
// Esta aparte de radar.c para poder probarlo en la Mac: radar.c arrastra el
// video entero y no se compila fuera de la placa, y esto es pura logica con
// muchos casos al borde. Ver herramientas/probar_seguir.sh.
#include "seguir.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Separa un indicativo en sus letras y su numero: "ARG1885" da "ARG" y 1885.
// El numero queda en -1 si no tiene.
static void partir_indicativo(const char *s, char *letras, int tam, int *numero) {
    int i = 0;
    // Los espacios y los guiones no cuentan: ADS-B manda "AR 1885" y la
    // gente escribe "AR-1885".
    while (*s == ' ' || *s == '-') s++;
    while (*s && isalpha((unsigned char)*s)) {
        if (i < tam - 1) letras[i++] = (char)toupper((unsigned char)*s);
        s++;
    }
    letras[i] = 0;
    while (*s == ' ' || *s == '-') s++;
    *numero = isdigit((unsigned char)*s) ? (int)strtol(s, NULL, 10) : -1;
}

// Si ese avion es el que el cliente pidio seguir.
//
// No alcanza con comparar contra el indicativo tal cual. Lo que manda ADS-B
// es el codigo OACI de tres letras (ARG1885, ANS5500), pero el numero que el
// pasajero conoce, el que sale en el pasaje y el que ofrece la pagina, es el
// IATA de dos (AR1885, OY5500). Comparando de una sola forma, seguir un vuelo
// de Andes no encontraba nunca nada.
//
// Se acepta entonces el indicativo en cualquiera de las dos formas, y tambien
// solo la aerolinea, que sigue al primero de esa aerolinea que aparezca.
bool radar_sigue_a(const avion_t *a) {
    if (!radar_seguir[0] || !a->vuelo[0]) return false;

    char busca_al[8]; int busca_nro;
    partir_indicativo(radar_seguir, busca_al, sizeof busca_al, &busca_nro);
    if (!busca_al[0]) return false;

    char tiene_al[8]; int tiene_nro;
    partir_indicativo(a->vuelo, tiene_al, sizeof tiene_al, &tiene_nro);
    if (!tiene_al[0]) return false;

    // Las dos formas de nombrar a la aerolinea de ESTE avion: la que vino en
    // el indicativo y la de dos letras que se saco de la tabla.
    const bool misma_linea =
        !strcmp(busca_al, tiene_al) ||
        (a->aerolinea[0] && !strcmp(busca_al, a->aerolinea));
    if (!misma_linea) return false;

    // Sin numero, sigue al primero de esa aerolinea que aparezca.
    if (busca_nro < 0) return true;
    return busca_nro == tiene_nro;
}
