// Prueba el camino entero de los datos reales, en la Mac y sin la placa:
// texto del proxy -> sky_parse -> vivo.c -> las tarjetas como se van a ver.
//
// Es la unica forma de saber que el formato del proxy y lo que espera el
// firmware siguen coincidiendo sin tener que cargar el uf2 y mirar el
// monitor. Si alguien toca sky-proxy/api/pico.js, esto tiene que seguir
// dando bien.
//
// Se corre con herramientas/probar_sky.sh.
#include "../firmware-c/radar.h"
#include "../firmware-c/sky_parse.h"
#include "../firmware-c/vivo.h"
#include "../firmware-c/trig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- lo que en la placa pone radar.c --------------------------------------
aeropuerto_t radar_apt;
avion_t      radar_aviones[RADAR_MAX_AVIONES];
int          radar_cantidad;
void radar_marcar_sucio(void) {}

// --- lo que en la placa pone sky.c ----------------------------------------
static int hora_de_prueba = -1;
int sky_hora_local(void) { return hora_de_prueba; }

// vivo.c lee el lote con esto; aca se lo damos ya parseado de un archivo.
static sky_avion_t pendientes[SKY_MAX];
static int pendientes_n = -1;
int sky_tomar(sky_avion_t *destino, int tope) {
    if (pendientes_n < 0) return -1;
    int n = pendientes_n < tope ? pendientes_n : tope;
    memcpy(destino, pendientes, sizeof(sky_avion_t) * n);
    pendientes_n = -1;
    return n;
}

static int fallos;
static void mal(const char *que) { printf("  MAL: %s\n", que); fallos++; }

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "uso: probar_sky <archivo con la respuesta del proxy>\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    static char texto[65536];
    size_t leido = fread(texto, 1, sizeof texto - 1, f);
    texto[leido] = 0;
    fclose(f);

    // Sin esto la tabla de senos queda en cero y todas las distancias
    // este-oeste dan cero. En la placa lo llama radar_init().
    trig_init();

    // El radar mirando Ezeiza, que es la pantalla de ejemplo.
    snprintf(radar_apt.iata, sizeof radar_apt.iata, "EZE");
    radar_apt.lat = -348220;
    radar_apt.lon = -585360;
    radar_apt.radio_km = 220;

    printf("== parseo ==\n");
    pendientes_n = sky_parse(texto, pendientes, SKY_MAX, &hora_de_prueba);
    printf("%d aviones, hora local %02d:%02d\n",
           pendientes_n, hora_de_prueba / 60, hora_de_prueba % 60);
    if (pendientes_n <= 0) mal("no salio ni un avion");
    if (hora_de_prueba < 0) mal("el proxy no mando la hora");

    printf("\n== tarjetas ==\n");
    vivo_avanzar();
    printf("%d aviones en el radar\n\n", radar_cantidad);
    if (radar_cantidad <= 0) mal("el radar quedo vacio");

    int con_ruta = 0, con_logo = 0, con_horario = 0;
    for (int i = 0; i < radar_cantidad; i++) {
        const avion_t *a = &radar_aviones[i];
        printf("%-8s %-5s %-4s %5ld ft %3d kt %3d deg | %-3s %-18s > %-3s %-18s | "
               "%3d%% %s-%s falta %3d min | %4ld km | %s\n",
               a->vuelo, a->aerolinea[0] ? a->aerolinea : "--", a->tipo,
               (long)a->alt, a->gs, a->track,
               a->origen[0] ? a->origen : "---", a->ciudad_o,
               a->destino[0] ? a->destino : "---", a->ciudad_d,
               a->pct, a->dep[0] ? a->dep : "--:--", a->arr[0] ? a->arr : "--:--",
               a->falta_min, (long)a->dist_km, a->estado);

        if (a->origen[0] && a->destino[0]) con_ruta++;
        if (a->aerolinea[0]) con_logo++;
        if (a->arr[0]) con_horario++;

        // Lo que no puede pasar pase lo que pase.
        if (a->pct > 100) mal("un avance mayor a 100%");
        if (a->dist_km < 0 || a->dist_km > 25000) mal("una distancia imposible");
        if (a->falta_min < 0) mal("faltan minutos negativos");
        if (!a->estado[0]) mal("un avion sin estado");
        if (a->demora != 0) mal("una demora que no puede estar calculada");
    }

    printf("\n== resumen ==\n");
    printf("con ruta:    %d de %d\n", con_ruta, radar_cantidad);
    printf("con logo:    %d de %d\n", con_logo, radar_cantidad);
    printf("con horario: %d de %d\n", con_horario, radar_cantidad);
    if (!con_ruta) mal("ningun vuelo trajo origen y destino");

    printf("\n%s\n", fallos ? "HAY FALLAS" : "todo bien");
    return fallos ? 1 : 0;
}
