// Volcado del framebuffer por el puerto serie, para poder mirar lo que dibuja
// la Pico sin depender de la camara ni del monitor. Se pide mandando 'v' por
// la consola; contesta una cabecera, el framebuffer en base64 y un cierre.
//
// Separa tres cosas que por foto son indistinguibles: lo que dibuja la Pico,
// lo que muestra el monitor y lo que capta la camara.
#include "vga.h"
#include <stdio.h>

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void volcado_fb(void) {
    printf("FB %d %d\n", VGA_ANCHO, VGA_ALTO);
    const uint8_t *p = vga_fb;
    const int n = VGA_ANCHO * VGA_ALTO;
    int col = 0;
    for (int i = 0; i + 2 < n; i += 3) {
        uint32_t v = ((uint32_t)p[i] << 16) | ((uint32_t)p[i + 1] << 8) | p[i + 2];
        putchar_raw(B64[(v >> 18) & 63]);
        putchar_raw(B64[(v >> 12) & 63]);
        putchar_raw(B64[(v >>  6) & 63]);
        putchar_raw(B64[ v        & 63]);
        if ((col += 4) >= 76) { putchar_raw('\n'); col = 0; }
    }
    putchar_raw('\n');
    printf("FBFIN\n");
}
