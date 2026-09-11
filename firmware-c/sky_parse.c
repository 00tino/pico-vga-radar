// El parser de lo que contesta el proxy.
//
// Esta aparte de sky.c a proposito: sky.c no se puede compilar fuera de la
// placa (usa la radio, lwIP y los dos nucleos), y esto si. Asi la parte que
// mas facil se rompe cuando cambia el formato se prueba en la Mac, con una
// respuesta de verdad, sin tener que cargar el firmware.
//
// Ver herramientas/probar_sky.sh.
#include "sky_parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Corta el campo hasta el proximo "|" y lo copia con su cero al final. Deja
// el puntero justo despues del separador, o en el fin de linea si era el
// ultimo.
static const char *campo(const char *s, char *destino, int tam) {
    int i = 0;
    while (*s && *s != '|' && *s != '\n' && *s != '\r') {
        if (i < tam - 1) destino[i++] = *s;
        s++;
    }
    destino[i] = 0;
    return (*s == '|') ? s + 1 : s;
}

static const char *campo_int(const char *s, int32_t *destino, int32_t si_vacio) {
    char tmp[16];
    s = campo(s, tmp, sizeof tmp);
    *destino = tmp[0] ? (int32_t)strtol(tmp, NULL, 10) : si_vacio;
    return s;
}

int sky_parse(const char *texto, sky_avion_t *destino, int tope, int *hora_min) {
    const char *s = texto;
    int n = 0;
    if (hora_min) *hora_min = -1;

    // Primera linea: "#1 <epoch utc> <tz en minutos> <fuente> <cantidad>".
    if (s[0] == '#') {
        long epoch = 0, tz = 0;
        // El epoch se mira contra una fecha ya pasada: si el proxy contesta
        // un error, ahi hay un cero y no una hora.
        if (sscanf(s, "#1 %ld %ld", &epoch, &tz) == 2 && epoch > 1700000000L && hora_min) {
            long local = (epoch + tz * 60) % 86400L;
            if (local < 0) local += 86400L;
            *hora_min = (int)(local / 60);
        }
        while (*s && *s != '\n') s++;
        if (*s == '\n') s++;
    }

    while (*s && n < tope) {
        if (*s == '\r' || *s == '\n') { s++; continue; }
        sky_avion_t *a = &destino[n];
        memset(a, 0, sizeof *a);
        int32_t v;
        s = campo(s, a->hex, sizeof a->hex);
        s = campo(s, a->vuelo, sizeof a->vuelo);
        s = campo(s, a->matricula, sizeof a->matricula);
        s = campo(s, a->tipo, sizeof a->tipo);
        s = campo_int(s, &a->lat, 0);
        s = campo_int(s, &a->lon, 0);
        s = campo_int(s, &v, 0);   a->alt = v;
        s = campo_int(s, &v, 0);   a->gs = (int16_t)v;
        // Parado en plataforma no manda rumbo: queda en -1 y el que dibuja
        // le deja el ultimo que tenia.
        s = campo_int(s, &v, -1);  a->track = (int16_t)v;
        s = campo(s, a->origen, sizeof a->origen);
        s = campo(s, a->destino, sizeof a->destino);
        while (*s && *s != '\n') s++;
        if (*s == '\n') s++;
        // Sin posicion no se puede dibujar. El proxy ya los filtra, pero una
        // linea cortada por el camino tambien cae aca.
        if (a->lat || a->lon) n++;
    }
    return n;
}
