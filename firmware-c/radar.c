#include "radar.h"
#include "gfx.h"
#include "area.h"
#include "trig.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

// crt_amber, el tema por defecto de la web: #e8b86d sobre #0a0805.
tema_t radar_tema = { 0xe8, 0xb8, 0x6d, 0x0a, 0x08, 0x05 };

aeropuerto_t radar_apt = { "EZE", "Buenos Aires (Ezeiza)", -348220, -585360, 150 };

avion_t radar_aviones[RADAR_MAX_AVIONES];
int radar_cantidad = 0;

static int beam = 0;              // angulo del barrido, en unidades de trig.h

// La web usa alpha sobre el color del tema. Aca se mezcla contra el fondo,
// que da el mismo resultado porque el fondo es plano.
static uint8_t tono(int alpha255) {
    return gfx_mezcla(radar_tema.fr, radar_tema.fg, radar_tema.fb,
                      radar_tema.br, radar_tema.bg, radar_tema.bb, alpha255);
}

void radar_init(void) {
    trig_init();
    beam = 0;
}

// Diferencia angular mas corta, en unidades de vuelta.
static int dif_ang(int a, int b) {
    int d = ((a - b) % TRIG_VUELTA + TRIG_VUELTA) % TRIG_VUELTA;
    return d > TRIG_VUELTA / 2 ? TRIG_VUELTA - d : d;
}

// Avanza el estado un cuadro: el barrido gira y el fosforo de los aviones
// decae. Va aparte del pintado porque el pintado se repite una vez por banda.
void radar_avanzar(void) {
    // El barrido avanza 0,012 radianes por cuadro = 1,96 unidades de 1024.
    beam = (beam + 2) % TRIG_VUELTA;

    const int cx = area.x + area.an / 2;
    const int cy = area.y + area.al * 52 / 100;
    const int semi = (area.an / 2 < area.al * 52 / 100 ? area.an / 2 : area.al * 52 / 100);
    const int R = semi * 86 / 100;
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;

    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = cx + (int)((int64_t)(a->lon - radar_apt.lon) * R / span);
        int y = cy - (int)((int64_t)(a->lat - radar_apt.lat) * R / span);
        int ang = trig_atan2(y - cy, x - cx);
        if (dif_ang(ang, beam) < 8) a->brillo = 255;
        else if (a->brillo > 82) a->brillo -= 1;          // 0,0024 por cuadro
    }
}

static void radar_pintar(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    const uint8_t fondo = tono(0);

    // Se limpia solo la banda que se esta dibujando, no toda la pantalla.
    vga_limpiar_filas(gfx_banda_y0, gfx_banda_y1, fondo);

    // Geometria igual a la de la web: centro un poco abajo del medio.
    const int cx = X + AN / 2;
    const int cy = Y + AL * 52 / 100;
    const int semi = (AN / 2 < AL * 52 / 100 ? AN / 2 : AL * 52 / 100);
    const int R = semi * 86 / 100;

    // Cuna del barrido: 0,28 radianes = 46 unidades. Va primero para que los
    // anillos y los aviones queden por encima, como en la web.
    gfx_sector(cx, cy, R, beam - 46, beam, tono(36));     // alpha 0,14
    gfx_linea(cx, cy, cx + trig_cos(beam) * R / TRIG_UNO,
                      cy + trig_sen(beam) * R / TRIG_UNO, tono(102));  // alpha 0,4

    // Anillos de alcance y cruz, alpha 0x28 como en la web.
    const uint8_t rejilla = tono(40);
    for (int i = 1; i <= 4; i++) gfx_circulo(cx, cy, R * i / 4, rejilla);
    gfx_linea(cx, cy - R, cx, cy + R, rejilla);
    gfx_linea(cx - R, cy, cx + R, cy, rejilla);

    // Encabezado: codigo y nombre del aeropuerto.
    gfx_texto(X + 10, Y + 8, radar_apt.iata, tono(255), 1);
    gfx_texto(X + 10, Y + 24, radar_apt.nombre, tono(178), 1);

    // Etiquetas de alcance sobre el eje horizontal.
    char km[12];
    for (int i = 1; i <= 4; i++) {
        snprintf(km, sizeof km, "%d", radar_apt.radio_km * i / 4);
        gfx_texto(cx + R * i / 4 - gfx_ancho_texto(km, 1) - 2, cy + 3, km, rejilla, 1);
    }
    gfx_texto(cx + R - gfx_ancho_texto("km", 1) - 2, cy - 14, "km", rejilla, 1);

    // Grados de 1e-4 que entran en el radio del scope: 1 grado = 111 km.
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;

    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = cx + (int)((int64_t)(a->lon - radar_apt.lon) * R / span);
        int y = cy - (int)((int64_t)(a->lat - radar_apt.lat) * R / span);
        if (x < X || x > X + AN || y < Y || y > Y + AL) continue;

        const uint8_t c = tono(a->brillo);

        // Triangulito apuntando al rumbo. En la web: (0,-6) (3.6,5) (-3.6,5).
        int t = trig_de_grados(a->track);
        int s = trig_sen(t), co = trig_cos(t);
        // Rotacion: el eje del avion es -Y, o sea que hay que girar el vector.
        // px y py van en pixeles: la unica division por TRIG_UNO es la que
        // saca la escala del seno y el coseno.
        #define ROT_X(px, py) (x + ((px) * co - (py) * s) / TRIG_UNO)
        #define ROT_Y(px, py) (y + ((px) * s + (py) * co) / TRIG_UNO)
        gfx_triangulo_lleno(ROT_X(0, -6),  ROT_Y(0, -6),
                            ROT_X(4,  5),  ROT_Y(4,  5),
                            ROT_X(-4, 5),  ROT_Y(-4, 5), c);
        #undef ROT_X
        #undef ROT_Y
        if (a->vuelo[0]) gfx_texto(x + 8, y - 12, a->vuelo, c, 1);
    }
}

// Un cuadro entero, por bandas de arriba hacia abajo. Ver gfx.h: es lo que
// evita que se pierda lo que se dibuja en la parte de arriba.
#define RADAR_BANDAS 4

void radar_cuadro(void) {
    const int alto = (VGA_ALTO + RADAR_BANDAS - 1) / RADAR_BANDAS;
    for (int b = 0; b < RADAR_BANDAS; b++) {
        int y0 = b * alto;
        int y1 = y0 + alto - 1;
        if (y1 > VGA_ALTO - 1) y1 = VGA_ALTO - 1;
        gfx_banda(y0, y1);
        radar_pintar();
    }
    gfx_banda(0, VGA_ALTO - 1);
}
