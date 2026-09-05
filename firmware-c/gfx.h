// Primitivas de dibujo sobre el framebuffer de vga.c.
// Todo recorta contra la pantalla: no hace falta chequear antes de llamar.
#ifndef GFX_H
#define GFX_H
#include <stdint.h>
#include "vga.h"

#define GFX_FUENTE_ALTO 14

extern const uint8_t gfx_fuente[96 * 15];

static inline void gfx_punto(int x, int y, uint8_t c) {
    if ((unsigned)x < VGA_ANCHO && (unsigned)y < VGA_ALTO)
        vga_fb[y * VGA_ANCHO + x] = c;
}

void gfx_hlinea(int x, int y, int largo, uint8_t c);
void gfx_vlinea(int x, int y, int largo, uint8_t c);
void gfx_linea(int x0, int y0, int x1, int y1, uint8_t c);

void gfx_rect(int x, int y, int an, int al, uint8_t c);        // contorno
void gfx_rect_lleno(int x, int y, int an, int al, uint8_t c);

void gfx_circulo(int cx, int cy, int r, uint8_t c);            // contorno
void gfx_circulo_lleno(int cx, int cy, int r, uint8_t c);

// Texto. escala 1 = 7x14, 2 = 14x28, etc. Devuelve el ancho dibujado.
int  gfx_texto(int x, int y, const char *s, uint8_t c, int escala);
int  gfx_ancho_texto(const char *s, int escala);
int  gfx_texto_centrado(int cx, int y, const char *s, uint8_t c, int escala);

#endif

// --- Color con dithering ordenado ---------------------------------
// Con 3-3-2 bits un degradado da bandas y vira de tono. El dither reparte
// el error entre pixeles vecinos usando una matriz de Bayer 4x4, asi que
// hace falta la posicion en pantalla para elegir el umbral.
uint8_t gfx_rgb_dither(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void gfx_rect_dither(int x, int y, int an, int al, uint8_t r, uint8_t g, uint8_t b);
