#include "vga.h"
#include "vga.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

// El framebuffer: 640x480 a un byte por pixel = 307.200 bytes.
// Alineado a su tamano para que el DMA pueda dar la vuelta solo al terminar.
uint8_t vga_fb[VGA_ANCHO * VGA_ALTO] __attribute__((aligned(4)));

// El WiFi usa PIO0, asi que el video va en PIO1. Esto ya nos mordio una vez
// con la version en MicroPython: en PIO0 la placa se cuelga al inicializar.
#define VGA_PIO pio1

static int dma_datos, dma_reinicio;
static uint8_t *const fb_ptr = vga_fb;

// Llega una vez por cuadro, durante el borrado vertical: el momento en que la
// maquina de pixeles esta parada esperando y se puede tocar sin romper nada.
static void vga_cuadro_nuevo(void) {
    pio_interrupt_clear(VGA_PIO, 0);
    dma_channel_abort(dma_datos);
    pio_sm_clear_fifos(VGA_PIO, 2);
    dma_channel_set_read_addr(dma_datos, vga_fb, true);
}

void vga_limpiar(uint8_t color) {
    memset(vga_fb, color, sizeof(vga_fb));
}

void vga_init(void) {
    vga_limpiar(0);

    // 100,8 MHz. Sale de dividir el VCO de 1008 MHz por 10, y a su vez divide
    // exacto para las tres maquinas de estado:
    //     hsync y vsync  6,3 MHz  = 100,8 / 16
    //     pixeles       50,4 MHz  = 100,8 / 2
    // Sin divisores fraccionarios, que meten temblor en la imagen. Y sin
    // sobrefrecuencia: a 252 MHz la placa puede no arrancar por tension.
    if (!set_sys_clock_khz(100800, false)) {
        set_sys_clock_khz(126000, true);   // plan B
    }

    uint off_h = pio_add_program(VGA_PIO, &hsync_program);
    uint off_v = pio_add_program(VGA_PIO, &vsync_program);
    uint off_r = pio_add_program(VGA_PIO, &rgb_program);
    printf("PIO: hsync en %u, vsync en %u, rgb en %u (de 32 lugares)\n",
           off_h, off_v, off_r);

    const float f = (float)clock_get_hz(clk_sys);

    // hsync y vsync: 6,3 MHz (un cuarto del reloj de pixel)
    pio_sm_config c = hsync_program_get_default_config(off_h);
    sm_config_set_sideset_pins(&c, VGA_PIN_HSYNC);
    sm_config_set_clkdiv(&c, f / 6300000.0f);
    pio_gpio_init(VGA_PIO, VGA_PIN_HSYNC);
    pio_sm_set_consecutive_pindirs(VGA_PIO, 0, VGA_PIN_HSYNC, 1, true);
    pio_sm_init(VGA_PIO, 0, off_h, &c);

    c = vsync_program_get_default_config(off_v);
    sm_config_set_sideset_pins(&c, VGA_PIN_VSYNC);
    sm_config_set_clkdiv(&c, f / 6300000.0f);
    pio_gpio_init(VGA_PIO, VGA_PIN_VSYNC);
    pio_sm_set_consecutive_pindirs(VGA_PIO, 1, VGA_PIN_VSYNC, 1, true);
    pio_sm_init(VGA_PIO, 1, off_v, &c);

    // rgb: 50,4 MHz, el doble del reloj de pixel porque el bucle
    // gasta dos instrucciones por pixel (sacar y saltar).
    c = rgb_program_get_default_config(off_r);
    sm_config_set_out_pins(&c, VGA_PIN_COLOR_BASE, 8);
    sm_config_set_out_shift(&c, true, true, 32);   // hacia la derecha, autocarga
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, f / 50400000.0f);
    for (int i = 0; i < 8; i++) pio_gpio_init(VGA_PIO, VGA_PIN_COLOR_BASE + i);
    pio_sm_set_consecutive_pindirs(VGA_PIO, 2, VGA_PIN_COLOR_BASE, 8, true);
    pio_sm_init(VGA_PIO, 2, off_r, &c);

    // Cuantos pixeles tiene una linea (el bucle cuenta hasta cero).
    pio_sm_put_blocking(VGA_PIO, 2, VGA_ANCHO - 1);

    // Dos canales de DMA encadenados: uno vuelca el framebuffer en el PIO,
    // el otro reinicia la direccion de lectura al terminar el cuadro. Asi el
    // video se mantiene solo, sin que el procesador tenga que intervenir.
    dma_datos    = dma_claim_unused_channel(true);
    dma_reinicio = dma_claim_unused_channel(true);

    // De a 32 bits: cada palabra lleva CUATRO pixeles, que es justo lo que la
    // maquina de pixeles saca por cada recarga automatica. Transfiriendo de a
    // un byte, tres de cada cuatro pixeles salian basura y el DMA corria
    // cuatro veces mas rapido que el video.
    dma_channel_config d = dma_channel_get_default_config(dma_datos);
    channel_config_set_transfer_data_size(&d, DMA_SIZE_32);
    channel_config_set_read_increment(&d, true);
    channel_config_set_write_increment(&d, false);
    channel_config_set_dreq(&d, pio_get_dreq(VGA_PIO, 2, true));
    channel_config_set_high_priority(&d, true);
    dma_channel_configure(dma_datos, &d, &VGA_PIO->txf[2], vga_fb,
                          sizeof(vga_fb) / 4, false);

    // Reiniciar el DMA en cada cuadro. Sin esto la imagen se desplaza sola:
    // lo que queda en la cola al terminar un cuadro corre el arranque del
    // siguiente, y el corrimiento se acumula. Es el temblor que se ve como
    // si la pantalla se tirara para un costado.
    irq_set_exclusive_handler(PIO1_IRQ_0, vga_cuadro_nuevo);
    irq_set_priority(PIO1_IRQ_0, 0);            // maxima: no puede llegar tarde
    pio_set_irq0_source_enabled(VGA_PIO, pis_interrupt0, true);
    irq_set_enabled(PIO1_IRQ_0, true);

    dma_channel_start(dma_datos);

    // Las tres maquinas arrancan juntas para que queden sincronizadas.
    pio_enable_sm_mask_in_sync(VGA_PIO, (1u << 0) | (1u << 1) | (1u << 2));
}
