// Pantalla de logos: una grilla con muchos, para revisarlos de una mirada.
//
// Sirve para ver si a alguno le quedaron puntas blancas en las esquinas o si
// se lee mal a 36 pixeles. Avanza de pagina sola cada tantos segundos.
#include "radar.h"
#include "gfx.h"
#include "area.h"
#include "logos.h"
#include <stdio.h>

extern uint8_t radar_tono(int alpha255);

static int pagina_logos = 0;
static uint32_t cuadros = 0;
static int sucio = 1;
static int fila_en_curso = 0;   // se dibuja una fila de logos por cuadro

void logos_pantalla_avanzar(void) {
    if (++cuadros >= 8 * 60) {            // ocho segundos por pagina
        cuadros = 0;
        pagina_logos++;
        sucio = 1;
        fila_en_curso = 0;
    }
}

void logos_pantalla_rehacer(void) { sucio = 1; fila_en_curso = 0; }

void logos_pantalla_pintar(void) {
    // Solo al cambiar de pagina, y de a una fila de logos por cuadro:
    // ochenta y cuatro logos de una no entran en el tiempo del cuadro, el haz
    // alcanzaba al dibujo por casi nueve milisegundos y la pagina entraba
    // rota. Repartida en filas, cada cuadro cuesta poco.
    if (!sucio) return;
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    if (fila_en_curso == 0) vga_limpiar_filas(0, VGA_ALTO - 1, radar_tono(0));

    const int paso_x = LOGO_LADO + 14;
    const int paso_y = LOGO_LADO + 20;
    const int cols = (AN - 20) / paso_x;
    const int filas = (AL - 40) / paso_y;
    const int por_pagina = cols * filas;
    const int paginas = (LOGOS_CANT + por_pagina - 1) / por_pagina;
    const int pag = pagina_logos % (paginas > 0 ? paginas : 1);

    // El titulo va en la primera pasada nada mas: rehacerlo en cada una es
    // trabajo de mas justo en las filas de arriba.
    if (fila_en_curso == 0) {
        char meta[64];
        snprintf(meta, sizeof meta, "%d logos - pagina %d de %d", LOGOS_CANT, pag + 1, paginas);
        gfx_texto(X + 10, Y + 8, "LOGOS", radar_tono(255), 1);
        gfx_texto(X + AN - 10 - gfx_ancho_texto(meta, 1), Y + 8, meta, radar_tono(170), 1);
        gfx_hlinea(X + 10, Y + 26, AN - 20, radar_tono(45));
    }

    for (int i = fila_en_curso * cols; i < (fila_en_curso + 1) * cols && i < por_pagina; i++) {
        int k = pag * por_pagina + i;
        if (k >= LOGOS_CANT) break;
        int cx = X + 10 + (i % cols) * paso_x;
        int cy = Y + 34 + (i / cols) * paso_y;
        // Marco fino: hay logos de fondo blanco con el dibujo muy tenue, y
        // sin borde sobre fondo oscuro parecen un hueco.
        gfx_rect(cx - 1, cy - 1, LOGO_LADO + 2, LOGO_LADO + 2, radar_tono(70));
        gfx_blit(cx, cy, LOGO_LADO, LOGO_LADO, &logos_datos[logos_indice[k].offset]);
        char cod[4] = { logos_indice[k].codigo[0], logos_indice[k].codigo[1], 0, 0 };
        gfx_texto_centrado(cx + LOGO_LADO / 2, cy + LOGO_LADO + 2, cod, radar_tono(150), 1);
    }
    if (++fila_en_curso >= filas) sucio = 0;
}
