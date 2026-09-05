// Tarjetas de vuelo: la columna que va al costado del scope, y la pared
// completa cuando el radar no se muestra.
//
// Es la traduccion de cardHTML() y .fa-card de la web. El contenido se reparte
// en el alto disponible en vez de quedar apelmazado arriba, y cuando la
// tarjeta es baja se van sacando filas en el orden de SHRINK_ORDER de la web:
// primero los extras, despues las ciudades, despues los horarios.
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
        char prueba[48];
        snprintf(prueba, sizeof prueba, "%s...", dst);
        if (gfx_ancho_texto(prueba, 1) <= ancho_px) { snprintf(dst, n, "%s", prueba); return; }
    }
}

// La pastilla de estado: marco fino con el texto adentro, como .pill.
static void pastilla(int x, int y, const char *txt, uint8_t c) {
    int an = gfx_ancho_texto(txt, 1) + 10;
    gfx_rect(x, y, an, 18, c);
    gfx_texto(x + 5, y + 2, txt, c, 1);
}

// El cuadro del logo. Si esa aerolinea no tiene, va su codigo en un recuadro,
// que es lo que hace la web cuando no encuentra la imagen.
static void cuadro_logo(int x, int y, int lado, const avion_t *a, uint8_t c) {
    const uint8_t *logo = logo_buscar(a->aerolinea);
    if (logo && lado == LOGO_LADO) { gfx_blit(x, y, LOGO_LADO, LOGO_LADO, logo); return; }
    gfx_rect(x, y, lado, lado, c);
    char cod[4] = {0};
    strncpy(cod, a->aerolinea, 2);
    if (!cod[0]) strncpy(cod, a->vuelo, 2);
    gfx_texto(x + lado / 2 - gfx_ancho_texto(cod, 1) / 2,
              y + lado / 2 - GFX_FUENTE_ALTO / 2, cod, c, 1);
}

void tarjeta_dibujar(int x, int y, int an, int al, const avion_t *a, int grande) {
    const uint8_t borde  = radar_tono(60);
    const uint8_t fuerte = radar_tono(255);
    const uint8_t medio  = radar_tono(190);
    const uint8_t suave  = radar_tono(150);

    gfx_rect(x, y, an, al, borde);

    const int px = x + 10;
    const int anu = an - 20;
    // Cuanto mas alta la tarjeta, mas grande el texto: una sola tarjeta a
    // pantalla completa se ve como el .hero de la web, no como una chica
    // estirada con huecos.
    const int esc = (grande && al > 300) ? 3 : (grande || al > 200) ? 2 : 1;
    const int esc2 = esc >= 3 ? 2 : 1;     // para las filas de datos
    const int fila = GFX_FUENTE_ALTO * esc + 4;

    char buf[48];
    int cy = y + 6;

    // --- Cabecera: logo, indicativo y estado ---
    const int lado = LOGO_LADO;   // el logo esta guardado a 36
    cuadro_logo(px, cy, lado, a, medio);
    const int tx = px + lado + 10;
    int tan = anu - lado - 10;

    // El estado va a la derecha si entra en la misma linea; si no, debajo.
    const int an_pill = gfx_ancho_texto(a->estado, 1) + 10;
    const int an_id = gfx_ancho_texto(a->vuelo, esc);
    int pill_al_lado = (an_id + 10 + an_pill <= tan);
    gfx_texto(tx, cy + 2, a->vuelo, fuerte, esc);
    if (pill_al_lado)
        pastilla(x + an - 10 - an_pill, cy + 2, a->estado, medio);
    else
        pastilla(tx, cy + 2 + fila, a->estado, medio);

    // El tipo va al lado del indicativo si hay ancho, y debajo si no. Al
    // lado la cabecera es una linea mas baja y entra una fila mas de datos,
    // que es lo que se pierde en la pared con el indicativo al doble.
    acortar(buf, sizeof buf, a->tipo, tan);
    const int an_tipo = gfx_ancho_texto(buf, 1);
    const int tipo_al_lado = pill_al_lado && (an_id + 10 + an_tipo + 10 + an_pill <= tan);
    int alto_cab;
    if (tipo_al_lado) {
        gfx_texto(tx + an_id + 10, cy + 2 + (GFX_FUENTE_ALTO * esc - GFX_FUENTE_ALTO) / 2,
                  buf, suave, 1);
        alto_cab = 2 + GFX_FUENTE_ALTO * esc;
    } else {
        const int y_tipo = cy + 2 + (pill_al_lado ? fila : 2 * fila + 4);
        gfx_texto(tx, y_tipo, buf, suave, 1);
        alto_cab = y_tipo + GFX_FUENTE_ALTO - cy;
    }
    cy += (alto_cab > lado ? alto_cab : lado) + 8;

    // --- Cuerpo ---
    // Primero se ve cuantas filas entran, despues se reparte el sobrante
    // entre todas por igual: asi la tarjeta queda llena y no con el
    // contenido apelmazado arriba y un hueco abajo.
    const int y_fin = y + al - 6;
    const int alto_ruta = GFX_FUENTE_ALTO * esc;
    const int alto_fila = GFX_FUENTE_ALTO * esc2;
    int filas = 1;                                   // la ruta va siempre
    int usado = alto_ruta;
    while (filas < 4 && cy + usado + 2 + alto_fila <= y_fin) { usado += alto_fila + 2; filas++; }
    // El sobrante se reparte entre las filas, pero con un tope: en una
    // tarjeta muy alta, repartir todo dejaba las filas desparramadas con
    // huecos enormes en el medio.
    const int sobra = y_fin - cy - usado;
    int sep = filas > 1 ? sobra / (filas - 1) : 0;
    const int sep_max = GFX_FUENTE_ALTO * esc2 + 10;
    if (sep > sep_max) sep = sep_max;

    // Ruta con la barra de avance.
    gfx_texto(px, cy, a->origen, fuerte, esc);
    gfx_texto(px + anu - gfx_ancho_texto(a->destino, esc), cy, a->destino, fuerte, esc);
    const int bx = px + gfx_ancho_texto(a->origen, esc) + 10;
    const int ban = anu - gfx_ancho_texto(a->origen, esc) - gfx_ancho_texto(a->destino, esc) - 20;
    if (ban > 8) {
        const int by = cy + alto_ruta / 2 - 1;
        gfx_rect_lleno(bx, by, ban, 3, borde);
        gfx_rect_lleno(bx, by, ban * a->pct / 100, 3, fuerte);
    }
    cy += alto_ruta + sep;

    // Ciudades.
    if (filas >= 2) {
        acortar(buf, sizeof buf, a->ciudad_o, (anu / 2 - 6) / esc2);
        gfx_texto(px, cy, buf, suave, esc2);
        acortar(buf, sizeof buf, a->ciudad_d, (anu / 2 - 6) / esc2);
        gfx_texto(px + anu - gfx_ancho_texto(buf, esc2), cy, buf, suave, esc2);
        cy += alto_fila + sep;
    }

    // Horarios.
    if (filas >= 3) {
        snprintf(buf, sizeof buf, "SALE %s", a->dep);
        gfx_texto(px, cy, buf, medio, esc2);
        snprintf(buf, sizeof buf, "LLEGA %s", a->arr);
        gfx_texto(px + anu - gfx_ancho_texto(buf, esc2), cy, buf, medio, esc2);
        cy += alto_fila + sep;
    }

    // Metricas: altura, velocidad y rumbo.
    if (filas >= 4) {
        if (a->alt <= 0) snprintf(buf, sizeof buf, "ALT GND");
        else snprintf(buf, sizeof buf, "ALT FL%03d", (int)(a->alt / 100));
        gfx_texto(px, cy, buf, medio, esc2);
        snprintf(buf, sizeof buf, "VEL %d kt", a->gs);
        gfx_texto(px + anu / 2 - gfx_ancho_texto(buf, esc2) / 2, cy, buf, medio, esc2);
        snprintf(buf, sizeof buf, "RUMBO %03d", a->track);
        gfx_texto(px + anu - gfx_ancho_texto(buf, esc2), cy, buf, medio, esc2);
    }
}

// --- Formato aeropuerto -------------------------------------------------
// La tabla de llegadas, como listStyle "fids" en la web: una fila por vuelo
// con logo, indicativo, ruta, horarios y estado. Es lo que se ve en las
// pantallas de un aeropuerto de verdad.
void fids_dibujar(int x, int y, int an, int al, const avion_t **vuelos, int n) {
    const uint8_t borde  = radar_tono(60);
    const uint8_t fuerte = radar_tono(255);
    const uint8_t medio  = radar_tono(190);
    const uint8_t suave  = radar_tono(150);

    // En la columna angosta van menos columnas, igual que la web con
    // compact: sin horario de salida y sin ruta larga.
    const int compacto = (an < 380);

    const int c_logo  = x + 6;
    const int c_vuelo = c_logo + 30;
    const int c_ruta  = c_vuelo + (compacto ? 62 : an * 14 / 100);
    const int c_dep   = c_ruta + (compacto ? 78 : an * 24 / 100);
    const int c_arr   = compacto ? c_dep : c_dep + an * 10 / 100;
    const int c_est   = compacto ? c_dep + 46 : c_arr + an * 10 / 100;

    gfx_texto(c_vuelo, y, "VUELO", suave, 1);
    gfx_texto(c_ruta,  y, "RUTA",  suave, 1);
    if (!compacto) gfx_texto(c_dep, y, "SALE", suave, 1);
    gfx_texto(c_arr, y, "LLEGA", suave, 1);
    gfx_texto(c_est, y, "ESTADO", suave, 1);
    gfx_hlinea(x, y + 16, an, borde);

    const int alto_fila = (al - 22) / (n > 0 ? n : 1);
    const int fila = alto_fila > 30 ? 30 : alto_fila;

    char buf[48];
    for (int i = 0; i < n; i++) {
        const avion_t *a = vuelos[i];
        const int fy = y + 22 + i * alto_fila;
        if (fy + fila < gfx_banda_y0 || fy > gfx_banda_y1) continue;
        const int ty = fy + (fila - GFX_FUENTE_ALTO) / 2;

        // El logo esta guardado a 36 y aca se muestra achicado salteando
        // pixeles: a este tamano no se nota y no hace falta otra copia.
        const uint8_t *logo = logo_buscar(a->aerolinea);
        const int lado = fila - 4 < 24 ? fila - 4 : 24;
        if (logo && lado > 6) {
            for (int j = 0; j < lado; j++)
                for (int k = 0; k < lado; k++)
                    gfx_punto(c_logo + k, fy + 2 + j,
                              logo[(j * LOGO_LADO / lado) * LOGO_LADO + (k * LOGO_LADO / lado)]);
        }

        gfx_texto(c_vuelo, ty, a->vuelo, fuerte, 1);
        snprintf(buf, sizeof buf, "%s>%s", a->origen, a->destino);
        gfx_texto(c_ruta, ty, buf, medio, 1);
        if (!compacto) gfx_texto(c_dep, ty, a->dep, medio, 1);
        gfx_texto(c_arr, ty, a->arr, medio, 1);

        // El estado abreviado, como shortSt() en la web.
        const char *est = a->estado;
        if (!strncmp(est, "APROXIMANDO", 11)) est = "APROX";
        else if (!strncmp(est, "EN TIERRA", 9)) est = "TIERRA";
        else if (!strncmp(est, "EN VUELO", 8)) est = "VUELO";
        gfx_texto(c_est, ty, est, fuerte, 1);

        if (i + 1 < n) gfx_hlinea(x, fy + alto_fila - 1, an, radar_tono(28));
    }
}
