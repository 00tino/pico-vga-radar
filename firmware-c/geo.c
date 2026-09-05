#include "geo.h"
#include "trig.h"

// Un grado de latitud son 111 km. En longitud hay que achicar por el coseno
// de la latitud, si no las distancias este-oeste salen infladas.
static int coseno_lat(int32_t lat) {
    int a = trig_de_grados((int)(lat / 10000));
    int c = trig_cos(a);
    return c < 0 ? -c : c;                  // 0..1024
}

int geo_km(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    int32_t dlat = lat2 - lat1;
    int32_t dlon = (int32_t)(((int64_t)(lon2 - lon1) * coseno_lat(lat1)) / TRIG_UNO);
    // De diezmilesimas de grado a km: 111 km por grado.
    int64_t x = (int64_t)dlat * 111 / 10000;
    int64_t y = (int64_t)dlon * 111 / 10000;
    int64_t d2 = x * x + y * y;
    int r = 0;
    while ((int64_t)(r + 1) * (r + 1) <= d2) r++;
    return r;
}

int geo_rumbo(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    int32_t dlat = lat2 - lat1;
    int32_t dlon = (int32_t)(((int64_t)(lon2 - lon1) * coseno_lat(lat1)) / TRIG_UNO);
    // atan2 devuelve el angulo desde el este creciendo hacia el sur; el rumbo
    // se cuenta desde el norte hacia el este.
    int a = trig_atan2(dlon, dlat);
    return (a * 360 / TRIG_VUELTA + 360) % 360;
}

int geo_dif_rumbo(int a, int b) {
    int d = ((a - b) % 360 + 360) % 360;
    return d > 180 ? 360 - d : d;
}

void geo_proyectar(int32_t lat, int32_t lon, int rumbo, int km,
                   int32_t *lat_out, int32_t *lon_out) {
    int a = trig_de_grados(rumbo);
    int32_t dlat = (int32_t)((int64_t)km * 10000 * trig_cos(a) / TRIG_UNO / 111);
    int32_t dlon = (int32_t)((int64_t)km * 10000 * trig_sen(a) / TRIG_UNO / 111);
    int c = coseno_lat(lat);
    if (c > 0) dlon = (int32_t)((int64_t)dlon * TRIG_UNO / c);
    *lat_out = lat + dlat;
    *lon_out = lon + dlon;
}
