// Video VGA 640x480 con 256 colores para la Pico 2 W.
#ifndef VGA_H
#define VGA_H
#include <stdint.h>
#include <stdbool.h>

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
static inline uint8_t vga_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return vga_color(r >> 5, g >> 5, b >> 6);
}

extern uint8_t vga_fb[VGA_ANCHO * VGA_ALTO];

void vga_init(void);
void vga_limpiar(uint8_t color);

#endif
