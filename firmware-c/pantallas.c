// El equipo va rotando entre las pantallas que armo el cliente: por ejemplo
// los vuelos de Ezeiza cinco segundos, despues un vuelo en particular otros
// cinco, y vuelta a empezar.
//
// Lo que hace que esto se sienta bien es que **la lista no vuelve a empezar**
// cuando se vuelve a una pantalla. Si hay veinte vuelos y entran cuatro por
// vez, la primera visita muestra del 1 al 4, la siguiente del 5 al 8, y asi
// hasta completar los veinte. Reiniciando el carrusel se verian siempre los
// mismos cuatro y los otros dieciseis no se verian nunca.
//
// Por eso cada pantalla guarda su propio lugar en el carrusel, y ese lugar
// solo avanza mientras esa pantalla esta a la vista.
//
// Con una sola pantalla configurada esto sigue funcionando igual: rota entre
// los vuelos como siempre, porque nunca deja de estar a la vista.
#include "pantallas.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

void demo_aeropuerto(const char *iata);
void radar_viaje_rehacer(void);
void logos_pantalla_rehacer(void);

pantalla_t pantallas[PANTALLAS_MAX];
int        pantallas_n = 0;

static int      actual = 0;
static uint32_t cuadros = 0;

// El lugar del carrusel de cada pantalla.
static int      pag_de[PANTALLAS_MAX];
static uint32_t cuadros_de[PANTALLAS_MAX];

static char apt_puesto[4];

static void aplicar(int i) {
    const pantalla_t *p = &pantallas[i];
    if (strncmp(apt_puesto, p->apt, 3)) {
        snprintf(apt_puesto, sizeof apt_puesto, "%s", p->apt);
        demo_aeropuerto(apt_puesto);
    }
    radar_vista = p->vista;
    radar_lista = p->lista;
    radar_tarjetas = p->tarjetas;
    radar_apt.radio_km = p->radio_km;
    radar_tema_poner(p->tema);
    snprintf(radar_seguir, sizeof radar_seguir, "%s", p->seguir);
    radar_marcar_sucio();
    radar_viaje_rehacer();
    logos_pantalla_rehacer();
    // Repone el carrusel donde lo habia dejado esta pantalla.
    radar_carrusel_poner(pag_de[i], cuadros_de[i]);
    printf("pantalla %d: %s (%d s) | grupo %d de la lista\n",
           i, p->nombre, p->segundos, radar_pagina_actual());
}

void pantallas_init(void) {
    actual = 0;
    cuadros = 0;
    memset(pag_de, 0, sizeof pag_de);
    memset(cuadros_de, 0, sizeof cuadros_de);
    apt_puesto[0] = 0;
    if (pantallas_n > 0) aplicar(0);
}

void pantallas_avanzar(void) {
    if (pantallas_n <= 1) return;    // con una sola no hay nada que rotar
    const int segs = pantallas[actual].segundos > 0 ? pantallas[actual].segundos : 10;
    if (++cuadros < (uint32_t)(segs * 60)) return;
    cuadros = 0;
    // Se guarda donde quedo el carrusel de esta, para retomarlo al volver.
    radar_carrusel_guardar(&pag_de[actual], &cuadros_de[actual]);
    actual = (actual + 1) % pantallas_n;
    aplicar(actual);
}

int pantallas_actual(void) { return actual; }

// Salta ya mismo a la siguiente, sin esperar el tiempo. Para revisar de a una
// desde la consola.
void pantallas_forzar_siguiente(void) {
    if (pantallas_n <= 0) return;
    cuadros = 0;
    radar_carrusel_guardar(&pag_de[actual], &cuadros_de[actual]);
    actual = (actual + 1) % pantallas_n;
    aplicar(actual);
}
