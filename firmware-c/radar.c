#include "radar.h"
#include "gfx.h"
#include "area.h"
#include "trig.h"
#include "geo.h"
#include "pistas.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

// crt_amber, el tema por defecto de la web: #e8b86d sobre #0a0805.
tema_t radar_tema = { 0xe8, 0xb8, 0x6d, 0x0a, 0x08, 0x05 };

aeropuerto_t radar_apt = { "EZE", "Buenos Aires (Ezeiza)", -348220, -585360, 150 };

avion_t radar_aviones[RADAR_MAX_AVIONES];
int radar_cantidad = 0;
vista_t radar_vista = VISTA_HIBRIDA;
int radar_tarjetas = 3;
int radar_rotacion_s = 8;
char radar_seguir[9] = "";
bool radar_pistas_on = true;

// Cabecera en uso: la que mas aviones tiene alineados aproximando. Se calcula
// una vez por cuadro en radar_avanzar y se dibuja despues.
static const pista_t *senda_pista;
static int senda_es_b;            // 0 = cabecera A, 1 = cabecera B
static int senda_hay;

void tarjeta_dibujar(int x, int y, int an, int al, const avion_t *a, int grande);

// Pagina del carrusel y cuadros que lleva mostrada. Las tarjetas cambian
// todas juntas al pasar de pagina, no de a una: con la lista reordenandose
// cada cuadro se cambiaban solas y quedaba raro.
static int pagina = 0;
static uint32_t cuadros_pagina = 0;

// Las tarjetas cambian cada varios segundos, no cada cuadro. Redibujarlas
// sesenta veces por segundo hacia que el cuadro tardara casi lo mismo que el
// haz en bajar la pantalla, y las de abajo salian rasgadas. Ahora solo se
// vuelven a dibujar cuando hay algo nuevo que mostrar.
static int tarjetas_sucias = 1;

void radar_marcar_sucio(void) { tarjetas_sucias = 1; }

uint8_t radar_tono(int alpha255);

// La barra de arriba, como .mon-head de la web: el aeropuerto a la izquierda
// y el resumen a la derecha, con una linea fina abajo.
static void encabezado(const char *derecha, int ancho) {
    const int X = area.x, Y = area.y, AN = ancho;
    char buf[72];
    snprintf(buf, sizeof buf, "%s  %s", radar_apt.iata, radar_apt.nombre);
    gfx_texto(X + 10, Y + 8, radar_apt.iata, radar_tono(255), 1);
    gfx_texto(X + 10 + gfx_ancho_texto(radar_apt.iata, 1) + 10, Y + 8,
              radar_apt.nombre, radar_tono(170), 1);
    if (derecha && derecha[0])
        gfx_texto(X + AN - 10 - gfx_ancho_texto(derecha, 1), Y + 8, derecha, radar_tono(170), 1);
    gfx_hlinea(X + 10, Y + 26, AN - 20, radar_tono(45));
}

// Version corta, para cuando el encabezado ocupa todo el ancho.
static void encabezado_ancho(const char *derecha) { encabezado(derecha, area.an); }

static int beam = 0;              // angulo del barrido, en unidades de trig.h
static uint8_t orden[RADAR_MAX_AVIONES];   // aviones ordenados por cercania
static uint8_t lista[RADAR_MAX_AVIONES];   // la que ven las tarjetas, congelada
static int lista_n = 0;

// La web usa alpha sobre el color del tema. Aca se mezcla contra el fondo,
// que da el mismo resultado porque el fondo es plano.
uint8_t radar_tono(int alpha255) {
    return gfx_mezcla(radar_tema.fr, radar_tema.fg, radar_tema.fb,
                      radar_tema.br, radar_tema.bg, radar_tema.bb, alpha255);
}

void radar_init(void) {
    trig_init();
    beam = 0;
}

// Diagnostico: cuantas pistas encontro y donde caen en pantalla.
void radar_informar(void) {
    int n = 0;
    const pista_t *p = pistas_de(radar_apt.iata, &n);
    printf("pistas de %s: %d | senda %s\n", radar_apt.iata, n,
           senda_hay ? (senda_es_b ? senda_pista->ident_b : senda_pista->ident_a) : "(ninguna)");
    const int ANS = (radar_vista == VISTA_HIBRIDA) ? area.an * 54 / 100 : area.an;
    const int cx = area.x + ANS / 2, cy = area.y + area.al * 52 / 100;
    const int semi = (ANS / 2 < area.al * 52 / 100 ? ANS / 2 : area.al * 52 / 100);
    const int R = semi * 86 / 100;
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;
    for (int i = 0; i < n; i++) {
        int ax = cx + (int)((int64_t)(p[i].lon_a - radar_apt.lon) * R / span);
        int ay = cy - (int)((int64_t)(p[i].lat_a - radar_apt.lat) * R / span);
        int bx = cx + (int)((int64_t)(p[i].lon_b - radar_apt.lon) * R / span);
        int by = cy - (int)((int64_t)(p[i].lat_b - radar_apt.lat) * R / span);
        printf("  %s/%s en pantalla: %d,%d a %d,%d\n",
               p[i].ident_a, p[i].ident_b, ax, ay, bx, by);
    }
}

// Diferencia angular mas corta, en unidades de vuelta.
static int dif_ang(int a, int b) {
    int d = ((a - b) % TRIG_VUELTA + TRIG_VUELTA) % TRIG_VUELTA;
    return d > TRIG_VUELTA / 2 ? TRIG_VUELTA - d : d;
}

// Avanza el estado un cuadro: el barrido gira y el fosforo de los aviones
// decae. Va aparte del pintado porque el pintado se repite una vez por banda.
// Ordena los aviones por distancia al aeropuerto: los mas cercanos primero.
// Es lo que decide cuales salen en las tarjetas.
static void ordenar_por_cercania(void) {
    int32_t d[RADAR_MAX_AVIONES];
    for (int i = 0; i < radar_cantidad; i++) {
        int32_t dla = radar_aviones[i].lat - radar_apt.lat;
        int32_t dlo = radar_aviones[i].lon - radar_apt.lon;
        d[i] = (dla / 16) * (dla / 16) + (dlo / 16) * (dlo / 16);
        orden[i] = (uint8_t)i;
    }
    for (int i = 1; i < radar_cantidad; i++) {          // insercion, son pocos
        uint8_t v = orden[i];
        int32_t k = d[v];
        int j = i - 1;
        while (j >= 0 && d[orden[j]] > k) { orden[j + 1] = orden[j]; j--; }
        orden[j + 1] = v;
    }
}

// Elige la cabecera en uso con el mismo criterio que la web: se cuentan los
// aviones bajos y lentos cuyo rumbo coincide con el de la cabecera dentro de
// 20 grados y que estan entre 1,5 y 55 km por delante de ella.
static void elegir_senda(void) {
    senda_hay = 0;
    if (!radar_pistas_on) return;
    int cuantas = 0;
    const pista_t *p = pistas_de(radar_apt.iata, &cuantas);
    if (!p) return;

    int mejor = 0;
    for (int i = 0; i < cuantas; i++) {
        for (int lado = 0; lado < 2; lado++) {
            int hdg      = lado ? p[i].hdg_b : p[i].hdg_a;
            int32_t clat = lado ? p[i].lat_b : p[i].lat_a;
            int32_t clon = lado ? p[i].lon_b : p[i].lon_a;
            if (!hdg) continue;
            int n = 0;
            for (int k = 0; k < radar_cantidad; k++) {
                const avion_t *a = &radar_aviones[k];
                if (a->alt > 15000 || a->gs < 70) continue;
                if (geo_dif_rumbo(a->track, hdg) > 20) continue;
                int km = geo_km(a->lat, a->lon, clat, clon);
                int haciala = geo_rumbo(a->lat, a->lon, clat, clon);
                // Solo cuenta si viene de frente, no si ya paso la cabecera.
                if (geo_dif_rumbo(a->track, haciala) > 45) continue;
                if (km <= 1 || km >= 55) continue;
                n++;
            }
            if (n > mejor) { mejor = n; senda_pista = &p[i]; senda_es_b = lado; senda_hay = 1; }
        }
    }
}

// Guarda una posicion en el rastro, mas o menos una vez por segundo.
static void anotar_rastro(void) {
    static uint32_t cuenta = 0;
    if (++cuenta % 60) return;
    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        if (a->rastro_n < RADAR_RASTRO) {
            a->rastro_lat[a->rastro_n] = a->lat;
            a->rastro_lon[a->rastro_n] = a->lon;
            a->rastro_n++;
        } else {
            for (int k = 1; k < RADAR_RASTRO; k++) {
                a->rastro_lat[k - 1] = a->rastro_lat[k];
                a->rastro_lon[k - 1] = a->rastro_lon[k];
            }
            a->rastro_lat[RADAR_RASTRO - 1] = a->lat;
            a->rastro_lon[RADAR_RASTRO - 1] = a->lon;
        }
    }
}

void radar_avanzar(void) {
    anotar_rastro();
    ordenar_por_cercania();
    elegir_senda();

    // El carrusel cambia de pagina cada tantos segundos, no cada cuadro, y la
    // lista se congela mientras la pagina esta a la vista: si se reordenara
    // en vivo las tarjetas se irian cambiando de a una, que es lo que pasaba.
    if (lista_n == 0 || ++cuadros_pagina >= (uint32_t)(radar_rotacion_s * 60)) {
        cuadros_pagina = 0;
        const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
        const int paginas = (radar_cantidad + por_pagina - 1) / por_pagina;
        if (paginas > 0 && lista_n) pagina = (pagina + 1) % paginas;
        lista_n = radar_cantidad;
        for (int i = 0; i < radar_cantidad; i++) lista[i] = orden[i];
        tarjetas_sucias = 1;
    }
    // El barrido avanza 0,012 radianes por cuadro = 1,96 unidades de 1024.
    beam = (beam + 2) % TRIG_VUELTA;

    const int cx = area.x + area.an / 2;
    const int cy = area.y + area.al * 52 / 100;
    const int semi = (area.an / 2 < area.al * 52 / 100 ? area.an / 2 : area.al * 52 / 100);
    const int R = semi * 86 / 100;
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;

    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = cx + (int)((int64_t)(a->lon - radar_apt.lon) * R / span);
        int y = cy - (int)((int64_t)(a->lat - radar_apt.lat) * R / span);
        int ang = trig_atan2(y - cy, x - cx);
        if (dif_ang(ang, beam) < 8) a->brillo = 255;
        else if (a->brillo > 82) a->brillo -= 1;          // 0,0024 por cuadro
    }
}

void radar_pintar_tarjetas(int x, int y, int an, int al, int grande);
void radar_pintar_viaje(void);

static void radar_pintar(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    const uint8_t fondo = radar_tono(0);

    // Geometria igual a la de la web. En la vista hibrida el scope ocupa el
    // 54 por ciento del ancho y las tarjetas el resto, igual que left=0.54.
    const int ANS = (radar_vista == VISTA_HIBRIDA) ? AN * 54 / 100 : AN;

    // Se limpia solo la banda que se esta dibujando, y solo el ancho del
    // scope: la columna de las tarjetas queda como estaba.
    vga_limpiar_rect(0, gfx_banda_y0, X + ANS + 4, gfx_banda_y1, fondo);
    const int cx = X + ANS / 2;
    const int cy = Y + 30 + (AL - 30) * 52 / 100;
    const int semi = (ANS / 2 < (AL - 30) * 52 / 100 ? ANS / 2 : (AL - 30) * 52 / 100);
    const int R = semi * 86 / 100;

    // Cuna del barrido: 0,28 radianes = 46 unidades. Va primero para que los
    // anillos y los aviones queden por encima, como en la web.
    gfx_sector(cx, cy, R, beam - 46, beam, radar_tono(36));     // alpha 0,14
    gfx_linea(cx, cy, cx + trig_cos(beam) * R / TRIG_UNO,
                      cy + trig_sen(beam) * R / TRIG_UNO, radar_tono(102));  // alpha 0,4

    // Anillos de alcance y cruz, alpha 0x28 como en la web.
    const uint8_t rejilla = radar_tono(40);
    for (int i = 1; i <= 4; i++) gfx_circulo(cx, cy, R * i / 4, rejilla);
    gfx_linea(cx, cy - R, cx, cy + R, rejilla);
    gfx_linea(cx - R, cy, cx + R, cy, rejilla);

    // Barra de arriba con el resumen, igual que la web.
    {
        char meta[72];
        snprintf(meta, sizeof meta, "%d en radar - %d km", radar_cantidad, radar_apt.radio_km);
        // En la vista hibrida la barra ocupa solo el ancho del scope: si no,
        // el resumen de la derecha cae encima de la primera tarjeta.
        encabezado(meta, ANS);
    }

    // Etiquetas de alcance sobre el eje horizontal.
    char km[12];
    for (int i = 1; i <= 4; i++) {
        snprintf(km, sizeof km, "%d", radar_apt.radio_km * i / 4);
        gfx_texto(cx + R * i / 4 - gfx_ancho_texto(km, 1) - 2, cy + 3, km, rejilla, 1);
    }
    gfx_texto(cx + R - gfx_ancho_texto("km", 1) - 2, cy - 14, "km", rejilla, 1);

    // Grados de 1e-4 que entran en el radio del scope: 1 grado = 111 km.
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;

    #define PROY_X(la, lo) (cx + (int)((int64_t)((lo) - radar_apt.lon) * R / span))
    #define PROY_Y(la, lo) (cy - (int)((int64_t)((la) - radar_apt.lat) * R / span))

    // Pistas del aeropuerto. Las cortas se estiran a un largo minimo, si no
    // a este alcance no se ven; es lo mismo que hace minLen en la web.
    if (radar_pistas_on) {
        int cuantas = 0;
        const pista_t *p = pistas_de(radar_apt.iata, &cuantas);
        const int minlen = R * 75 / 1000 < 9 ? 9 : (R * 75 / 1000 > 30 ? 30 : R * 75 / 1000);
        for (int i = 0; i < cuantas; i++) {
            int ax = PROY_X(p[i].lat_a, p[i].lon_a), ay = PROY_Y(p[i].lat_a, p[i].lon_a);
            int bx = PROY_X(p[i].lat_b, p[i].lon_b), by = PROY_Y(p[i].lat_b, p[i].lon_b);
            int dx = bx - ax, dy = by - ay;
            int largo = 0;
            while ((largo + 1) * (largo + 1) <= dx * dx + dy * dy) largo++;
            if (largo > 0 && largo < minlen) {          // estirar desde el medio
                int mx = (ax + bx) / 2, my = (ay + by) / 2;
                ax = mx - dx * minlen / (2 * largo); ay = my - dy * minlen / (2 * largo);
                bx = mx + dx * minlen / (2 * largo); by = my + dy * minlen / (2 * largo);
            }
            int en_uso = senda_hay && senda_pista == &p[i];
            const uint8_t c = radar_tono(en_uso ? 250 : 180);
            // Grosor: dos o tres pasadas, que la fuente de lineas es de 1 px.
            gfx_linea(ax, ay, bx, by, c);
            gfx_linea(ax, ay + 1, bx, by + 1, c);
            if (en_uso) gfx_linea(ax, ay - 1, bx, by - 1, c);
        }

        // Senda de aproximacion: punteada desde donde viene el avion hasta la
        // cabecera en uso, con el cartel al lado.
        if (senda_hay) {
            int hdg      = senda_es_b ? senda_pista->hdg_b : senda_pista->hdg_a;
            int32_t clat = senda_es_b ? senda_pista->lat_b : senda_pista->lat_a;
            int32_t clon = senda_es_b ? senda_pista->lon_b : senda_pista->lon_a;
            const char *ident = senda_es_b ? senda_pista->ident_b : senda_pista->ident_a;
            // Largo de la senda, igual que approachKm() en la web: 35 por
            // ciento del alcance, entre 18 y 32 km.
            int apxkm = radar_apt.radio_km * 35 / 100;
            if (apxkm < 18) apxkm = 18;
            if (apxkm > 32) apxkm = 32;
            int32_t slat, slon;
            geo_proyectar(clat, clon, (hdg + 180) % 360, apxkm, &slat, &slon);
            int sx = PROY_X(slat, slon), sy = PROY_Y(slat, slon);
            int tx2 = PROY_X(clat, clon), ty2 = PROY_Y(clat, clon);
            // Si en pantalla queda mas corta que 64 px no se lee: se estira,
            // igual que hace la web con plen < 64.
            int ddx = sx - tx2, ddy = sy - ty2;
            int plen = 0;
            while ((plen + 1) * (plen + 1) <= ddx * ddx + ddy * ddy) plen++;
            if (plen > 0 && plen < 64) {
                sx = tx2 + ddx * 64 / plen;
                sy = ty2 + ddy * 64 / plen;
            }
            const uint8_t c = radar_tono(245);
            gfx_linea_punteada(sx, sy, tx2, ty2, 8, 6, c);
            gfx_linea_punteada(sx, sy + 1, tx2, ty2 + 1, 8, 6, c);
            char cartel[24];
            snprintf(cartel, sizeof cartel, "APX %s EN USO", ident);
            gfx_texto(sx + 6, sy - 16, cartel, c, 1);
            // Y el aviso arriba a la derecha del scope, como en la web.
            snprintf(cartel, sizeof cartel, "SENDA %s EN USO", ident);
            gfx_texto(X + ANS - gfx_ancho_texto(cartel, 1) - 10, Y + 32, cartel, c, 1);
        }
    }

    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = cx + (int)((int64_t)(a->lon - radar_apt.lon) * R / span);
        int y = cy - (int)((int64_t)(a->lat - radar_apt.lat) * R / span);
        // Solo adentro del circulo: si no, quedan aviones sueltos flotando
        // en las esquinas, fuera del alcance que dice el radar.
        int rdx = x - cx, rdy = y - cy;
        if (rdx * rdx + rdy * rdy > R * R) continue;
        // Si ni el avion ni su etiqueta caen en esta banda, no hay nada que
        // hacer: el cuadro se pinta una vez por banda y esto se salta cuatro
        // quintos del trabajo.
        if (y + 8 < gfx_banda_y0 || y - 14 > gfx_banda_y1) continue;

        const uint8_t c = radar_tono(a->brillo);

        // Triangulito apuntando al rumbo. En la web: (0,-6) (3.6,5) (-3.6,5).
        int t = trig_de_grados(a->track);
        int s = trig_sen(t), co = trig_cos(t);
        // Rotacion: el eje del avion es -Y, o sea que hay que girar el vector.
        // px y py van en pixeles: la unica division por TRIG_UNO es la que
        // saca la escala del seno y el coseno.
        #define ROT_X(px, py) (x + ((px) * co - (py) * s) / TRIG_UNO)
        #define ROT_Y(px, py) (y + ((px) * s + (py) * co) / TRIG_UNO)
        gfx_triangulo_lleno(ROT_X(0, -6),  ROT_Y(0, -6),
                            ROT_X(4,  5),  ROT_Y(4,  5),
                            ROT_X(-4, 5),  ROT_Y(-4, 5), c);
        #undef ROT_X
        #undef ROT_Y
        // En la vista hibrida solo llevan nombre los que salen en las
        // tarjetas, igual que labeled.has(a.hex) en la web: con todos
        // etiquetados el scope se vuelve ilegible.
        int etiquetar = (radar_vista != VISTA_HIBRIDA);
        const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
        for (int k = 0; !etiquetar && k < por_pagina && k < radar_cantidad; k++)
            if (lista_n && lista[(pagina * por_pagina + k) % lista_n] == i) etiquetar = 1;
        if (etiquetar && a->vuelo[0]) gfx_texto(x + 8, y - 12, a->vuelo, c, 1);
    }

    if (radar_vista != VISTA_HIBRIDA || !tarjetas_sucias) return;
    vga_limpiar_rect(X + ANS + 4, gfx_banda_y0, AN - ANS - 4, gfx_banda_y1, fondo);
    radar_pintar_tarjetas(X + ANS + 8, Y + 32, AN - ANS - 16, AL - 36, 0);
}

// La columna, o la pared entera. Muestra la pagina que toca del carrusel.
void radar_pintar_tarjetas(int x, int y, int an, int al, int grande) {
    const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
    const int cuantas = radar_cantidad < por_pagina ? radar_cantidad : por_pagina;
    if (cuantas <= 0) return;
    const int sep = 6;
    const int tal = (al - (cuantas - 1) * sep) / cuantas;
    for (int i = 0; i < cuantas; i++) {
        int k = (pagina * por_pagina + i) % (lista_n ? lista_n : 1);
        int ty = y + i * (tal + sep);
        if (ty + tal < gfx_banda_y0 || ty > gfx_banda_y1) continue;
        tarjeta_dibujar(x, ty, an, tal, &radar_aviones[lista[k]], grande);
    }
}

// Un cuadro entero, por bandas de arriba hacia abajo. Ver gfx.h: es lo que
// evita que se pierda lo que se dibuja en la parte de arriba.
// Cuantas bandas. Cuanto mas finas, menos margen hay entre lo que se dibuja
// al final de una banda y el haz que ya viene pasando por sus primeras filas:
// con cuatro, el texto de la tarjeta de arriba salia cortado por la mitad.
#define RADAR_BANDAS 10

static void pintar_pared(void) {
    if (!tarjetas_sucias) return;
    vga_limpiar_filas(gfx_banda_y0, gfx_banda_y1, radar_tono(0));
    {
        char meta[72];
        const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
        const int paginas = (radar_cantidad + por_pagina - 1) / por_pagina;
        snprintf(meta, sizeof meta, "%d vuelos - pagina %d de %d",
                 radar_cantidad, pagina + 1, paginas > 0 ? paginas : 1);
        encabezado_ancho(meta);
    }
    radar_pintar_tarjetas(area.x + 8, area.y + 36, area.an - 16, area.al - 44, 1);
}

void radar_cuadro(void) {
    const int alto = (VGA_ALTO + RADAR_BANDAS - 1) / RADAR_BANDAS;
    for (int b = 0; b < RADAR_BANDAS; b++) {
        int y0 = b * alto;
        int y1 = y0 + alto - 1;
        if (y1 > VGA_ALTO - 1) y1 = VGA_ALTO - 1;
        gfx_banda(y0, y1);
        switch (radar_vista) {
            case VISTA_PARED:  pintar_pared(); break;
            case VISTA_SEGUIR: radar_pintar_viaje(); break;
            default:           radar_pintar(); break;
        }
    }
    gfx_banda(0, VGA_ALTO - 1);
    tarjetas_sucias = 0;
}
