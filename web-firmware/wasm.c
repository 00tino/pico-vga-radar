// Lo que el navegador le pide al firmware.
//
// Esto es lo que hace que la vista previa de la web sea una replica y no una
// imitacion: la pagina no vuelve a dibujar nada por su cuenta, le pide los
// cuadros al mismo codigo en C que corre en la placa. Si manana se cambia
// como se dibuja una tarjeta, la web cambia sola.
#include "vga.h"
#include "gfx.h"
#include "area.h"
#include "radar.h"
#include "pantallas.h"
#include <emscripten/emscripten.h>
#include <string.h>
#include <stdio.h>

void demo_init(void);
void demo_casa(void);
void demo_aeropuerto(const char *iata);
void demo_avanzar(void);
void vga_web_nuevo_cuadro(void);
void radar_viaje_rehacer(void);
void logos_pantalla_rehacer(void);

// El framebuffer pasado a RGB, que es lo que el canvas sabe pintar. Se arma
// con la misma cuenta que hacen las resistencias del cable VGA.
static uint8_t rgba[VGA_ANCHO * VGA_ALTO * 4];

EMSCRIPTEN_KEEPALIVE
void radar_web_init(int margen_arriba, int margen_abajo,
                    int margen_izq, int margen_der) {
    vga_init();
    area_set(margen_arriba, margen_abajo, margen_izq, margen_der);
    radar_init();
    demo_init();
    demo_casa();
}

// Arma una pantalla. La web las manda de a una y despues llama a
// radar_web_pantallas_listas().
EMSCRIPTEN_KEEPALIVE
void radar_web_pantalla(int i, int vista, int lista, const char *apt,
                        int radio_km, int tarjetas, const char *tema,
                        const char *seguir, int segundos) {
    if (i < 0 || i >= PANTALLAS_MAX) return;
    pantalla_t *p = &pantallas[i];
    memset(p, 0, sizeof *p);
    snprintf(p->nombre, sizeof p->nombre, "pantalla %d", i + 1);
    p->vista = (vista_t)vista;
    p->lista = (lista_t)lista;
    snprintf(p->apt, sizeof p->apt, "%s", apt ? apt : "EZE");
    p->radio_km = radio_km;
    p->tarjetas = tarjetas;
    snprintf(p->tema, sizeof p->tema, "%s", tema ? tema : "crt_amber");
    snprintf(p->seguir, sizeof p->seguir, "%s", seguir ? seguir : "");
    p->segundos = segundos;
    if (i + 1 > pantallas_n) pantallas_n = i + 1;
}

EMSCRIPTEN_KEEPALIVE
void radar_web_pantallas_listas(int cuantas) {
    pantallas_n = cuantas < 1 ? 1 : (cuantas > PANTALLAS_MAX ? PANTALLAS_MAX : cuantas);
    pantallas_init();
}

// La casa, igual que la configura el cliente desde la web.
EMSCRIPTEN_KEEPALIVE
void radar_web_casa(int on, int lat, int lon, int km) {
    radar_casa_on = on ? true : false;
    radar_casa_lat = lat;
    radar_casa_lon = lon;
    radar_casa_km = km < 1 ? 1 : km;
}

// Un cuadro. Devuelve el framebuffer ya en RGBA para el canvas.
EMSCRIPTEN_KEEPALIVE
uint8_t *radar_web_cuadro(void) {
    vga_web_nuevo_cuadro();
    demo_avanzar();
    radar_avanzar();
    pantallas_avanzar();
    radar_cuadro();

    for (int i = 0; i < VGA_ANCHO * VGA_ALTO; i++) {
        const uint8_t v = vga_fb[i];
        // RRRGGGBB, expandido igual que lo hace el divisor de resistencias.
        rgba[i * 4 + 0] = (uint8_t)(((v >> 5) & 7) * 255 / 7);
        rgba[i * 4 + 1] = (uint8_t)(((v >> 2) & 7) * 255 / 7);
        rgba[i * 4 + 2] = (uint8_t)((v & 3) * 255 / 3);
        rgba[i * 4 + 3] = 255;
    }
    return rgba;
}

EMSCRIPTEN_KEEPALIVE int radar_web_ancho(void) { return VGA_ANCHO; }
EMSCRIPTEN_KEEPALIVE int radar_web_alto(void)  { return VGA_ALTO; }
EMSCRIPTEN_KEEPALIVE int radar_web_pantalla_actual(void) { return pantallas_actual(); }
