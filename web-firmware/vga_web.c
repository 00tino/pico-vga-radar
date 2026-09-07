// El framebuffer, sin nada de hardware.
//
// En la placa, vga.c ademas maneja el PIO, el DMA y los sincronismos. Nada de
// eso hace falta para dibujar: el dibujo escribe en vga_fb y listo. Asi la
// web puede usar el mismo gfx.c, radar.c, tarjetas.c y viaje.c que la Pico,
// que es lo unico que garantiza que se vea igual.
#include "vga.h"
#include <string.h>

uint8_t vga_fb[VGA_ANCHO * VGA_ALTO];
volatile uint32_t vga_cuadros = 0;
volatile uint32_t vga_us_vsync = 0;

// Un reloj de mentira que avanza un cuadro por llamada al dibujo. El firmware
// lo usa para el margen contra el haz, que en la web no significa nada.
static uint32_t reloj_us = 0;
uint32_t time_us_32(void) { return reloj_us += 10; }
void vga_web_nuevo_cuadro(void) { vga_cuadros++; vga_us_vsync = reloj_us; }

void vga_init(void) {}

void vga_limpiar(uint8_t color) { memset(vga_fb, color, sizeof vga_fb); }

void vga_limpiar_filas(int y0, int y1, uint8_t color) {
    if (y0 < 0) y0 = 0;
    if (y1 > VGA_ALTO - 1) y1 = VGA_ALTO - 1;
    if (y0 > y1) return;
    memset(&vga_fb[y0 * VGA_ANCHO], color, (size_t)(y1 - y0 + 1) * VGA_ANCHO);
}

void vga_limpiar_rect(int x, int y0, int an, int y1, uint8_t color) {
    if (x < 0) { an += x; x = 0; }
    if (x + an > VGA_ANCHO) an = VGA_ANCHO - x;
    if (an <= 0) return;
    if (y0 < 0) y0 = 0;
    if (y1 > VGA_ALTO - 1) y1 = VGA_ALTO - 1;
    for (int y = y0; y <= y1; y++) memset(&vga_fb[y * VGA_ANCHO + x], color, (size_t)an);
}
