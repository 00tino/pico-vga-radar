// Las pantallas del cliente, guardadas en la flash.
//
// Hasta ahora la lista se armaba en main.c y se perdia en cada arranque.
// Aca se guarda, para que el dia que exista el portal no haya que tocar nada
// de esto: la web va a llamar a config_guardar() y listo.
//
// Donde va: el ANTEULTIMO sector de la flash. El ultimo es de arranques.c y
// se borra solo cuando el cliente hace el gesto de los tres cortes; si las
// pantallas estuvieran ahi, ese gesto se las llevaria puestas.
//
// Como se sabe si lo que hay sirve: una firma al principio, la version del
// formato, y una suma de todo lo demas al final. Flash sin escribir es todo
// 0xFF, asi que sin la firma ya se sabe que no hay nada. La suma es para el
// caso feo: que se corte la corriente en el medio de un guardado.
#include "config.h"
#include "instalacion.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include <string.h>
#include <stdio.h>

// El de arranques.c es el ultimo; este es el de al lado.
#define SECTOR_OFF  (PICO_FLASH_SIZE_BYTES - 2 * FLASH_SECTOR_SIZE)

#define FIRMA    0x57535031u    // "WSP1", de Wingsplit pantallas
#define VERSION  2

typedef struct {
    uint32_t   firma;
    uint16_t   version;
    uint16_t   n;                  // cuantas pantallas, 0 si solo hay wifi
    uint16_t   max_puntos;         // cuantos aviones se dibujan en el circulo
    uint8_t    solo_aerolineas;
    uint8_t    casa_on;
    int32_t    casa_lat, casa_lon;
    uint16_t   casa_km;
    int16_t    tz_min;             // minutos contra UTC; cero es hora Zulu
    uint8_t    tz_puesto;          // si no, no se distingue de un cero sin usar
    uint8_t    reservado;
    char       ssid[33];
    char       pass[64];
    pantalla_t p[PANTALLAS_MAX];
    uint32_t   suma;
} guardado_t;

char config_ssid[33];
char config_pass[64];

// Cuantos aviones se dibujan como punto en el circulo. Vive aca y no en
// radar.c porque este archivo tambien lo compilan las pruebas de wifi, que no
// tienen radar: quien se lo pasa al dibujo es main.c.
int config_max_puntos = CONFIG_MAX_PUNTOS_DEF;
bool config_solo_aerolineas = true;
int config_tz_min = INSTALACION_TZ_MINUTOS;
bool    config_casa_on;
int32_t config_casa_lat, config_casa_lon;
int     config_casa_km = 3;

static const guardado_t *en_flash = (const guardado_t *)(XIP_BASE + SECTOR_OFF);

// Se graban paginas enteras, asi que el buffer tiene que ser multiplo de
// FLASH_PAGE_SIZE. Con ocho pantallas son unos 600 bytes: entran en tres.
#define PAGINAS ((sizeof(guardado_t) + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE)
static uint8_t buffer[PAGINAS * FLASH_PAGE_SIZE];

// Suma simple de todo menos el campo de la suma. No es criptografia: lo unico
// que tiene que detectar es un guardado a medio hacer.
static uint32_t sumar(const guardado_t *g) {
    const uint8_t *b = (const uint8_t *)g;
    const int hasta = (int)(sizeof *g - sizeof g->suma);
    uint32_t s = 2166136261u;
    for (int i = 0; i < hasta; i++) { s ^= b[i]; s *= 16777619u; }
    return s;
}

static void hacer_guardado(void *nada) {
    (void)nada;
    flash_range_erase(SECTOR_OFF, FLASH_SECTOR_SIZE);
    flash_range_program(SECTOR_OFF, buffer, sizeof buffer);
}

// Deja en config_ssid y config_pass la red que corresponda usar. La del
// portal le gana a la de instalacion.h: si el cliente cargo una, es la que
// quiere, aunque el equipo haya salido de fabrica con otra.
static void elegir_wifi(const char *ssid, const char *pass) {
    const bool hay_guardada = ssid && ssid[0];
    snprintf(config_ssid, sizeof config_ssid, "%s",
             hay_guardada ? ssid : INSTALACION_WIFI_SSID);
    snprintf(config_pass, sizeof config_pass, "%s",
             hay_guardada ? (pass ? pass : "") : INSTALACION_WIFI_PASS);
}

bool config_hay_wifi(void) { return config_ssid[0] != 0; }

bool config_leer(void) {
    elegir_wifi(0, 0);              // por si no hay nada guardado
    if (en_flash->firma != FIRMA) return false;
    if (en_flash->version != VERSION) {
        printf("config: hay pantallas guardadas en otro formato (v%d), se ignoran\n",
               en_flash->version);
        return false;
    }
    if (en_flash->n > PANTALLAS_MAX) return false;
    if (sumar(en_flash) != en_flash->suma) {
        printf("config: lo guardado quedo a medias, se ignora\n");
        return false;
    }
    // El wifi sirve aunque todavia no haya ni una pantalla armada: el cliente
    // carga primero la red y recien despues configura lo que quiere ver.
    elegir_wifi(en_flash->ssid, en_flash->pass);
    if (en_flash->max_puntos >= 1) config_max_puntos = en_flash->max_puntos;
    config_solo_aerolineas = en_flash->solo_aerolineas != 0;
    config_casa_on  = en_flash->casa_on != 0;
    config_casa_lat = en_flash->casa_lat;
    config_casa_lon = en_flash->casa_lon;
    if (en_flash->casa_km >= 1) config_casa_km = en_flash->casa_km;
    if (en_flash->tz_puesto) config_tz_min = en_flash->tz_min;
    if (en_flash->n == 0) {
        printf("config: hay wifi guardado pero ninguna pantalla\n");
        return false;
    }
    memcpy(pantallas, en_flash->p, sizeof(pantalla_t) * en_flash->n);
    pantallas_n = en_flash->n;
    printf("config: %d pantallas leidas de la flash\n", pantallas_n);
    return true;
}

// Arma el bloque con lo que haya que guardar. Las pantallas y el wifi viven
// en el mismo sector, asi que guardar una cosa no puede borrar la otra: se
// escriben siempre las dos.
static void armar(int cuantas) {
    guardado_t *g = (guardado_t *)buffer;
    memset(buffer, 0xFF, sizeof buffer);
    memset(g, 0, sizeof *g);
    g->firma = FIRMA;
    g->version = VERSION;
    g->n = (uint16_t)cuantas;
    g->max_puntos = (uint16_t)config_max_puntos;
    g->solo_aerolineas = config_solo_aerolineas ? 1 : 0;
    g->casa_on  = config_casa_on ? 1 : 0;
    g->casa_lat = config_casa_lat;
    g->casa_lon = config_casa_lon;
    g->casa_km  = (uint16_t)config_casa_km;
    g->tz_min    = (int16_t)config_tz_min;
    g->tz_puesto = 1;
    snprintf(g->ssid, sizeof g->ssid, "%s", config_ssid);
    snprintf(g->pass, sizeof g->pass, "%s", config_pass);
    if (cuantas > 0) memcpy(g->p, pantallas, sizeof(pantalla_t) * cuantas);
    g->suma = sumar(g);
}

bool config_guardar_wifi(const char *ssid, const char *pass) {
    if (!ssid || !ssid[0]) return false;
    snprintf(config_ssid, sizeof config_ssid, "%s", ssid);
    snprintf(config_pass, sizeof config_pass, "%s", pass ? pass : "");

    // Solo se conservan las pantallas que el cliente YA HABIA GUARDADO, no
    // las que esten puestas en memoria. La diferencia importa: mientras nadie
    // configuro nada, lo que hay en memoria son las de ejemplo, y guardarlas
    // aca las convertia en "las del cliente". El equipo entonces creia que ya
    // estaba configurado y no mostraba nunca el QR para configurarlo.
    int cuantas = 0;
    if (en_flash->firma == FIRMA && en_flash->version == VERSION &&
        en_flash->n > 0 && en_flash->n <= PANTALLAS_MAX &&
        sumar(en_flash) == en_flash->suma) {
        memcpy(pantallas, en_flash->p, sizeof(pantalla_t) * en_flash->n);
        pantallas_n = en_flash->n;
        cuantas = en_flash->n;
    }
    armar(cuantas);
    if (flash_safe_execute(hacer_guardado, NULL, 3000) != PICO_OK) {
        printf("config: no se pudo guardar la red\n");
        return false;
    }
    printf("config: red \"%s\" guardada\n", config_ssid);
    return true;
}

bool config_guardar(void) {
    if (pantallas_n <= 0 || pantallas_n > PANTALLAS_MAX) return false;
    armar(pantallas_n);

    // Borrar un sector son decenas de milisegundos con el video parado: sale
    // una franja. Es a proposito que esto no se llame solo, sino cuando el
    // cliente guarda desde el portal, que es un momento en el que ya esta
    // mirando otra cosa.
    if (flash_safe_execute(hacer_guardado, NULL, 3000) != PICO_OK) {
        printf("config: no se pudieron guardar las pantallas\n");
        return false;
    }
    printf("config: %d pantallas guardadas\n", pantallas_n);
    return true;
}

bool config_borrar(void) {
    memset(buffer, 0xFF, sizeof buffer);
    if (flash_safe_execute(hacer_guardado, NULL, 3000) != PICO_OK) return false;
    printf("config: pantallas borradas, vuelven las de fabrica\n");
    return true;
}
