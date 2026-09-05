// Patron que demuestra las primitivas de dibujo: texto, lineas, circulos,
// rectangulos y logos. Usa el tema crt_amber de la web (#e8b86d sobre #0a0805).
//
// Todo se dibuja relativo al area util, nunca a 0,0: asi se ve igual en un
// monitor que recorta y en uno que no.
#include "vga.h"
#include "gfx.h"
#include "area.h"
#include "logos.h"
#include "radar.h"
#include "trig.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Mide cuanto tiempo pasa alto cada pin: con esto se vio en su momento que
// los cables estaban en pines muertos. Sirve para saber si la señal sigue
// saliendo bien mientras el radar redibuja.
void demo_init(void);
void demo_aeropuerto(const char *iata);
void demo_avanzar(void);

static uint8_t fondo, ambar, tenue, apagado, blanco;

static void dibujar_patron(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    vga_limpiar(fondo);
    gfx_rect(X, Y, AN, AL, apagado);          // borde del area util

    // --- TEXTO: tres escalas y la fuente completa ---
    gfx_texto_centrado(X + AN / 2, Y + 6, "PICO VGA RADAR", ambar, 2);
    gfx_texto_centrado(X + AN / 2, Y + 36,
                       "primitivas de dibujo en C  -  640x480  -  256 colores", tenue, 1);
    gfx_hlinea(X + 16, Y + 54, AN - 32, apagado);
    gfx_texto(X + 16, Y + 62, "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", ambar, 1);
    gfx_texto(X + 16, Y + 78, "0123456789  !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", tenue, 1);

    // Tres paneles repartidos a lo ancho del area.
    const int py = Y + 96, pal = AL - 96 - 74;
    const int pan = (AN - 32 - 16) / 3;
    const int p0 = X + 16, p1 = p0 + pan + 8, p2 = p1 + pan + 8;

    // --- LINEAS: abanico de 24 rayos, mide los ocho octantes de Bresenham ---
    gfx_rect(p0, py, pan, pal, apagado);
    gfx_texto(p0 + 8, py + 4, "LINEAS", tenue, 1);
    const int lx = p0 + pan / 2, ly = py + pal / 2;
    const int lr = (pan < pal ? pan : pal) / 2 - 20;
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
    gfx_texto(p0 + 8, py + pal - 18, "24 rayos, 8 octantes", apagado, 1);

    // --- CIRCULOS: anillos de alcance y blips, como el radar ---
    gfx_rect(p1, py, pan, pal, apagado);
    gfx_texto(p1 + 8, py + 4, "CIRCULOS", tenue, 1);
    const int cx = p1 + pan / 2, cy = py + pal / 2;
    const int cr = (pan < pal ? pan : pal) / 2 - 20;
    for (int i = 4; i >= 1; i--)
        gfx_circulo(cx, cy, cr * i / 4, i == 4 ? ambar : tenue);
    gfx_linea(cx - cr, cy, cx + cr, cy, apagado);
    gfx_linea(cx, cy - cr, cx, cy + cr, apagado);
    gfx_circulo_lleno(cx, cy, 4, ambar);
    gfx_circulo_lleno(cx + cr * 5 / 8, cy - cr / 2, 5, blanco);
    gfx_circulo_lleno(cx - cr * 3 / 4, cy + cr / 3, 5, blanco);
    gfx_texto(p1 + 8, py + pal - 18, "contorno y relleno", apagado, 1);

    // --- RECTANGULOS: tarjeta de vuelo con logo real, como la de la web ---
    gfx_rect(p2, py, pan, pal, apagado);
    gfx_texto(p2 + 8, py + 4, "RECTANGULOS", tenue, 1);
    const int tx = p2 + 8, ty = py + 24, tan = pan - 16;
    gfx_rect(tx, ty, tan, 86, tenue);
    const uint8_t *logo = logo_buscar("AR");
    if (logo) gfx_blit(tx + 8, ty + 6, LOGO_LADO, LOGO_LADO, logo);
    gfx_texto(tx + 8 + LOGO_LADO + 8, ty + 10, "AR1301", ambar, 1);
    gfx_texto(tx + 8 + LOGO_LADO + 8, ty + 26, "EZE > MAD", tenue, 1);
    gfx_rect(tx + 8, ty + 52, tan - 16, 8, apagado);
    gfx_rect_lleno(tx + 9, ty + 53, (tan - 18) * 62 / 100, 6, ambar);
    gfx_texto(tx + 8, ty + 66, "FL350  842 km/h", tenue, 1);
    for (int i = 0; i < 6; i++)
        gfx_rect(tx + i * 6, ty + 104 + i * 6, tan - i * 12,
                 pal - 128 - i * 12, (i & 1) ? tenue : apagado);
    gfx_texto(p2 + 8, py + pal - 18, "contorno y relleno", apagado, 1);

    // --- LOGOS: reales, sacados de la flash ---
    const int ly2 = py + pal + 6;
    gfx_texto(X + 16, ly2 + 10, "LOGOS", tenue, 1);
    static const char *muestra[] = {"AR", "LA", "AA", "IB", "JL", "NH",
                                    "AF", "BA", "UA", "DL", "EK", "QF"};
    const int lx0 = X + 66, paso = (AN - 82) / 12;
    for (int i = 0; i < 12; i++) {
        const uint8_t *l = logo_buscar(muestra[i]);
        if (l) gfx_blit(lx0 + i * paso, ly2, LOGO_LADO, LOGO_LADO, l);
    }

    // --- RAMPA: el degradado con dither, el que se ve parejo ---
    const int ry = ly2 + LOGO_LADO + 6;
    gfx_texto(X + 16, ry + 2, "RAMPA", tenue, 1);
    const int ran = AN - 82 - 70;
    for (int i = 0; i < ran; i++) {
        int n = 255 * i / (ran - 1);
        gfx_rect_dither(lx0 + i, ry, 1, 14,
                        0xe8 * n / 255, 0xb8 * n / 255, 0x6d * n / 255);
    }
    gfx_texto(X + AN - 16 - gfx_ancho_texto("824 logos", 1), ry + 2, "824 logos", apagado, 1);
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

    // Leido de la pantalla de calibracion en el ViewSonic VA1703wb: recorta
    // apenas el borde, 1 a 3 px. Con 4 por lado sobra y no se pierde nada.
    area_set(4, 4, 4, 4);
    printf("area util: %dx%d en %d,%d\n", area.an, area.al, area.x, area.y);

    // Arranca mostrando el patron de primitivas unos segundos y despues se
    // queda en el radar, que es lo que el equipo muestra de verdad.
    dibujar_patron();
    sleep_ms(6000);

    radar_init();
    demo_init();
    { void demo_casa(void); demo_casa(); }
    printf("radar andando\n");

    // Recorrido por todas las vistas y alcances, para poder revisarlas. Cada
    // escena dura unos segundos y avisa por consola cual esta mostrando.
    static const struct {
        const char *nombre; vista_t vista; lista_t lista;
        int tarjetas, radio; const char *apt, *seguir, *tema;
    } ESCENA[] = {
        { "hibrida 3 tarjetas, 150 km", VISTA_HIBRIDA, LISTA_TARJETAS, 3, 150, "EZE", "", "crt_amber" },
        { "hibrida 4 tarjetas",         VISTA_HIBRIDA, LISTA_TARJETAS, 4, 150, "EZE", "", "crt_amber" },
        { "formato aeropuerto (FIDS)",  VISTA_HIBRIDA, LISTA_FIDS,     3, 150, "EZE", "", "crt_amber" },
        { "solo radar, 80 km",          VISTA_RADAR,   LISTA_TARJETAS, 3,  80, "EZE", "", "crt_amber" },
        { "solo radar, 20 km",          VISTA_RADAR,   LISTA_TARJETAS, 3,  20, "EZE", "", "crt_amber" },
        { "pared de tarjetas",          VISTA_PARED,   LISTA_TARJETAS, 3, 150, "EZE", "", "crt_amber" },
        { "una sola tarjeta grande",    VISTA_PARED,   LISTA_TARJETAS, 1, 150, "EZE", "", "crt_amber" },
        { "pared formato aeropuerto",   VISTA_PARED,   LISTA_FIDS,     3, 150, "EZE", "", "crt_amber" },
        { "seguir QF17, solo mapa",     VISTA_SEGUIR,  LISTA_TARJETAS, 1, 150, "EZE", "QF17", "crt_amber" },
        { "seguir QF17 con tarjeta",    VISTA_SEGUIR_HIBRIDA, LISTA_TARJETAS, 1, 150, "EZE", "QF17", "crt_amber" },
        { "seguir AA954, mapa America", VISTA_SEGUIR,  LISTA_TARJETAS, 1, 150, "EZE", "AA954", "crt_amber" },
        { "tema verde",                 VISTA_HIBRIDA, LISTA_TARJETAS, 3, 150, "EZE", "", "crt_green" },
        { "tema atc azul",              VISTA_HIBRIDA, LISTA_TARJETAS, 3, 150, "EZE", "", "atc_dark" },
        { "tema hielo",                 VISTA_RADAR,   LISTA_TARJETAS, 3,  80, "EZE", "", "ice" },
        { "Madrid",                     VISTA_HIBRIDA, LISTA_TARJETAS, 3, 150, "MAD", "", "crt_amber" },
        { "Narita",                     VISTA_HIBRIDA, LISTA_TARJETAS, 3,  80, "NRT", "", "crt_amber" },
        { "pantalla de logos",          VISTA_LOGOS,   LISTA_TARJETAS, 3, 150, "EZE", "", "crt_amber" },
    };
    const int ESCENAS = sizeof ESCENA / sizeof ESCENA[0];

    int esc = 10;   // PRUEBA: mapa Miami-Ezeiza
    uint32_t desde = time_us_32();
    char apt_actual[4] = "";
    for (;;) {
        if (strncmp(apt_actual, ESCENA[esc].apt, 3)) {
            snprintf(apt_actual, sizeof apt_actual, "%s", ESCENA[esc].apt);
            demo_aeropuerto(apt_actual);
        }
        radar_vista = ESCENA[esc].vista;
        radar_lista = ESCENA[esc].lista;
        radar_tema_poner(ESCENA[esc].tema);
        radar_marcar_sucio();
        { void radar_viaje_rehacer(void); radar_viaje_rehacer();
          void logos_pantalla_rehacer(void); logos_pantalla_rehacer(); }
        radar_tarjetas = ESCENA[esc].tarjetas;
        radar_apt.radio_km = ESCENA[esc].radio;
        snprintf(radar_seguir, sizeof radar_seguir, "%s", ESCENA[esc].seguir);
        printf("ESCENA %d: %s\n", esc, ESCENA[esc].nombre);

        uint32_t cuadros = 0, us_total = 0, us_peor = 0;
        while (time_us_32() - desde < 15000000u) {
            vga_esperar_cuadro();
            uint32_t t0 = time_us_32();
            demo_avanzar();
            radar_avanzar();
            radar_cuadro();
            uint32_t d = time_us_32() - t0;
            us_total += d;
            if (d > us_peor) us_peor = d;
            cuadros++;
        }
        // El haz tarda 15200 us en bajar la pantalla: si el dibujo se pasa de
        // ahi, lo alcanza y la imagen titila.
        printf("  %lu cuadros, %lu por segundo | dibujo %lu us promedio, %lu us el peor%s\n",
               (unsigned long)cuadros, (unsigned long)(cuadros / 15),
               (unsigned long)(cuadros ? us_total / cuadros : 0), (unsigned long)us_peor,
               us_peor > 15200 ? "  <-- SE PASA" : "");
        {
            extern uint32_t radar_us_banda[];
            printf("    por banda (us):");
            for (int b = 0; b < 10; b++) printf(" %lu", (unsigned long)radar_us_banda[b]);
            printf("   | el haz tarda 1520 us por banda\n");
        }
        desde = time_us_32();
        esc = (esc + 1) % ESCENAS;
    }
}
