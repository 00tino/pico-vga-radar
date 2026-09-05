// Trafico simulado, para poder ver el radar andando antes de que exista el
// WiFi. Los aviones se mueven de verdad: cada cuadro avanzan segun su rumbo
// y su velocidad, igual que van a hacer con los datos reales del proxy.
#include "radar.h"
#include "trig.h"
#include <string.h>

static const struct { const char *vuelo, *al; int dlat, dlon, track, gs, alt; } SEMILLA[] = {
    { "AR1301", "AR",  4200, -6100,  52, 430, 31000 },
    { "LA7724",  "LA", -3100,  5200, 228, 390, 27000 },
    { "AA954",   "AA",  7600,  2400, 195, 460, 35000 },
    { "IB6841",  "IB", -6200, -4300,  15, 440, 33000 },
    { "JJ8003",  "LA",  1200,  8100, 275, 410, 29000 },
    { "AF228",   "AF", -8400,  1100,  78, 470, 37000 },
    { "UA820",   "UA",  5300,  7200, 310, 420, 32000 },
    { "AR1885",  "AR", -1800, -7600, 132, 300, 12000 },
    { "AZ680",   "AZ",  8900, -2100, 168, 450, 36000 },
    { "EK247",   "EK", -4700,  6800, 340, 480, 38000 },
    { "QF17",    "QF",  2600, -8800,  95, 400, 30000 },
    { "AR2451",  "AR",  -900,  1400, 240, 180,  4500 },
    { "LA4090",  "LA",  1100,  -600,  62, 160,  2800 },
    { "BA245",   "BA", -7300, -5400, 288, 455, 34000 },
};

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

        // Cuando se van lejos del alcance, entran de nuevo por el otro lado.
        int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;
        if (a->lat - radar_apt.lat >  span) a->lat -= 2 * span;
        if (a->lat - radar_apt.lat < -span) a->lat += 2 * span;
        if (a->lon - radar_apt.lon >  span) a->lon -= 2 * span;
        if (a->lon - radar_apt.lon < -span) a->lon += 2 * span;
    }
}
