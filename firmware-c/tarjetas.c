// Tarjetas de vuelo, la columna que va al costado del scope.
//
// Es la traduccion de cardHTML() y .fa-card de la web: fila con el logo, el
// indicativo y el estado; despues el subtitulo, la ruta con barra de avance,
// las ciudades, los horarios y las metricas. Cuando no entra, se van sacando
// filas en el mismo orden que SHRINK_ORDER de la web.
#include "radar.h"
#include "gfx.h"
#include "logos.h"
#include "trig.h"
#include <stdio.h>
#include <string.h>

extern uint8_t radar_tono(int alpha255);

// Recorta un texto para que entre en un ancho, con puntos suspensivos.
static void acortar(char *dst, size_t n, const char *src, int ancho_px) {
    snprintf(dst, n, "%s", src);
    if (gfx_ancho_texto(dst, 1) <= ancho_px) return;
    for (int len = (int)strlen(dst); len > 1; len--) {
        dst[len - 1] = 0;
        char prueba[40];
        snprintf(prueba, sizeof prueba, "%s...", dst);
        if (gfx_ancho_texto(prueba, 1) <= ancho_px) { snprintf(dst, n, "%s", prueba); return; }
    }
}

// La pastilla de estado: marco fino con el texto adentro, como .pill.
static void pastilla(int x, int y, const char *txt, uint8_t c) {
    int an = gfx_ancho_texto(txt, 1) + 8;
    gfx_rect(x, y, an, 16, c);
    gfx_texto(x + 4, y + 1, txt, c, 1);
}

// Dibuja una tarjeta y devuelve el alto que ocupo.
void tarjeta_dibujar(int x, int y, int an, int al, const avion_t *a) {
    const uint8_t borde  = radar_tono(56);
    const uint8_t fuerte = radar_tono(255);
    const uint8_t medio  = radar_tono(180);
    const uint8_t suave  = radar_tono(120);

    gfx_rect(x, y, an, al, borde);

    const int px = x + 8;          // margen interno
    const int anu = an - 16;       // ancho util
    int cy = y + 7;

    // Fila de arriba: logo, indicativo y estado.
    const uint8_t *logo = logo_buscar(a->aerolinea);
    if (logo) gfx_blit(px, cy, LOGO_LADO, LOGO_LADO, logo);
    else      gfx_rect(px, cy, LOGO_LADO, LOGO_LADO, borde);

    const int tx = px + LOGO_LADO + 8;
    const int tan = anu - LOGO_LADO - 8;
    gfx_texto(tx, cy, a->vuelo, fuerte, 1);

    // El estado va debajo del indicativo si no entra al lado.
    int an_estado = gfx_ancho_texto(a->estado, 1) + 8;
    if (gfx_ancho_texto(a->vuelo, 1) + 8 + an_estado <= tan)
        pastilla(x + an - 8 - an_estado, cy - 1, a->estado, medio);
    else
        pastilla(tx, cy + 16, a->estado, medio);

    char buf[40];
    acortar(buf, sizeof buf, a->tipo, tan);
    gfx_texto(tx, cy + 32, buf, suave, 1);
    cy += LOGO_LADO + 6;

    // Ruta: origen, barra de avance, destino.
    gfx_texto(px, cy, a->origen, fuerte, 1);
    gfx_texto(px + anu - gfx_ancho_texto(a->destino, 1), cy, a->destino, fuerte, 1);
    const int bx = px + gfx_ancho_texto(a->origen, 1) + 8;
    const int ban = anu - gfx_ancho_texto(a->origen, 1) - gfx_ancho_texto(a->destino, 1) - 16;
    if (ban > 8) {
        gfx_rect_lleno(bx, cy + 6, ban, 3, borde);
        gfx_rect_lleno(bx, cy + 6, ban * a->pct / 100, 3, fuerte);
    }
    cy += 18;

    // Ciudades: origen a la izquierda, destino a la derecha.
    acortar(buf, sizeof buf, a->ciudad_o, anu / 2 - 4);
    gfx_texto(px, cy, buf, suave, 1);
    acortar(buf, sizeof buf, a->ciudad_d, anu / 2 - 4);
    gfx_texto(px + anu - gfx_ancho_texto(buf, 1), cy, buf, suave, 1);
    cy += 18;

    // Horarios, si queda lugar.
    if (cy + 16 <= y + al - 4) {
        snprintf(buf, sizeof buf, "SALE %s", a->dep);
        gfx_texto(px, cy, buf, medio, 1);
        snprintf(buf, sizeof buf, "LLEGA %s", a->arr);
        gfx_texto(px + anu - gfx_ancho_texto(buf, 1), cy, buf, medio, 1);
        cy += 16;
    }

    // Metricas: altura, velocidad y rumbo.
    if (cy + 16 <= y + al - 4) {
        if (a->alt <= 0) snprintf(buf, sizeof buf, "GND");
        else snprintf(buf, sizeof buf, "FL%03d", (int)(a->alt / 100));
        gfx_texto(px, cy, buf, medio, 1);
        snprintf(buf, sizeof buf, "%d kt", a->gs);
        gfx_texto(px + anu / 2 - gfx_ancho_texto(buf, 1) / 2, cy, buf, medio, 1);
        snprintf(buf, sizeof buf, "%03d", a->track);
        gfx_texto(px + anu - gfx_ancho_texto(buf, 1), cy, buf, medio, 1);
    }
}
