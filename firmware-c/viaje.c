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

void radar_pintar_viaje(void) {
    const int X = area.x, Y = area.y, AL = area.al;
    // En la vista con tarjeta al costado el mapa ocupa el 54 por ciento, igual
    // que el scope en la hibrida.
    const int AN = (radar_vista == VISTA_SEGUIR_HIBRIDA) ? area.an * 54 / 100 : area.an;
    vga_limpiar_rect(0, gfx_banda_y0, X + AN + 4, gfx_banda_y1, radar_tono(0));

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

    const int x0 = X + 18, y0 = Y + 34;
    const int anc = AN - 36, alt = AL - 46;

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
    int32_t pad_la = (maxla - minla) / 6 + 24000;
    int32_t pad_lo = (maxlo - minlo) / 6 + 24000;
    minla -= pad_la; maxla += pad_la; minlo -= pad_lo; maxlo += pad_lo;

    // Escala: la misma en los dos ejes, para que no se deforme.
    int32_t span_la = maxla - minla, span_lo = maxlo - minlo;
    int esc_la = (int)((int64_t)alt * 10000 / span_la);
    int esc_lo = (int)((int64_t)anc * 10000 / span_lo);
    int esc = esc_la < esc_lo ? esc_la : esc_lo;          // px por grado
    const int32_t midla = (minla + maxla) / 2, midlo = (minlo + maxlo) / 2;

    #define MX(la, lo) (x0 + anc / 2 + (int)((int64_t)(WL(lo) - midlo) * esc / 10000))
    #define MY(la, lo) (y0 + alt / 2 - (int)((int64_t)((la) - midla) * esc / 10000))

    // Las costas del mundo: relleno tenue y contorno, igual que la web
    // (alpha 0,16 para el relleno y 0,5 para el borde).
    {
        int px[GFX_POLIGONO_MAX], py[GFX_POLIGONO_MAX];
        const uint8_t relleno = radar_tono(41), linea_costa = radar_tono(128);
        for (int i = 0; i < COSTAS_ANILLOS; i++) {
            const int desde = costas_anillos[i].desde, cant = costas_anillos[i].cantidad;
            if (cant < 4 || cant > GFX_POLIGONO_MAX) continue;

            // Descartar rapido lo que no cae en el encuadre.
            int32_t a = 900000, b = -900000, cc = 0x7fffffff, dd = -0x7fffffff;
            for (int j = 0; j < cant; j++) {
                int32_t la = (int32_t)costas_lat[desde + j] * 100;
                int32_t lo = WL((int32_t)costas_lon[desde + j] * 100);
                if (la < a) a = la;
                if (la > b) b = la;
                if (lo < cc) cc = lo;
                if (lo > dd) dd = lo;
            }
            if (b < minla || a > maxla || dd < minlo || cc > maxlo) continue;

            for (int j = 0; j < cant; j++) {
                int32_t la = (int32_t)costas_lat[desde + j] * 100;
                int32_t lo = (int32_t)costas_lon[desde + j] * 100;
                px[j] = MX(la, lo);
                py[j] = MY(la, lo);
            }
            gfx_poligono_lleno(px, py, cant, relleno);
            for (int j = 0; j < cant; j++) {
                int k = (j + 1) % cant;
                gfx_linea(px[j], py[j], px[k], py[k], linea_costa);
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
    for (int32_t g = (minla / (paso_la * 10000)) * paso_la * 10000; g <= maxla; g += paso_la * 10000) {
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

    // Ruta entera, punteado fino; y lo que falta, punteado marcado.
    if (o && d) {
        int px = 0, py = 0;
        for (int i = 0; i <= PUNTOS; i++) {
            int32_t la, lo;
            interpolar(o->lat, o->lon, d->lat, d->lon, (float)i / PUNTOS, &la, &lo);
            int cxp = MX(la, lo), cyp = MY(la, lo);
            if (i) gfx_linea_punteada(px, py, cxp, cyp, 4, 8, radar_tono(90));
            px = cxp; py = cyp;
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

        snprintf(buf, sizeof buf, "%s %s", rotulos[k], par[k]->ciudad);
        const int an_cod = gfx_ancho_texto(par[k]->iata, 1);
        const int an_txt = gfx_ancho_texto(buf, 1);
        int izq = (k == 0);                       // el origen rotula a la izquierda
        if (izq && pxx - 12 - an_txt < x0) izq = 0;
        if (!izq && pxx + 12 + an_txt > x0 + anc) izq = 1;
        const int cod_x = izq ? pxx - 12 - an_cod : pxx + 12;
        const int txt_x = izq ? pxx - 12 - an_txt : pxx + 12;
        const int base  = (k == 0) ? pyy - 22 : pyy + 6;
        gfx_texto(cod_x, base, par[k]->iata, radar_tono(255), 1);
        gfx_texto(txt_x, base + 14, buf, radar_tono(150), 1);
    }

    // El avion, con su circulo y el triangulito apuntando al rumbo.
    {
        int pxx = MX(ac->lat, ac->lon), pyy = MY(ac->lat, ac->lon);
        const uint8_t c = radar_tono(255);
        gfx_circulo(pxx, pyy, 13, radar_tono(170));
        int t = trig_de_grados(ac->track);
        int s = trig_sen(t), co = trig_cos(t);
        #define RX(a, b) (pxx + ((a) * co - (b) * s) / TRIG_UNO)
        #define RY(a, b) (pyy + ((a) * s + (b) * co) / TRIG_UNO)
        gfx_triangulo_lleno(RX(0, -9), RY(0, -9), RX(6, 8), RY(6, 8), RX(-6, 8), RY(-6, 8), c);
        #undef RX
        #undef RY
        gfx_texto(pxx + 16, pyy - 8, ac->vuelo, c, 1);
    }

    // Barra de arriba: el vuelo a la izquierda y cuanto falta a la derecha.
    snprintf(buf, sizeof buf, "%s", ac->vuelo);
    gfx_texto(X + 10, Y + 8, buf, radar_tono(255), 1);
    snprintf(buf, sizeof buf, "%s > %s", ac->origen, ac->destino);
    gfx_texto(X + 10 + gfx_ancho_texto(ac->vuelo, 1) + 10, Y + 8, buf, radar_tono(170), 1);
    if (d) {
        int km = km_gc(ac->lat, ac->lon, d->lat, d->lon);
        snprintf(buf, sizeof buf, "Ruta del viaje - %d km al destino", km);
    } else {
        snprintf(buf, sizeof buf, "Ruta del viaje");
    }
    gfx_texto(X + AN - 10 - gfx_ancho_texto(buf, 1), Y + 8, buf, radar_tono(170), 1);
    gfx_hlinea(X + 10, Y + 26, AN - 20, radar_tono(45));

    // La tarjeta del vuelo al costado, si la vista la lleva.
    if (radar_vista == VISTA_SEGUIR_HIBRIDA) {
        void tarjeta_dibujar(int x, int y, int an, int al, const avion_t *a, int grande);
        extern int radar_tarjetas_sucias(void);
        vga_limpiar_rect(X + AN + 4, gfx_banda_y0, area.an - AN - 4, gfx_banda_y1, radar_tono(0));
        tarjeta_dibujar(X + AN + 8, Y + 8, area.an - AN - 16, AL - 16, ac, 0);
    }
}
