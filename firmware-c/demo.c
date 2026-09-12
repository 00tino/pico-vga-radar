// Trafico simulado, para poder ver el radar andando antes de que exista el
// WiFi. Los aviones se mueven de verdad: cada cuadro avanzan segun su rumbo
// y su velocidad, igual que van a hacer con los datos reales del proxy.
#include "radar.h"
#include "config.h"
#include "trig.h"
#include "aeropuertos.h"
#include <stdio.h>
#include <string.h>

static const struct {
    const char *vuelo, *al, *tipo, *ori, *des, *ciudad_o, *ciudad_d, *dep, *arr, *estado;
    int dlat, dlon, track, gs, alt, pct;
} SEMILLA[] = {
    { "AR1301", "AR", "A330-200", "EZE", "MAD", "Buenos Aires", "Madrid",     "23:55", "16:20", "EN VUELO",    4200, -6100,  52, 430, 31000, 62 },
    { "LA7724", "LA", "A320neo",  "SCL", "EZE", "Santiago",     "Buenos Aires","08:10", "11:05", "APROXIMANDO",-3100,  5200, 228, 390, 27000, 88 },
    { "AA954",  "AA", "B788",     "MIA", "EZE", "Miami",        "Buenos Aires","22:40", "09:15", "EN VUELO",   7600,  2400, 195, 460, 35000, 74 },
    { "IB6841", "IB", "A350-900", "MAD", "EZE", "Madrid",       "Buenos Aires","12:00", "21:30", "EN VUELO",  -6200, -4300,  15, 440, 33000, 55 },
    { "JJ8003", "LA", "B777",     "GRU", "EZE", "Sao Paulo",    "Buenos Aires","07:25", "10:35", "EN VUELO",   1200,  8100, 275, 410, 29000, 41 },
    { "AF228",  "AF", "B777-300", "CDG", "EZE", "Paris",        "Buenos Aires","23:20", "08:45", "EN VUELO",  -8400,  1100,  78, 470, 37000, 80 },
    { "UA820",  "UA", "B789",     "IAH", "EZE", "Houston",      "Buenos Aires","21:50", "10:10", "EN VUELO",   5300,  7200, 310, 420, 32000, 67 },
    { "AR1885", "AR", "B737-800", "COR", "EZE", "Cordoba",      "Buenos Aires","09:40", "10:55", "APROXIMANDO",  404, -2321, 102, 160,  3000, 96 },
    { "AZ680",  "AZ", "A330-900", "FCO", "EZE", "Roma",         "Buenos Aires","22:15", "08:30", "EN VUELO",   8900, -2100, 168, 450, 36000, 71 },
    { "EK247",  "EK", "B777-300", "DXB", "EZE", "Dubai",        "Buenos Aires","03:30", "17:45", "EN VUELO",  -4700,  6800, 340, 480, 38000, 58 },
    { "QF17",   "QF", "B789",     "SYD", "EZE", "Sydney",       "Buenos Aires","11:30", "12:15", "EN VUELO",   2600, -8800,  95, 400, 30000, 63 },
    { "AR2451", "AR", "E190",     "AEP", "MDZ", "Buenos Aires", "Mendoza",     "10:05", "11:50", "EN VUELO",   -900,  1400, 240, 180,  4500, 12 },
    { "LA4090", "LA", "A320",     "EZE", "LIM", "Buenos Aires", "Lima",        "10:20", "14:35", "EN TIERRA",  1100,  -600,  62, 160,     0,  2 },
    { "BA245",  "BA", "B788",     "LHR", "EZE", "Londres",      "Buenos Aires","22:05", "09:40", "EN VUELO",  -7300, -5400, 288, 455, 34000, 76 },
    // Sin logo a proposito: en la tarjeta tiene que salir el codigo en un
    // recuadro, que es lo que hace la web cuando no encuentra la imagen.
    { "MX703",  "MX", "A320",     "MEX", "EZE", "Mexico",       "Buenos Aires","01:15", "12:40", "EN VUELO",   6100, -3900, 172, 445, 35000, 69 },
};

void demo_init(void);

// Cambia el aeropuerto del radar y vuelve a repartir el trafico alrededor.
// Sin esto los aviones se quedan en las coordenadas del aeropuerto anterior.
// La casa de prueba: a unos 12 km al noreste de Ezeiza, para que se vea que
// no tiene por que estar pegada al aeropuerto.
void demo_casa(void) {
    // Si el cliente cargo la suya, esta no existe: demo_aeropuerto() llama
    // aca cada vez que cambia de aeropuerto, y sin esto le pisaba la casa
    // configurada en cada rotacion de pantallas.
    if (config_casa_on) return;
    radar_casa_on = true;
    radar_casa_lat = radar_apt.lat + 800;
    radar_casa_lon = radar_apt.lon + 900;
    radar_casa_km = 3;
}

void demo_aeropuerto(const char *iata) {
    const aeropuerto_dato_t *a = aeropuerto_buscar(iata);
    if (!a) return;
    snprintf(radar_apt.iata, sizeof radar_apt.iata, "%s", a->iata);
    snprintf(radar_apt.nombre, sizeof radar_apt.nombre, "%s", a->ciudad);
    radar_apt.lat = a->lat;
    radar_apt.lon = a->lon;
    demo_init();
    demo_casa();
}

void demo_init(void) {
    radar_cantidad = sizeof(SEMILLA) / sizeof(SEMILLA[0]);
    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        a->lat = radar_apt.lat + SEMILLA[i].dlat;
        a->lon = radar_apt.lon + SEMILLA[i].dlon;
        a->track = SEMILLA[i].track;
        a->gs = SEMILLA[i].gs;
        a->alt = SEMILLA[i].alt;
        strncpy(a->vuelo, SEMILLA[i].vuelo, sizeof a->vuelo - 1);
        strncpy(a->aerolinea, SEMILLA[i].al, 2);
        strncpy(a->tipo, SEMILLA[i].tipo, sizeof a->tipo - 1);
        strncpy(a->origen, SEMILLA[i].ori, 3);
        strncpy(a->ciudad_o, SEMILLA[i].ciudad_o, sizeof a->ciudad_o - 1);
        // Los que en la semilla llegan a Ezeiza llegan en realidad al
        // aeropuerto que este elegido: si no, en Narita las tarjetas dirian
        // que todos van a Buenos Aires.
        // Ojo con el vuelo que ya salia de ese aeropuerto: si no, queda con
        // origen y destino iguales, como MAD > MAD.
        if (!strncmp(SEMILLA[i].des, "EZE", 3) && strncmp(SEMILLA[i].ori, radar_apt.iata, 3)) {
            strncpy(a->destino, radar_apt.iata, 3);
            strncpy(a->ciudad_d, radar_apt.nombre, sizeof a->ciudad_d - 1);
        } else {
            strncpy(a->destino, SEMILLA[i].des, 3);
            strncpy(a->ciudad_d, SEMILLA[i].ciudad_d, sizeof a->ciudad_d - 1);
        }
        strncpy(a->dep, SEMILLA[i].dep, 5);
        strncpy(a->arr, SEMILLA[i].arr, 5);
        strncpy(a->estado, SEMILLA[i].estado, sizeof a->estado - 1);
        a->pct = SEMILLA[i].pct;
        // Datos de puntualidad, repartidos para que se vean los dos casos.
        // Uno de cada cuatro demorado y uno de cada cuatro adelantado, para
        // que se vean los tres casos.
        a->demora = (int16_t)((i % 4 == 1) ? 10 + (i * 7) % 40
                            : (i % 4 == 2) ? -(5 + (i * 3) % 15) : 0);
        // Duracion del vuelo y cuanto falta, coherentes con el avance.
        a->vuelo_min = (int16_t)(60 + (i * 97) % 600);
        a->falta_min = (int16_t)(a->vuelo_min * (100 - SEMILLA[i].pct) / 100);
        a->brillo = 82;
    }
}

// Un cuadro dura 1/60 de segundo. A 450 nudos eso son 3,86 metros, o sea
// 0,000035 grados: hace falta acumular para que el paso no se pierda en el
// redondeo de los enteros.
static int32_t resto_lat[RADAR_MAX_AVIONES], resto_lon[RADAR_MAX_AVIONES];

void demo_avanzar(void) {
    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int t = trig_de_grados(a->track);
        // Nudos a diezmilesimas de grado por cuadro, x1000 para no perder el resto:
        //   nudos * 1852 m/h / 3600 / 60 cuadros / 111320 m/grado * 10000 * 1000
        int32_t paso = (int32_t)a->gs * 771 / 1000;
        resto_lat[i] += paso * trig_cos(t) / TRIG_UNO;
        resto_lon[i] += paso * trig_sen(t) / TRIG_UNO;
        a->lat += resto_lat[i] / 1000; resto_lat[i] %= 1000;
        a->lon += resto_lon[i] / 1000; resto_lon[i] %= 1000;

        // AR2451 pasa una y otra vez por encima de la casa, siempre con el
        // mismo rumbo: un avion de verdad no gira sobre el lugar.
        if (!strncmp(a->vuelo, "AR2451", 6) && radar_casa_on) {
            a->track = 75;
            int t = trig_de_grados(a->track);
            int32_t paso = (int32_t)a->gs * 771 / 1000;
            resto_lat[i] += paso * trig_cos(t) / TRIG_UNO;
            resto_lon[i] += paso * trig_sen(t) / TRIG_UNO;
            a->lat += resto_lat[i] / 1000; resto_lat[i] %= 1000;
            a->lon += resto_lon[i] / 1000; resto_lon[i] %= 1000;
            // Cuando se aleja, vuelve a arrancar antes de la casa.
            if (a->lon - radar_casa_lon > 600) {
                a->lat = radar_casa_lat - 160;
                a->lon = radar_casa_lon - 600;
                resto_lat[i] = resto_lon[i] = 0;
            }
            continue;
        }

        // El vuelo que viene aproximando se manda de nuevo al principio de la
        // senda cuando pasa la cabecera, asi la aproximacion siempre se ve.
        if (!strncmp(a->vuelo, "AR1885", 6)) {
            int32_t dla = a->lat - (radar_apt.lat + 404 + 2000);
            int32_t dlo = a->lon - (radar_apt.lon - 2321 + 2000);
            if (dlo > 0 || dla > 2500) {
                a->lat = radar_apt.lat + 404;
                a->lon = radar_apt.lon - 2321;
                resto_lat[i] = resto_lon[i] = 0;
            }
            continue;
        }

        // Cuando se van del alcance vuelven a entrar por el lado opuesto,
        // desplazados un poco para que no terminen todos amontonados.
        int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;
        int32_t sesgo = (int32_t)(i * 1700) - 6000;
        if (a->lat - radar_apt.lat >  span) { a->lat -= 2 * span; a->lon = radar_apt.lon + sesgo; }
        if (a->lat - radar_apt.lat < -span) { a->lat += 2 * span; a->lon = radar_apt.lon + sesgo; }
        if (a->lon - radar_apt.lon >  span) { a->lon -= 2 * span; a->lat = radar_apt.lat + sesgo; }
        if (a->lon - radar_apt.lon < -span) { a->lon += 2 * span; a->lat = radar_apt.lat + sesgo; }
    }
}
