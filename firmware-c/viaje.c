// Vista de seguimiento: la ruta completa del vuelo que se esta siguiendo.
//
// Traduce drawJourney() de la web: mapa con rejilla de meridianos y paralelos,
// la ruta entera punteada fina, el tramo que falta en trazo marcado, circulos
// en origen y destino con sus nombres, y el avion sobre la ruta.
//
// El arco se calcula por circulo maximo con la unidad de punto flotante del
// RP2350: son 48 puntos por cuadro y sale barato. En lineas rectas de lat/lon
// una ruta larga se iria varios grados de donde va en serio.
#include "radar.h"
#include "gfx.h"
#include "area.h"
#include "trig.h"
#include "geo.h"
#include "vga.h"
#include "aeropuertos.h"
#include "costas.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern uint8_t radar_tono(int alpha255);

#define PUNTOS 48

static float g2r(int32_t g) { return (float)g / 10000.0f * (float)M_PI / 180.0f; }

// Lleva una longitud al entorno de un ancla, para que la diferencia nunca
// pase de media vuelta. Sin esto un vuelo Sydney-Buenos Aires se dibuja
// cruzando el mundo por el lado largo en vez de por el Pacifico.
static int32_t wrap_lon(int32_t lon, int32_t ancla) {
    while (lon - ancla >  1800000) lon -= 3600000;
    while (lon - ancla < -1800000) lon += 3600000;
    return lon;
}

// Distancia por circulo maximo, en kilometros. La de geo.c es plana y sirve
// para el alcance de un radar, no para un vuelo intercontinental.
static int km_gc(int32_t la1, int32_t lo1, int32_t la2, int32_t lo2) {
    float p1 = g2r(la1), p2 = g2r(la2);
    float dp = p2 - p1, dl = g2r(lo2) - g2r(lo1);
    float a = sinf(dp / 2) * sinf(dp / 2) +
              cosf(p1) * cosf(p2) * sinf(dl / 2) * sinf(dl / 2);
    return (int)lrintf(6371.0f * 2.0f * asinf(sqrtf(a > 1.0f ? 1.0f : a)));
}

// Un punto del circulo maximo entre dos coordenadas, con f de 0 a 1.
static void interpolar(int32_t la1, int32_t lo1, int32_t la2, int32_t lo2,
                       float f, int32_t *la, int32_t *lo) {
    float p1 = g2r(la1), l1 = g2r(lo1), p2 = g2r(la2), l2 = g2r(lo2);
    float d = 2.0f * asinf(sqrtf(
        powf(sinf((p1 - p2) / 2.0f), 2.0f) +
        cosf(p1) * cosf(p2) * powf(sinf((l1 - l2) / 2.0f), 2.0f)));
    if (d < 1e-6f) { *la = la1; *lo = lo1; return; }
    float a = sinf((1.0f - f) * d) / sinf(d);
    float b = sinf(f * d) / sinf(d);
    float x = a * cosf(p1) * cosf(l1) + b * cosf(p2) * cosf(l2);
    float y = a * cosf(p1) * sinf(l1) + b * cosf(p2) * sinf(l2);
    float z = a * sinf(p1) + b * sinf(p2);
    *la = (int32_t)lrintf(atan2f(z, sqrtf(x * x + y * y)) * 180.0f / (float)M_PI * 10000.0f);
    *lo = (int32_t)lrintf(atan2f(y, x) * 180.0f / (float)M_PI * 10000.0f);
}

// Donde quedo el avion en el ultimo dibujo del mapa.
int avion_x, avion_y;

// Los aeropuertos por los que va pasando el vuelo. Los calcula el mapa y los
// muestra tambien la tarjeta, asi que viven aca afuera.
char    viaje_paso_cod[5][4];
int32_t viaje_paso_lat[5], viaje_paso_lon[5];
int     viaje_paso_n = 0;

void radar_pintar_viaje(void) {
    const int X = area.x, Y = area.y, AL = area.al;
    // En la vista con tarjeta al costado el mapa ocupa el 54 por ciento, igual
    // que el scope en la hibrida.
    const int AN = area.an;
    vga_limpiar_rect(0, gfx_banda_y0, VGA_ANCHO, gfx_banda_y1, radar_tono(0));

    // El avion que se sigue.
    const avion_t *ac = 0;
    for (int i = 0; i < radar_cantidad; i++)
        if (!strncmp(radar_aviones[i].vuelo, radar_seguir, sizeof radar_seguir - 1))
            { ac = &radar_aviones[i]; break; }
    if (!ac) {
        gfx_texto_centrado(X + AN / 2, Y + AL / 2 - 8, "Buscando el vuelo...", radar_tono(200), 2);
        gfx_texto_centrado(X + AN / 2, Y + AL / 2 + 24, radar_seguir, radar_tono(255), 1);
        return;
    }

    const aeropuerto_dato_t *o = aeropuerto_buscar(ac->origen);
    const aeropuerto_dato_t *d = aeropuerto_buscar(ac->destino);

    // El mapa y la tarjeta se acomodan segun la forma de la ruta. Un vuelo
    // que cruza el Pacifico es ancho y bajo: si se lo mete en la mitad
    // izquierda queda diminuto. En ese caso el mapa va arriba y la tarjeta
    // abajo. Un vuelo norte-sur, como Buenos Aires a Miami, es alto y
    // angosto, y ahi conviene el mapa a la izquierda con la tarjeta al lado.
    int mapa_x = X, mapa_y = Y + 34, mapa_an = area.an, mapa_al = AL - 46;
    int tarj_x = 0, tarj_y = 0, tarj_an = 0, tarj_al = 0;
    if (radar_vista == VISTA_SEGUIR_HIBRIDA) {
        int32_t ancho_ruta = 0, alto_ruta_geo = 0;
        if (o && d) {
            int32_t dlo = wrap_lon(d->lon, o->lon) - o->lon;
            ancho_ruta = dlo < 0 ? -dlo : dlo;
            int32_t dla = d->lat - o->lat;
            alto_ruta_geo = dla < 0 ? -dla : dla;
            // La longitud se achica por la latitud, para comparar peras con peras.
            ancho_ruta = (int32_t)((int64_t)ancho_ruta * geo_coslat((o->lat + d->lat) / 2) / 1024);
        }
        if (ancho_ruta > alto_ruta_geo * 3 / 2) {
            // Ruta ancha: mapa arriba, tarjeta abajo.
            mapa_an = area.an;
            mapa_al = (AL - 46) * 62 / 100;
            tarj_x = X + 8;  tarj_y = mapa_y + mapa_al + 8;
            tarj_an = area.an - 16;
            tarj_al = AL - (tarj_y - Y) - 8;
        } else {
            // Ruta alta: mapa a la izquierda, tarjeta al costado.
            mapa_an = area.an * 54 / 100;
            tarj_x = X + mapa_an + 8;  tarj_y = Y + 34;
            tarj_an = area.an - mapa_an - 16;
            tarj_al = AL - 42;
        }
    }
    const int x0 = mapa_x + 18, y0 = mapa_y;
    const int anc = mapa_an - 36, alt = mapa_al;
    // Todo lo del mapa se recorta a su recuadro; la barra y la tarjeta van
    // fuera de el, asi que hay que guardar la banda para reponerla despues.
    const int banda_afuera_y0 = gfx_banda_y0, banda_afuera_y1 = gfx_banda_y1;
    const int banda_afuera_x0 = gfx_banda_x0, banda_afuera_x1 = gfx_banda_x1;
    {
        const int ry0 = y0 > gfx_banda_y0 ? y0 : gfx_banda_y0;
        const int ry1 = (y0 + alt - 1) < gfx_banda_y1 ? (y0 + alt - 1) : gfx_banda_y1;
        gfx_banda(ry0, ry1);
        // Y tambien a lo ancho: con la tarjeta al costado, las fronteras y la
        // ruta se dibujaban encima de ella.
        gfx_banda_ancho(x0, x0 + anc - 1);
    }
    // Todas las longitudes se miden respecto del origen, para que el
    // antimeridiano no parta la ruta al medio.
    const int32_t ancla = o ? o->lon : (d ? d->lon : ac->lon);
    #define WL(lo) wrap_lon((lo), ancla)

    // Encuadre: todo lo que hay que mostrar, con un margen alrededor.
    int32_t minla = ac->lat, maxla = ac->lat;
    int32_t minlo = WL(ac->lon), maxlo = WL(ac->lon);
    #define ABARCAR(la, lo) do { if ((la) < minla) minla = (la); if ((la) > maxla) maxla = (la); \
                                 if ((lo) < minlo) minlo = (lo); if ((lo) > maxlo) maxlo = (lo); } while (0)
    if (o) ABARCAR(o->lat, WL(o->lon));
    if (d) ABARCAR(d->lat, WL(d->lon));
    // Margen chico: mejor que se corte un continente por el borde y no que
    // la ruta quede perdida en el medio de un mapa vacio.
    int32_t pad_la = (maxla - minla) / 12 + 60000;
    int32_t pad_lo = (maxlo - minlo) / 12 + 60000;
    minla -= pad_la; maxla += pad_la; minlo -= pad_lo; maxlo += pad_lo;

    // Escala: la misma en los dos ejes, para que no se deforme.
    int32_t span_la = maxla - minla, span_lo = maxlo - minlo;
    if (span_la < 1) span_la = 1;
    if (span_lo < 1) span_lo = 1;
    // Pixeles por grado, en centesimas: con enteros pelados una escala de
    // 2,9 se volvia 2 y el mapa quedaba mucho mas chico de lo que entra.
    int64_t esc_la = (int64_t)alt * 1000000 / span_la;
    int64_t esc_lo = (int64_t)anc * 1000000 / span_lo;
    int64_t esc = esc_la < esc_lo ? esc_la : esc_lo;
    if (esc < 1) esc = 1;
    const int32_t midla = (minla + maxla) / 2, midlo = (minlo + maxlo) / 2;

    // Lo que de verdad entra en pantalla con esa escala. Hay que recalcularlo:
    // la escala es la misma en los dos ejes, asi que el eje que no manda
    // termina mostrando mucho mas de lo que pedia el encuadre. Sin esto se
    // descartaban continentes que si se veian: en un Sydney-Buenos Aires
    // aparecia Australia y faltaba Sudamerica.
    minla = midla - (int32_t)((int64_t)alt * 1000000 / esc) / 2;
    maxla = midla + (int32_t)((int64_t)alt * 1000000 / esc) / 2;
    minlo = midlo - (int32_t)((int64_t)anc * 1000000 / esc) / 2;
    maxlo = midlo + (int32_t)((int64_t)anc * 1000000 / esc) / 2;
    span_la = maxla - minla;
    span_lo = maxlo - minlo;

    // MXW toma una longitud ya envuelta; MX la envuelve al ancla primero.
    #define MXW(low)   (x0 + anc / 2 + (int)((int64_t)((low) - midlo) * esc / 1000000))
    #define MX(la, lo) MXW(WL(lo))
    #define MY(la, lo) (y0 + alt / 2 - (int)((int64_t)((la) - midla) * esc / 1000000))

    // Las costas del mundo: relleno tenue y contorno, igual que la web
    // (alpha 0,16 para el relleno y 0,5 para el borde).
    {
        // Estaticos y no en la pila: son 7 KB, y la pila de la Pico es chica.
        static int px[GFX_POLIGONO_MAX], py[GFX_POLIGONO_MAX];
        static int32_t lonw[GFX_POLIGONO_MAX];
        const uint8_t relleno = radar_tono(41), linea_costa = radar_tono(128);
        for (int i = 0; i < COSTAS_ANILLOS; i++) {
            const int desde = costas_anillos[i].desde, cant = costas_anillos[i].cantidad;
            if (cant < 4 || cant > GFX_POLIGONO_MAX) continue;

            // Las longitudes del contorno se envuelven en cadena, cada una
            // respecto de la anterior, y solo la primera respecto del centro
            // del mapa. Envolviendolas sueltas contra el ancla, un contorno
            // que cruza el antimeridiano relativo quedaba partido en dos
            // mitades separadas por media vuelta: sus lados atravesaban el
            // mapa entero y el relleno salia en franjas a lo ancho.
            lonw[0] = wrap_lon((int32_t)costas_lon[desde] * 100, midlo);
            for (int j = 1; j < cant; j++)
                lonw[j] = wrap_lon((int32_t)costas_lon[desde + j] * 100, lonw[j - 1]);

            // Descartar rapido lo que no cae en el encuadre.
            int32_t a = 900000, b = -900000, cc = 0x7fffffff, dd = -0x7fffffff;
            for (int j = 0; j < cant; j++) {
                int32_t la = (int32_t)costas_lat[desde + j] * 100;
                if (la < a) a = la;
                if (la > b) b = la;
                if (lonw[j] < cc) cc = lonw[j];
                if (lonw[j] > dd) dd = lonw[j];
            }
            if (b < minla || a > maxla || dd < minlo || cc > maxlo) continue;

            for (int j = 0; j < cant; j++) {
                int32_t la = (int32_t)costas_lat[desde + j] * 100;
                // Cerca de los polos esta proyeccion estira sin fin: la
                // Antartida salia como una franja aplastada de punta a punta.
                if (la < -780000) la = -780000;
                if (la >  780000) la =  780000;
                px[j] = MXW(lonw[j]);
                py[j] = MY(la, 0);
            }
            // Recortado contra el recuadro del mapa. Antes se acotaban los
            // puntos sueltos al borde, y eso juntaba vertices que estan lejos
            // entre si: el relleno se escapaba en franjas a lo ancho de la
            // pantalla y la Antartida salia aplastada.
            static int rx[GFX_POLIGONO_MAX * 2], ry[GFX_POLIGONO_MAX * 2];
            int rn = gfx_poligono_recortar(px, py, cant, x0, y0, x0 + anc - 1, y0 + alt - 1,
                                           rx, ry, GFX_POLIGONO_MAX * 2);
            if (rn < 3) continue;
            gfx_poligono_lleno(rx, ry, rn, relleno);
            for (int j = 0; j < rn; j++) {
                int k = (j + 1) % rn;
                gfx_linea(rx[j], ry[j], rx[k], ry[k], linea_costa);
            }
        }

    }

    // Limites entre paises, con trazo mas fino que las costas.
    {
        const uint8_t c_frontera = radar_tono(90);
        for (int i = 0; i < FRONTERAS_TRAMOS; i++) {
            const int desde = fronteras_tramos[i].desde, cant = fronteras_tramos[i].cantidad;
            int ax = 0, ay = 0;
            // Mismo encadenado que las costas: si no, una frontera que cruza
            // el antimeridiano relativo cruzaba el mapa de lado a lado.
            int32_t low = 0;
            for (int j = 0; j < cant; j++) {
                int32_t la = (int32_t)fronteras_lat[desde + j] * 100;
                int32_t lo = (int32_t)fronteras_lon[desde + j] * 100;
                low = j ? wrap_lon(lo, low) : wrap_lon(lo, midlo);
                int bx = MXW(low), by = MY(la, 0);
                if (bx < -4000) bx = -4000;
                if (bx >  4000) bx =  4000;
                if (by < -4000) by = -4000;
                if (by >  4000) by =  4000;
                if (j) gfx_linea(ax, ay, bx, by, c_frontera);
                ax = bx; ay = by;
            }
        }
    }

    // Rejilla de paralelos y meridianos, con el paso que corresponda.
    // Cada eje lleva su propio paso: con el de latitud, un vuelo largo
    // dibujaba cientos de meridianos pegados.
    const int paso_la = span_la > 400000 ? 20 : span_la > 180000 ? 10 : span_la > 60000 ? 5 : 2;
    const int paso_lo = span_lo > 900000 ? 30 : span_lo > 400000 ? 20 : span_lo > 180000 ? 10 : span_lo > 60000 ? 5 : 2;
    const uint8_t linea = radar_tono(30), rotulo = radar_tono(100);
    char buf[64];
    // Los paralelos existen entre -90 y 90: sin acotar salian rotulos de
    // -140 grados cuando el encuadre se estira por la escala.
    int32_t la_desde = minla < -900000 ? -900000 : minla;
    int32_t la_hasta = maxla >  900000 ?  900000 : maxla;
    for (int32_t g = (la_desde / (paso_la * 10000)) * paso_la * 10000; g <= la_hasta; g += paso_la * 10000) {
        int py = MY(g, midlo);
        if (py < y0 || py > y0 + alt) continue;
        gfx_hlinea(x0, py, anc, linea);
        snprintf(buf, sizeof buf, "%d", (int)(g / 10000));
        gfx_texto(x0 + 4, py - 15, buf, rotulo, 1);
    }
    for (int32_t g = (minlo / (paso_lo * 10000)) * paso_lo * 10000; g <= maxlo; g += paso_lo * 10000) {
        int pxx = MX(midla, g);
        if (pxx < x0 || pxx > x0 + anc) continue;
        gfx_vlinea(pxx, y0, alt, linea);
    }
    gfx_rect(x0, y0, anc, alt, radar_tono(60));

    // Los puntos por donde pasa el vuelo: en cinco lugares de la ruta se
    // busca el aeropuerto mas cercano y se lo marca, como hace la web. Se
    // calcula solo cuando cambia el vuelo, que recorrer los 5334 aeropuertos
    // cinco veces no es para hacerlo en cada cuadro.
    if (o && d) {
        static char cache_vuelo[9];

        if (strncmp(cache_vuelo, ac->vuelo, sizeof cache_vuelo - 1)) {
            snprintf(cache_vuelo, sizeof cache_vuelo, "%s", ac->vuelo);
            viaje_paso_n = 0;
            static const int FRAC[5] = { 18, 36, 54, 72, 88 };
            for (int k = 0; k < 5; k++) {
                int32_t la, lo;
                interpolar(o->lat, o->lon, d->lat, d->lon, FRAC[k] / 100.0f, &la, &lo);
                int mejor = -1, mejor_km = 260;
                for (int i = 0; i < AEROPUERTOS_CANT; i++) {
                    if (!strncmp(aeropuertos[i].iata, o->iata, 3) ||
                        !strncmp(aeropuertos[i].iata, d->iata, 3)) continue;
                    int km = km_gc(aeropuertos[i].lat, aeropuertos[i].lon, la, lo);
                    if (km < mejor_km) { mejor_km = km; mejor = i; }
                }
                if (mejor >= 0) {
                    int repetido = 0;
                    for (int q = 0; q < viaje_paso_n; q++)
                        if (!strncmp(viaje_paso_cod[q], aeropuertos[mejor].iata, 3)) repetido = 1;
                    if (!repetido) {
                        viaje_paso_lat[viaje_paso_n] = aeropuertos[mejor].lat;
                        viaje_paso_lon[viaje_paso_n] = aeropuertos[mejor].lon;
                        snprintf(viaje_paso_cod[viaje_paso_n], sizeof viaje_paso_cod[viaje_paso_n], "%s", aeropuertos[mejor].iata);
                        viaje_paso_n++;
                    }
                }
            }
        }
    }

    // Ruta entera, punteado fino; y lo que falta, punteado marcado.
    //
    // La ruta va de tramo en tramo por los puntos de paso, no derecho del
    // origen al destino: si no, la linea no tocaba ninguno de los puntos que
    // ella misma marca. De Miami a Ezeiza pasaba lejos de API y de TRQ, que
    // son justamente los que dicen por donde va el vuelo.
    if (o && d) {
        int32_t tla[7], tlo[7];
        int tn = 0;
        tla[tn] = o->lat; tlo[tn] = o->lon; tn++;
        for (int k = 0; k < viaje_paso_n && tn < 6; k++) {
            tla[tn] = viaje_paso_lat[k]; tlo[tn] = viaje_paso_lon[k]; tn++;
        }
        tla[tn] = d->lat; tlo[tn] = d->lon; tn++;

        // Cada tramo por su propio circulo maximo: los puntos estan lejos
        // entre si y una recta en pantalla no es el camino que vuela.
        const int por_tramo = (PUNTOS / (tn - 1)) < 4 ? 4 : (PUNTOS / (tn - 1));
        int px = 0, py = 0;
        for (int t = 0; t + 1 < tn; t++) {
            for (int i = 0; i <= por_tramo; i++) {
                int32_t la, lo;
                interpolar(tla[t], tlo[t], tla[t + 1], tlo[t + 1],
                           (float)i / por_tramo, &la, &lo);
                int cxp = MX(la, lo), cyp = MY(la, lo);
                // Mas marcada que en la web: alla el relleno de tierra es casi
                // transparente, pero con 3-3-2 bits el relleno y un tono 90
                // caen a dos escalones de distancia y la ruta se perdia
                // encima del continente.
                if (t || i) gfx_linea_punteada(px, py, cxp, cyp, 5, 6, radar_tono(155));
                px = cxp; py = cyp;
            }
        }
    }
    if (d) {
        int px = 0, py = 0;
        for (int i = 0; i <= PUNTOS; i++) {
            int32_t la, lo;
            interpolar(ac->lat, ac->lon, d->lat, d->lon, (float)i / PUNTOS, &la, &lo);
            int cxp = MX(la, lo), cyp = MY(la, lo);
            if (i) {
                gfx_linea_punteada(px, py, cxp, cyp, 8, 5, radar_tono(240));
                gfx_linea_punteada(px, py + 1, cxp, cyp + 1, 8, 5, radar_tono(240));
            }
            px = cxp; py = cyp;
        }
    }

    // Origen y destino.
    // Por donde ya paso: linea continua, mas marcada que la ruta prevista.
    if (ac->rastro_n > 1) {
        for (int i = 1; i < ac->rastro_n; i++) {
            int ax = MX(ac->rastro_lat[i - 1], ac->rastro_lon[i - 1]);
            int ay = MY(ac->rastro_lat[i - 1], ac->rastro_lon[i - 1]);
            int bx = MX(ac->rastro_lat[i], ac->rastro_lon[i]);
            int by = MY(ac->rastro_lat[i], ac->rastro_lon[i]);
            gfx_linea(ax, ay, bx, by, radar_tono(220));
            gfx_linea(ax, ay + 1, bx, by + 1, radar_tono(220));
        }
    }

    // Las marcas de los puntos de paso, ya con la ruta dibujada por encima.
    for (int k = 0; k < viaje_paso_n; k++) {
        int hx = MX(viaje_paso_lat[k], viaje_paso_lon[k]);
        int hy = MY(viaje_paso_lat[k], viaje_paso_lon[k]);
        gfx_circulo_lleno(hx, hy, 3, radar_tono(200));
        gfx_texto(hx + 6, hy - 12, viaje_paso_cod[k], radar_tono(150), 1);
    }

    // Origen y destino. El origen rotula hacia la izquierda y el destino
    // hacia la derecha, y en alturas distintas: con los dos del mismo lado se
    // pisaban cuando la ruta quedaba horizontal.
    const aeropuerto_dato_t *par[2] = { o, d };
    const char *rotulos[2] = { "ORIGEN", "DESTINO" };
    for (int k = 0; k < 2; k++) {
        if (!par[k]) continue;
        int pxx = MX(par[k]->lat, par[k]->lon), pyy = MY(par[k]->lat, par[k]->lon);
        gfx_circulo(pxx, pyy, 7, radar_tono(255));
        gfx_circulo(pxx, pyy, 6, radar_tono(255));

        // Sobre el mapa va solo el codigo: los nombres largos tapaban medio
        // continente. Las ciudades se leen en la barra de arriba.
        const int an_cod = gfx_ancho_texto(par[k]->iata, 1);
        int izq = (pxx + 14 + an_cod > x0 + anc);
        gfx_texto(izq ? pxx - 12 - an_cod : pxx + 12, pyy - 18,
                  par[k]->iata, radar_tono(255), 1);
    }

    // Donde cae el avion, para que el dibujo de encima sepa adonde ir.
    avion_x = MX(ac->lat, ac->lon);
    avion_y = MY(ac->lat, ac->lon);

    gfx_banda(banda_afuera_y0, banda_afuera_y1);
    gfx_banda_ancho(banda_afuera_x0, banda_afuera_x1);

    // Barra de arriba: el vuelo a la izquierda y cuanto falta a la derecha.
    snprintf(buf, sizeof buf, "%s", ac->vuelo);
    gfx_texto(X + 10, Y + 8, buf, radar_tono(255), 1);
    if (o && d) snprintf(buf, sizeof buf, "%s %s  >  %s %s",
                         ac->origen, o->ciudad, ac->destino, d->ciudad);
    else        snprintf(buf, sizeof buf, "%s > %s", ac->origen, ac->destino);
    gfx_texto(X + 10 + gfx_ancho_texto(ac->vuelo, 1) + 10, Y + 8, buf, radar_tono(170), 1);
    if (d) {
        int km = km_gc(ac->lat, ac->lon, d->lat, d->lon);
        snprintf(buf, sizeof buf, "Ruta del viaje - %d km al destino", km);
    } else {
        snprintf(buf, sizeof buf, "Ruta del viaje");
    }
    gfx_texto(X + AN - 10 - gfx_ancho_texto(buf, 1), Y + 8, buf, radar_tono(170), 1);
    gfx_hlinea(X + 10, Y + 26, AN - 20, radar_tono(45));

    // La tarjeta del vuelo, donde haya quedado lugar.
    if (radar_vista == VISTA_SEGUIR_HIBRIDA && tarj_an > 40 && tarj_al > 40) {
        void tarjeta_dibujar(int x, int y, int an, int al, const avion_t *a, int grande);
        tarjeta_dibujar(tarj_x, tarj_y, tarj_an, tarj_al, ac, 0);
    }
}

// Solo el avion, para poder moverlo sin rehacer el mapa entero.
void radar_viaje_avion(void) {
    extern int avion_x, avion_y;
    const avion_t *ac = 0;
    for (int i = 0; i < radar_cantidad; i++)
        if (!strncmp(radar_aviones[i].vuelo, radar_seguir, sizeof radar_seguir - 1))
            { ac = &radar_aviones[i]; break; }
    if (!ac) return;

        int pxx = avion_x, pyy = avion_y;
        const uint8_t c = radar_tono(255);
        // Sin circulo alrededor: cuando el avion llega al destino se juntaba
        // con los dos anillos del aeropuerto y se veian tres circulos y un
        // triangulo amontonados, sin entenderse nada.
        int t = trig_de_grados(ac->track);
        int s = trig_sen(t), co = trig_cos(t);
        #define RX(a, b) (pxx + ((a) * co - (b) * s) / TRIG_UNO)
        #define RY(a, b) (pyy + ((a) * s + (b) * co) / TRIG_UNO)
        gfx_triangulo_lleno(RX(0, -9), RY(0, -9), RX(6, 8), RY(6, 8), RX(-6, 8), RY(-6, 8), c);
        #undef RX
        #undef RY
        // El rotulo va por debajo, y el del aeropuerto por arriba: llegando
        // al destino los dos caian en el mismo renglon.
        gfx_texto(pxx + 12, pyy + 4, ac->vuelo, c, 1);
    
}
