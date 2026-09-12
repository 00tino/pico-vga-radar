// Pasa el lote que trae el nucleo 1 a los aviones que dibuja el radar.
//
// El proxy manda lo poco que ADS-B sabe de verdad: quien es, donde esta, a
// que altura, a que velocidad, para donde apunta, y de donde sale y a donde
// va. Todo lo demas de la tarjeta se arma aca, porque es aca donde estan los
// 5334 aeropuertos con sus coordenadas y los 824 logos.
//
// Lo que se calcula y lo que no:
//
//   - La ciudad de origen y de destino salen de la tabla de aeropuertos.
//   - El avance del vuelo es la parte del camino que ya hizo, medida entre
//     el aeropuerto de salida y el de llegada.
//   - Lo que falta para llegar sale de la distancia al destino y de la
//     velocidad que lleva en este momento.
//   - La demora queda SIEMPRE en cero. Para saber si un vuelo esta atrasado
//     hace falta el horario que publico la aerolinea, y eso ADS-B no lo
//     manda: no hay con que compararlo. El dia que haya una fuente de
//     horarios, es el unico numero que hay que llenar aca.
#include "radar.h"
#include "sky.h"
#include "geo.h"
#include "aeropuertos.h"
#include "aerolineas.h"
#include <string.h>
#include <stdio.h>

// Cuanto se banca un avion sin que el proxy lo vuelva a nombrar antes de
// borrarlo de la pantalla. Un lote llega cada veinte segundos y las fuentes
// pierden aviones de a ratos: sacarlos al primer lote que no los trae haria
// que parpadeen.
#define OLVIDO_LOTES 3

static uint8_t faltazos[RADAR_MAX_AVIONES];   // lotes seguidos sin aparecer
static char    hexes[RADAR_MAX_AVIONES][8];   // quien es cada lugar

static bool vivo_andando;

// Kilometros entre dos aeropuertos por codigo IATA. 0 si falta alguno.
static int km_entre(const char *a, const char *b, int32_t *lat_b, int32_t *lon_b) {
    const aeropuerto_dato_t *pa = a[0] ? aeropuerto_buscar(a) : 0;
    const aeropuerto_dato_t *pb = b[0] ? aeropuerto_buscar(b) : 0;
    if (lat_b && pb) { *lat_b = pb->lat; *lon_b = pb->lon; }
    if (!pa || !pb) return 0;
    return geo_km(pa->lat, pa->lon, pb->lat, pb->lon);
}

static void ciudad_de(const char *iata, char *destino, int tam) {
    const aeropuerto_dato_t *p = iata[0] ? aeropuerto_buscar(iata) : 0;
    snprintf(destino, tam, "%s", p ? p->ciudad : "");
}

// Minutos desde medianoche a "hh:mm". Da la vuelta al dia sola: la hora de
// salida de un vuelo largo cae ANTES de medianoche, o sea en negativo, y un
// vuelo que llega despues de las doce se pasa de 1440. Tratar el negativo
// como "no se sabe" era lo que dejaba sin hora de salida a todos los vuelos
// de mas de unas horas: Roma, Miami, Madrid, ninguno mostraba a que hora
// habia salido.
static void reloj(int minutos, char *destino, int tam) {
    minutos %= 1440;
    if (minutos < 0) minutos += 1440;
    snprintf(destino, tam, "%02d:%02d", minutos / 60, minutos % 60);
}

// Busca en que lugar del radar esta ese avion, o consigue uno libre.
static int lugar_de(const char *hex, int usados) {
    for (int i = 0; i < usados; i++)
        if (!strcmp(hexes[i], hex)) return i;
    return -1;
}

// Arma un avion del radar con lo que vino del proxy. Si ya estaba, le
// conserva el rastro y el brillo del barrido.
static void componer(avion_t *a, const sky_avion_t *s, bool es_nuevo) {
    const bool se_movio = !es_nuevo && (a->lat != s->lat || a->lon != s->lon);

    if (se_movio) {
        // El rastro es una cola: entra la posicion de antes y sale la mas
        // vieja. Es lo mismo que hace TRAIL en la web.
        if (a->rastro_n < RADAR_RASTRO) {
            a->rastro_lat[a->rastro_n] = a->lat;
            a->rastro_lon[a->rastro_n] = a->lon;
            a->rastro_n++;
        } else {
            memmove(a->rastro_lat, a->rastro_lat + 1, sizeof(int32_t) * (RADAR_RASTRO - 1));
            memmove(a->rastro_lon, a->rastro_lon + 1, sizeof(int32_t) * (RADAR_RASTRO - 1));
            a->rastro_lat[RADAR_RASTRO - 1] = a->lat;
            a->rastro_lon[RADAR_RASTRO - 1] = a->lon;
        }
    }

    a->lat = s->lat;
    a->lon = s->lon;
    a->alt = s->alt;
    a->gs  = s->gs;
    // Parado en plataforma no manda rumbo. Dejarle el ultimo conocido evita
    // que el dibujo lo haga girar al norte de golpe.
    if (s->track >= 0) a->track = s->track;

    snprintf(a->vuelo, sizeof a->vuelo, "%s", s->vuelo[0] ? s->vuelo : s->matricula);
    snprintf(a->tipo,  sizeof a->tipo,  "%s", s->tipo);
    snprintf(a->origen,  sizeof a->origen,  "%s", s->origen);
    snprintf(a->destino, sizeof a->destino, "%s", s->destino);

    const char *iata = aerolinea_de_indicativo(s->vuelo);
    memset(a->aerolinea, 0, sizeof a->aerolinea);
    if (iata) memcpy(a->aerolinea, iata, 2);

    ciudad_de(a->origen,  a->ciudad_o, sizeof a->ciudad_o);
    ciudad_de(a->destino, a->ciudad_d, sizeof a->ciudad_d);

    a->dist_km = geo_km(radar_apt.lat, radar_apt.lon, a->lat, a->lon);

    // --- el camino ---
    int32_t lat_d = 0, lon_d = 0;
    const int total_km = km_entre(a->origen, a->destino, &lat_d, &lon_d);
    const int falta_km = lon_d || lat_d ? geo_km(a->lat, a->lon, lat_d, lon_d) : 0;

    if (total_km > 0 && falta_km >= 0) {
        int hecho = total_km - falta_km;
        if (hecho < 0) hecho = 0;
        if (hecho > total_km) hecho = total_km;
        a->pct = (uint8_t)(hecho * 100 / total_km);
    } else {
        a->pct = 0;
    }

    // Velocidad en km/h. Abajo de cien nudos esta rodando o parado, y
    // dividir por eso da horas de vuelo que no significan nada.
    const int kmh = s->gs > 100 ? s->gs * 1852 / 1000 : 0;
    a->falta_min = (int16_t)(kmh && falta_km ? falta_km * 60 / kmh : 0);
    a->vuelo_min = (int16_t)(kmh && total_km ? total_km * 60 / kmh : 0);

    // --- horarios ---
    // Los dos salen de la hora de llegada estimada: no son los que publica la
    // aerolinea, son los que se deducen de donde esta el avion ahora.
    const int ahora = sky_hora_local();
    a->arr[0] = 0;
    a->dep[0] = 0;
    if (ahora >= 0 && a->falta_min > 0) {
        reloj(ahora + a->falta_min, a->arr, sizeof a->arr);
        // La salida solo si se sabe cuanto dura el vuelo entero, que sale de
        // la distancia entre los dos aeropuertos. Sin ruta conocida, poner la
        // misma hora que la llegada seria mentir.
        if (a->vuelo_min > 0)
            reloj(ahora + a->falta_min - a->vuelo_min, a->dep, sizeof a->dep);
    }
    a->demora = 0;   // ver el comentario de arriba de todo

    // --- estado ---
    if (a->alt <= 0 && s->gs < 60)
        snprintf(a->estado, sizeof a->estado, "EN TIERRA");
    else if ((a->falta_min > 0 && a->falta_min <= 25) || (falta_km && falta_km < 60))
        snprintf(a->estado, sizeof a->estado, "APROXIMANDO");
    else
        snprintf(a->estado, sizeof a->estado, "EN VUELO");

    if (es_nuevo) {
        a->rastro_n = 0;
        a->brillo = 82;
    }
}

// Se llama una vez por cuadro. Casi siempre no hay nada nuevo y sale enseguida.
void vivo_avanzar(void) {
    sky_avion_t nuevos[SKY_MAX];
    const int n = sky_tomar(nuevos, SKY_MAX);
    if (n < 0) return;

    avion_t copia[RADAR_MAX_AVIONES];
    char    copia_hex[RADAR_MAX_AVIONES][8];
    uint8_t copia_faltazos[RADAR_MAX_AVIONES];
    int     usados = 0;

    // Primero los que vinieron en el lote, en el orden que los mando el
    // proxy: llegan ordenados por cercania.
    for (int i = 0; i < n && usados < RADAR_MAX_AVIONES; i++) {
        const int antes = lugar_de(nuevos[i].hex, radar_cantidad);
        if (antes >= 0) copia[usados] = radar_aviones[antes];
        else            memset(&copia[usados], 0, sizeof copia[usados]);
        componer(&copia[usados], &nuevos[i], antes < 0);
        snprintf(copia_hex[usados], sizeof copia_hex[usados], "%s", nuevos[i].hex);
        copia_faltazos[usados] = 0;
        usados++;
    }

    // Despues los que ya estaban y este lote no nombro. Se quedan quietos un
    // par de vueltas por si la fuente los vuelve a ver.
    for (int i = 0; i < radar_cantidad && usados < RADAR_MAX_AVIONES; i++) {
        bool esta = false;
        for (int j = 0; j < n; j++)
            if (!strcmp(hexes[i], nuevos[j].hex)) { esta = true; break; }
        if (esta) continue;
        if (faltazos[i] + 1 >= OLVIDO_LOTES) continue;
        copia[usados] = radar_aviones[i];
        memcpy(copia_hex[usados], hexes[i], sizeof hexes[i]);
        copia_faltazos[usados] = faltazos[i] + 1;
        usados++;
    }

    memcpy(radar_aviones, copia, sizeof(avion_t) * usados);
    memcpy(hexes, copia_hex, sizeof(copia_hex[0]) * usados);
    memcpy(faltazos, copia_faltazos, usados);
    radar_cantidad = usados;
    vivo_andando = true;
    radar_marcar_sucio();
}

// Cambia el aeropuerto del radar. Es lo mismo que demo_aeropuerto pero sin
// repartir aviones inventados alrededor: los de verdad ya estan donde estan,
// y el proximo lote va a venir centrado en el nuevo.
void vivo_aeropuerto(const char *iata) {
    const aeropuerto_dato_t *a = aeropuerto_buscar(iata);
    if (!a) return;
    snprintf(radar_apt.iata,   sizeof radar_apt.iata,   "%s", a->iata);
    snprintf(radar_apt.nombre, sizeof radar_apt.nombre, "%s", a->ciudad);
    radar_apt.lat = a->lat;
    radar_apt.lon = a->lon;
    // Las distancias de las tarjetas son contra este aeropuerto: si no se
    // rehacen, quedan las del anterior hasta el proximo lote.
    for (int i = 0; i < radar_cantidad; i++)
        radar_aviones[i].dist_km = geo_km(a->lat, a->lon,
                                          radar_aviones[i].lat, radar_aviones[i].lon);
    radar_marcar_sucio();
}

// Si alguna vez llego un lote de verdad. Mientras sea falso, el que mueve los
// aviones es demo.c.
bool vivo_hay_datos(void) { return vivo_andando; }
