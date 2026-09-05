#include "area.h"
#include "gfx.h"
#include <stdio.h>

area_t area = { 0, 0, VGA_ANCHO, VGA_ALTO };

void area_set(int arriba, int abajo, int izquierda, int derecha) {
    if (arriba < 0) arriba = 0;
    if (abajo < 0) abajo = 0;
    if (izquierda < 0) izquierda = 0;
    if (derecha < 0) derecha = 0;
    // Nunca dejar el area en nada: si los margenes se pasan, se ignoran.
    if (arriba + abajo >= VGA_ALTO || izquierda + derecha >= VGA_ANCHO) {
        arriba = abajo = izquierda = derecha = 0;
    }
    area.x  = izquierda;
    area.y  = arriba;
    area.an = VGA_ANCHO - izquierda - derecha;
    area.al = VGA_ALTO  - arriba   - abajo;
}

// Reglas contra los cuatro bordes del framebuffer, no del area: la idea es
// mirar el monitor y ver desde que numero se empieza a leer cada regla.
// Ese numero es el margen que hay que cargar en ese borde.
void area_calibrar(uint8_t fondo, uint8_t trazo, uint8_t acento) {
    vga_limpiar(fondo);
    char n[8];

    // Marcas cada 5 px, mas largas y numeradas cada 20.
    for (int x = 0; x < VGA_ANCHO; x += 5) {
        int grande = (x % 20 == 0);
        gfx_vlinea(x, 0, grande ? 10 : 5, grande ? acento : trazo);
        gfx_vlinea(x, VGA_ALTO - (grande ? 10 : 5), grande ? 10 : 5, grande ? acento : trazo);
        if (grande && x % 40 == 0 && x > 0) {
            snprintf(n, sizeof n, "%d", x);
            gfx_texto_centrado(x, 12, n, trazo, 1);
            gfx_texto_centrado(x, VGA_ALTO - 26, n, trazo, 1);
        }
    }
    for (int y = 0; y < VGA_ALTO; y += 5) {
        int grande = (y % 20 == 0);
        gfx_hlinea(0, y, grande ? 10 : 5, grande ? acento : trazo);
        gfx_hlinea(VGA_ANCHO - (grande ? 10 : 5), y, grande ? 10 : 5, grande ? acento : trazo);
        if (grande && y % 40 == 0 && y > 0) {
            snprintf(n, sizeof n, "%d", y);
            gfx_texto(14, y - 7, n, trazo, 1);
            gfx_texto(VGA_ANCHO - 14 - gfx_ancho_texto(n, 1), y - 7, n, trazo, 1);
        }
    }

    // El borde del area util cargada: tiene que quedar entero a la vista.
    gfx_rect(area.x, area.y, area.an, area.al, acento);
    snprintf(n, sizeof n, "%dx%d", area.an, area.al);
    gfx_texto_centrado(area_cx(), area_cy() - 24, "AREA UTIL", acento, 2);
    gfx_texto_centrado(area_cx(), area_cy() + 8, n, trazo, 1);
    gfx_texto_centrado(area_cx(), area_cy() + 26,
                       "si este marco se ve entero, los margenes estan bien", trazo, 1);
}
