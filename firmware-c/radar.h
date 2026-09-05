// El radar: lo mismo que dibuja loop() en docs/radar.js, pero en C.
//
// Las medidas salen de ahi y no se inventan: anillos a 1/4, 1/2, 3/4 y 1 del
// radio, centro en 0,52 del alto, radio 0,86 del semilado menor, barrido de
// 0,28 radianes que avanza 0,012 por cuadro, y aviones que se encienden
// cuando el barrido les pasa por encima y se van apagando hasta 0,32.
#ifndef RADAR_H
#define RADAR_H
#include <stdint.h>
#include <stdbool.h>

#define RADAR_MAX_AVIONES 32

typedef struct {
    int32_t lat, lon;      // grados x 10000
    int16_t track;         // rumbo en grados, 0 = norte
    int16_t gs;            // velocidad en nudos
    int32_t alt;           // pies
    char vuelo[9];
    char aerolinea[3];     // IATA, para el logo
    uint8_t brillo;        // 0-255, el fosforo que deja el barrido

    // Lo que va en la tarjeta, igual que cardHTML() en la web.
    char tipo[8];          // modelo, por ejemplo A333
    char origen[4], destino[4];
    char ciudad_o[18], ciudad_d[18];
    char dep[6], arr[6];   // horarios hh:mm
    uint8_t pct;           // avance del vuelo, 0-100
    char estado[12];       // EN VUELO, APROXIMANDO, EN TIERRA
} avion_t;

typedef struct {
    char iata[4];
    char nombre[28];
    int32_t lat, lon;      // grados x 10000
    int radio_km;          // alcance del scope
} aeropuerto_t;

// Tema: color de trazo y de fondo, como THEMES en la web.
typedef struct {
    uint8_t fr, fg, fb;    // trazo
    uint8_t br, bg, bb;    // fondo
} tema_t;

extern tema_t radar_tema;
extern aeropuerto_t radar_apt;
extern avion_t radar_aviones[RADAR_MAX_AVIONES];
extern int radar_cantidad;

void radar_init(void);
void radar_marcar_sucio(void);   // pide que se redibujen las tarjetas
void radar_avanzar(void);    // gira el barrido y apaga el fosforo
void radar_cuadro(void);     // redibuja, por bandas de arriba hacia abajo

// Las cuatro vistas de la web: solo el scope, el scope con las tarjetas al
// costado, solo las tarjetas a pantalla completa, y el seguimiento de un vuelo.
typedef enum { VISTA_RADAR, VISTA_HIBRIDA, VISTA_PARED, VISTA_SEGUIR } vista_t;
extern vista_t radar_vista;

// Cuantas tarjetas por pagina y cada cuantos segundos rota el carrusel.
extern int radar_tarjetas;
extern int radar_rotacion_s;

// Indicativo del vuelo que se sigue en VISTA_SEGUIR.
extern char radar_seguir[9];
extern bool radar_pistas_on;

#endif
