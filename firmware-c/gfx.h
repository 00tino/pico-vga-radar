// Primitivas de dibujo sobre el framebuffer de vga.c.
// Todo recorta contra la pantalla: no hace falta chequear antes de llamar.
#ifndef GFX_H
#define GFX_H
#include <stdint.h>
#include "vga.h"

#define GFX_FUENTE_ALTO 14

extern const uint8_t gfx_fuente[96 * 15];

// Banda de filas habilitada para dibujar.
//
// El framebuffer es el mismo que el monitor esta leyendo: no hay lugar en la
// RAM para un segundo buffer (307 KB cada uno, y la Pico tiene 520). Por eso
// el cuadro se dibuja por bandas de arriba hacia abajo, cada una completa
// antes de que el haz llegue: como el cuadro entero lleva la mitad del tiempo
// que tarda el haz en bajar, nunca lo alcanza. Sin esto, lo que se dibuja
// ultimo en la parte de arriba no llega a tiempo y no se ve.
extern int gfx_banda_y0, gfx_banda_y1;

static inline void gfx_banda(int y0, int y1) { gfx_banda_y0 = y0; gfx_banda_y1 = y1; }

static inline void gfx_punto(int x, int y, uint8_t c) {
    if ((unsigned)x < VGA_ANCHO && y >= gfx_banda_y0 && y <= gfx_banda_y1)
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

// Copia una imagen ya en formato de pantalla (un byte por pixel).
void gfx_blit(int x, int y, int an, int al, const uint8_t *datos);

// Mezcla un color con el fondo, como el globalAlpha del canvas de la web.
// a va de 0 (todo fondo) a 255 (todo el color de adelante).
uint8_t gfx_mezcla(uint8_t r, uint8_t g, uint8_t b,
                   uint8_t fr, uint8_t fg_, uint8_t fb, int a);

void gfx_triangulo_lleno(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c);
void gfx_linea_punteada(int x0, int y0, int x1, int y1, int trazo, int hueco, uint8_t c);

// Cuna del barrido: sector lleno entre dos angulos, en unidades de trig.h.
void gfx_sector(int cx, int cy, int r, int a0, int a1, uint8_t c);

// Relleno de poligono por barrido de filas. Se usa para las costas del mapa.
#define GFX_POLIGONO_MAX 900   // el contorno mas grande de land.json tiene 776 puntos
void gfx_poligono_lleno(const int *xs, const int *ys, int n, uint8_t c);

// Guarda y repone un rectangulo del framebuffer. Sirve para mover algo
// encima de un fondo caro de dibujar sin tener que rehacer el fondo.
void gfx_guardar(int x, int y, int an, int al, uint8_t *dst);
void gfx_reponer(int x, int y, int an, int al, const uint8_t *src);

// Recorta un poligono contra un rectangulo (Sutherland-Hodgman) y lo rellena.
// Acotar los puntos sueltos al borde no sirve: junta vertices que estan lejos
// y el relleno se escapa en franjas a lo ancho de la pantalla.
int gfx_poligono_recortar(const int *xs, const int *ys, int n,
                          int rx0, int ry0, int rx1, int ry1,
                          int *sx, int *sy, int max);
