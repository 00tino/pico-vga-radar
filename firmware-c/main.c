// Patron que demuestra las primitivas de dibujo: texto, lineas, circulos
// y rectangulos. Usa el tema crt_amber de la web (#e8b86d sobre #0a0805).
#include "vga.h"
#include "gfx.h"
#include "area.h"
#include "logos.h"
#include "pico/stdlib.h"
#include <stdio.h>

static uint8_t fondo, ambar, tenue, apagado, blanco;

static void dibujar_patron(void) {
    vga_limpiar(fondo);

    // Marco: si no se ve entero, el monitor esta recortando.
    gfx_rect(0, 0, VGA_ANCHO, VGA_ALTO, apagado);

    // --- TEXTO: tres escalas y la fuente completa ---
    gfx_texto_centrado(320, 6, "PICO VGA RADAR", ambar, 2);
    gfx_texto_centrado(320, 36, "primitivas de dibujo en C  -  640x480  -  256 colores", tenue, 1);
    gfx_hlinea(20, 54, 600, apagado);

    gfx_texto(20, 62, "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", ambar, 1);
    gfx_texto(20, 78, "0123456789  !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", tenue, 1);

    // --- LINEAS: abanico de 24 rayos, mide los ocho octantes de Bresenham ---
    const int lx = 108, ly = 250, lr = 90;
    gfx_rect(18, 100, 180, 300, apagado);
    gfx_texto(26, 104, "LINEAS", tenue, 1);
    // seno en punto fijo (1/256) cada 15 grados, un cuadrante
    static const int sen90[7] = {0, 66, 128, 181, 222, 247, 256};
    for (int i = 0; i < 24; i++) {
        int q = i % 6, cuad = i / 6;
        int s = sen90[q], co = sen90[6 - q];
        int dx, dy;
        switch (cuad) {
            case 0: dx =  co; dy = -s;  break;
            case 1: dx =  s;  dy =  co; break;
            case 2: dx = -co; dy =  s;  break;
            default:dx = -s;  dy = -co; break;
        }
        gfx_linea(lx, ly, lx + dx * lr / 256, ly + dy * lr / 256,
                  (i % 6 == 0) ? ambar : tenue);
    }
    gfx_texto(26, 372, "24 rayos, 8 octantes", apagado, 1);

    // --- CIRCULOS: anillos de alcance y un blip lleno, como el radar ---
    const int cx = 320, cy = 250;
    gfx_rect(210, 100, 220, 300, apagado);
    gfx_texto(218, 104, "CIRCULOS", tenue, 1);
    for (int r = 25; r <= 100; r += 25)
        gfx_circulo(cx, cy, r, r == 100 ? ambar : tenue);
    gfx_linea(cx - 100, cy, cx + 100, cy, apagado);
    gfx_linea(cx, cy - 100, cx, cy + 100, apagado);
    gfx_circulo_lleno(cx, cy, 4, ambar);
    gfx_circulo_lleno(cx + 62, cy - 48, 5, blanco);
    gfx_circulo_lleno(cx - 74, cy + 30, 5, blanco);
    gfx_texto(218, 372, "contorno y relleno", apagado, 1);

    // --- RECTANGULOS: tarjeta con barra de progreso, como la de la web ---
    gfx_rect(442, 100, 180, 300, apagado);
    gfx_texto(450, 104, "RECTANGULOS", tenue, 1);
    gfx_rect(450, 124, 164, 86, tenue);            // tarjeta
    const uint8_t *logo = logo_buscar("AR");       // logo real de la flash
    if (logo) gfx_blit(458, 130, LOGO_LADO, LOGO_LADO, logo);
    else gfx_rect_lleno(458, 130, LOGO_LADO, LOGO_LADO, blanco);
    gfx_texto(502, 134, "AR1301", ambar, 1);
    gfx_texto(502, 150, "EZE > MAD", tenue, 1);
    gfx_rect(458, 176, 148, 8, apagado);           // barra de progreso
    gfx_rect_lleno(459, 177, 92, 6, ambar);
    gfx_texto(458, 190, "FL350  842 km/h", tenue, 1);

    // escalera de rectangulos anidados
    for (int i = 0; i < 6; i++)
        gfx_rect(450 + i * 6, 224 + i * 6, 164 - i * 12, 100 - i * 12,
                 (i & 1) ? tenue : apagado);
    gfx_texto(450, 372, "contorno y relleno", apagado, 1);

    // --- LOGOS: reales, sacados de la flash ---
    gfx_texto(20, 410, "LOGOS", tenue, 1);
    static const char *muestra[] = {"AR", "LA", "AA", "IB", "JL", "NH",
                                    "AF", "BA", "UA", "DL", "EK", "QF"};
    for (int i = 0; i < 12; i++) {
        const uint8_t *l = logo_buscar(muestra[i]);
        if (l) gfx_blit(80 + i * 44, 406, LOGO_LADO, LOGO_LADO, l);
    }
    gfx_texto(20, 426, "RAMPA", tenue, 1);
    for (int i = 0; i < 128; i++) {
        int n = 255 * i / 127;
        gfx_rect_dither(80 + i * 4, 448, 4, 14,
                        0xe8 * n / 255, 0xb8 * n / 255, 0x6d * n / 255);
    }
    gfx_texto(600, 448, "824 logos", apagado, 1);

}

int main(void) {
    stdio_init_all();
    sleep_ms(2500);                       // margen para que el Mac tome el puerto
    printf("arrancando firmware en C\n");
    vga_init();
    printf("video inicializado: %dx%d, 256 colores\n", VGA_ANCHO, VGA_ALTO);

    fondo   = vga_rgb(0x0a, 0x08, 0x05);
    ambar   = vga_rgb(0xe8, 0xb8, 0x6d);
    tenue   = vga_rgb(0x7a, 0x60, 0x38);
    apagado = vga_rgb(0x3d, 0x30, 0x1c);
    blanco  = vga_color(7, 7, 3);

    // Todavia sin margenes: primero hay que leerlos de la pantalla de
    // calibracion. Cuando esten, van aca.
    area_set(0, 0, 0, 0);

    // Alterna las dos pantallas para poder fotografiar las dos.
    while (true) {
        area_calibrar(fondo, tenue, ambar);
        printf("calibracion\n");
        sleep_ms(9000);
        dibujar_patron();
        printf("patron\n");
        sleep_ms(9000);
    }
}
