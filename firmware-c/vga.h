// Video VGA 640x480 con 256 colores para la Pico 2 W.
#ifndef VGA_H
#define VGA_H
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define VGA_ANCHO 640
#define VGA_ALTO  480

// Pines. Los ocho de color TIENEN que ser consecutivos: el PIO los escribe
// de a ocho con una sola instruccion.
#define VGA_PIN_COLOR_BASE 0      // GP0..GP7
#define VGA_PIN_HSYNC      8
#define VGA_PIN_VSYNC      9

// Un pixel es un byte: RRRGGGBB.
//   bits 7-5 rojo   (GP7 GP6 GP5)
//   bits 4-2 verde  (GP4 GP3 GP2)
//   bits 1-0 azul   (GP1 GP0)
static inline uint8_t vga_color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint8_t)(((r & 7) << 5) | ((g & 7) << 2) | (b & 3));
}

// Desde 0-255 por canal, como se piensa un color en la web.
// Redondea en vez de truncar: con >>5 un 232 caia en el nivel 7 (=255) y
// todos los colores salian mas claros y desbalanceados de lo pedido.
static inline uint8_t vga_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return vga_color((uint8_t)((r * 7 + 127) / 255),
                     (uint8_t)((g * 7 + 127) / 255),
                     (uint8_t)((b * 3 + 127) / 255));
}

extern uint8_t vga_fb[VGA_ANCHO * VGA_ALTO];

// Sube uno por cuadro, desde la interrupcion de vsync. Sirve para dibujar
// justo despues del borrado vertical y que no se parta la imagen.
extern volatile uint32_t vga_cuadros;

// Cuando empezo el borrado vertical del cuadro que se esta dibujando. Sirve
// para saber por cuanto le gana el dibujo al haz en cada banda: si el haz lo
// alcanza, esa franja de la pantalla queda rota y titila.
extern volatile uint32_t vga_us_vsync;

// Espera al proximo cuadro.
static inline void vga_esperar_cuadro(void) {
    uint32_t n = vga_cuadros;
    while (vga_cuadros == n) tight_loop_contents();
}

// Deja el reloj del sistema en su valor final. Va PRIMERO de todo, antes de
// arrancar la radio y antes de vga_init(). Ver vga.c.
void vga_reloj(void);

void vga_init(void);
void vga_limpiar(uint8_t color);
void vga_limpiar_filas(int y0, int y1, uint8_t color);
void vga_limpiar_rect(int x, int y0, int an, int y1, uint8_t color);

#endif
