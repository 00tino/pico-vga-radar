// Patron de prueba: verifica el video de 640x480 con 256 colores.
#include "vga.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/structs/sio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include <stdio.h>

static void rect(int x0, int y0, int x1, int y1, uint8_t c) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= VGA_ANCHO) x1 = VGA_ANCHO - 1;
    if (y1 >= VGA_ALTO) y1 = VGA_ALTO - 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            vga_fb[y * VGA_ANCHO + x] = c;
}

static void marco(uint8_t c) {
    rect(0, 0, VGA_ANCHO - 1, 1, c);
    rect(0, VGA_ALTO - 2, VGA_ANCHO - 1, VGA_ALTO - 1, c);
    rect(0, 0, 1, VGA_ALTO - 1, c);
    rect(VGA_ANCHO - 2, 0, VGA_ANCHO - 1, VGA_ALTO - 1, c);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2500);                       // margen para que el Mac tome el puerto
    printf("arrancando firmware en C\n");
    vga_init();
    printf("video inicializado: %dx%d, 256 colores\n", VGA_ANCHO, VGA_ALTO);
    vga_limpiar(0);

    // Marco al borde: si no se ve entero, el monitor esta recortando.
    marco(vga_color(7, 7, 3));

    // ROJO: 8 bandas anchas, una por nivel. Se pueden contar a ojo.
    for (int i = 0; i < 8; i++)
        rect(20 + i * 75, 20, 20 + i * 75 + 70, 150, vga_color(i, 0, 0));

    // VERDE: 8 bandas
    for (int i = 0; i < 8; i++)
        rect(20 + i * 75, 160, 20 + i * 75 + 70, 290, vga_color(0, i, 0));

    // AZUL: 4 bandas (el azul lleva 2 bits)
    for (int i = 0; i < 4; i++)
        rect(20 + i * 150, 300, 20 + i * 150 + 145, 400, vga_color(0, 0, i));

    // GRIS: mezcla de los tres, para ver el blanco y sus escalones
    for (int i = 0; i < 8; i++)
        rect(20 + i * 75, 410, 20 + i * 75 + 70, 470,
             vga_color(i, i, (uint8_t)(i * 3 / 7)));

    while (true) {
        const int N = 40000;
        int alto[10] = {0};
        for (int i = 0; i < N; i++) {
            uint32_t v = sio_hw->gpio_in;
            for (int p = 0; p < 10; p++)
                if (v & (1u << p)) alto[p]++;
        }
        printf("--- reloj %lu Hz ---\n", (unsigned long)clock_get_hz(clk_sys));
        printf("  hsync GP8: %5.2f%%   vsync GP9: %5.2f%%\n",
               100.0 * alto[8] / N, 100.0 * alto[9] / N);
        printf("  color GP0..GP7:");
        for (int p = 0; p < 8; p++) printf(" %.0f%%", 100.0 * alto[p] / N);
        printf("\n  PIO1 CTRL %08lx | DMA datos restantes %lu\n",
               (unsigned long)pio1->ctrl,
               (unsigned long)dma_hw->ch[0].transfer_count);
        printf("  banderas IRQ del PIO1: %02lx\n", (unsigned long)pio1->irq);
        printf("  donde esta parada cada maquina (PC):  hsync %lu  vsync %lu  rgb %lu\n",
               (unsigned long)pio1->sm[0].addr,
               (unsigned long)pio1->sm[1].addr,
               (unsigned long)pio1->sm[2].addr);
        printf("  nivel de la cola de pixeles: %lu\n",
               (unsigned long)((pio1->flevel >> 16) & 0xf));
        sleep_ms(3000);
    }
}
