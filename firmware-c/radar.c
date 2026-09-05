#include "radar.h"
#include <math.h>
#include "gfx.h"
#include "area.h"
#include "trig.h"
#include "geo.h"
#include "pistas.h"
#include "logos.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

// crt_amber, el tema por defecto de la web: #e8b86d sobre #0a0805.
tema_t radar_tema = { 0xe8, 0xb8, 0x6d, 0x0a, 0x08, 0x05 };
lista_t radar_lista = LISTA_TARJETAS;

// Los catorce temas de THEMES en docs/radar.js, con los mismos colores.
const tema_nombrado_t radar_temas[] = {
    { "crt_amber", { 0xe8, 0xb8, 0x6d, 0x0a, 0x08, 0x05 } },
    { "crt_green", { 0xb4, 0xff, 0x50, 0x03, 0x11, 0x05 } },
    { "phosphor",  { 0xc6, 0xf5, 0x9a, 0x03, 0x08, 0x05 } },
    { "atc_dark",  { 0x7e, 0xc8, 0xe3, 0x07, 0x09, 0x0c } },
    { "navy",      { 0xb4, 0xd2, 0xff, 0x06, 0x10, 0x18 } },
    { "violet",    { 0xc4, 0xa8, 0xff, 0x0c, 0x08, 0x14 } },
    { "magenta",   { 0xff, 0x7a, 0xd9, 0x12, 0x08, 0x14 } },
    { "red",       { 0xff, 0x9a, 0x8c, 0x12, 0x06, 0x06 } },
    { "orange",    { 0xff, 0x9a, 0x4a, 0x12, 0x08, 0x04 } },
    { "gold",      { 0xff, 0xd5, 0x6a, 0x0c, 0x0a, 0x04 } },
    { "ice",       { 0xd9, 0xf6, 0xff, 0x08, 0x10, 0x16 } },
    { "cyan",      { 0x5e, 0xea, 0xd4, 0x04, 0x12, 0x10 } },
    { "olive",     { 0xb7, 0xc4, 0x7a, 0x0c, 0x0e, 0x08 } },
    { "rose",      { 0xff, 0x8f, 0xab, 0x14, 0x08, 0x0c } },
};
const int radar_temas_cant = (int)(sizeof radar_temas / sizeof radar_temas[0]);

void radar_tema_poner(const char *nombre) {
    for (int i = 0; i < radar_temas_cant; i++)
        if (!strcmp(radar_temas[i].nombre, nombre)) { radar_tema = radar_temas[i].tema; return; }
}

aeropuerto_t radar_apt = { "EZE", "Buenos Aires (Ezeiza)", -348220, -585360, 150 };

avion_t radar_aviones[RADAR_MAX_AVIONES];
int radar_cantidad = 0;
vista_t radar_vista = VISTA_HIBRIDA;
int radar_tarjetas = 3;
int radar_rotacion_s = 8;
char radar_seguir[9] = "";

// La casa, con sus propias coordenadas: puede estar a cualquier distancia del
// aeropuerto que muestra el radar.
bool    radar_casa_on = false;
int32_t radar_casa_lat = 0, radar_casa_lon = 0;
int     radar_casa_km = 3;
char    radar_casa_nombre[20] = "CASA";
bool radar_pistas_on = true;

// Cabecera en uso: la que mas aviones tiene alineados aproximando. Se calcula
// una vez por cuadro en radar_avanzar y se dibuja despues.
// Se prende cuando cambia el texto del encabezado, o al cambiar de vista: es
// lo unico que obliga a rehacerlo. Rehacerlo en cada cuadro era casi todo lo
// que costaban las dos bandas de arriba, que son las de menos margen.
static int encabezado_sucio = 1;
static const pista_t *senda_pista;
static int senda_es_b;            // 0 = cabecera A, 1 = cabecera B
static int senda_hay;
int radar_senda_rumbo = -1;
static char senda_cartel[32];     // el aviso de arriba, listo para dibujar
static int casa_encima = -1;      // avion que esta pasando sobre la casa

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
// Cuantas tarjetas quedan por redibujar. Se hace de a una por cuadro: al
// rehacerlas todas juntas el cuadro se iba a veinte milisegundos, el haz lo
// alcanzaba y se veia el tiron cada vez que cambiaba la pagina.
static int tarjetas_sucias = 99;
static int tarjeta_en_curso = 0;

static int limpiar_todo = 1;

void radar_marcar_sucio(void) {
    tarjetas_sucias = 99;
    tarjeta_en_curso = 0;
    // Cada vista limpia solo su parte, asi que al cambiar de una a otra
    // quedaban restos de la anterior: pedazos del mapa detras de las
    // tarjetas y al costado del scope.
    limpiar_todo = 1;
    encabezado_sucio = 1;
}

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

// El carrusel de la lista se guarda y se repone al cambiar de pantalla: si se
// reiniciara, volviendo a la lista de vuelos siempre se verian los mismos
// cuatro. Lo que se quiere es que siga donde iba: primero el 1 al 4, y en la
// vuelta siguiente el 5 al 8, hasta completar todos.
int radar_pagina_actual(void) { return pagina; }

void radar_carrusel_guardar(int *pag, uint32_t *cuadros) {
    *pag = pagina;
    *cuadros = cuadros_pagina;
}

void radar_carrusel_poner(int pag, uint32_t cuadros) {
    pagina = pag;
    cuadros_pagina = cuadros;
    // La lista se rearma con la pagina repuesta, pero SIN avanzarla: la
    // pagina avanza por el paso del tiempo, no por cambiar de pantalla.
    // Poniendo lista_n en cero se entraba por la rama de arriba, que resetea
    // el reloj y no incrementa la pagina, y entonces cada visita mostraba los
    // mismos cuatro vuelos.
    lista_n = radar_cantidad;
    for (int i = 0; i < radar_cantidad; i++) lista[i] = orden[i];
    const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
    const int paginas = (radar_cantidad + por_pagina - 1) / por_pagina;
    if (paginas > 0) pagina %= paginas;    // el aeropuerto pudo cambiar
    tarjetas_sucias = 1;
}


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
            if (n > mejor) {
                mejor = n; senda_pista = &p[i]; senda_es_b = lado; senda_hay = 1;
                snprintf(senda_cartel, sizeof senda_cartel, "%s EN USO",
                         lado ? p[i].ident_b : p[i].ident_a);
            }
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
    if (radar_vista == VISTA_LOGOS) { void logos_pantalla_avanzar(void); logos_pantalla_avanzar(); }
    anotar_rastro();
    ordenar_por_cercania();
    elegir_senda();

    // Quien esta pasando sobre la casa. Se elige el mas cercano dentro del
    // radio, y se sostiene hasta que se va: el aviso de abajo a la izquierda
    // es la razon de ser de la casa, y hasta ahora nunca se prendia porque
    // esto no se calculaba en ningun lado.
    casa_encima = -1;
    if (radar_casa_on) {
        int mejor = radar_casa_km + 1;
        for (int i = 0; i < radar_cantidad; i++) {
            int d = geo_km(radar_aviones[i].lat, radar_aviones[i].lon,
                           radar_casa_lat, radar_casa_lon);
            if (d <= radar_casa_km && d < mejor) { mejor = d; casa_encima = i; }
        }
    }

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

    const int coslat = geo_coslat(radar_apt.lat);
    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = cx + (int)((int64_t)(a->lon - radar_apt.lon) * coslat / TRIG_UNO * R / span);
        int y = cy - (int)((int64_t)(a->lat - radar_apt.lat) * R / span);
        int ang = trig_atan2(y - cy, x - cx);
        if (dif_ang(ang, beam) < 8) a->brillo = 255;
        else if (a->brillo > 82) a->brillo -= 1;          // 0,0024 por cuadro
    }
}

void radar_pintar_tarjetas(int x, int y, int an, int al, int grande);
void radar_pintar_viaje(void);
void radar_viaje_avion(void);

static void radar_pintar(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    const uint8_t fondo = radar_tono(0);

    // Geometria igual a la de la web. En la vista hibrida el scope ocupa el
    // 54 por ciento del ancho y las tarjetas el resto, igual que left=0.54.
    const int ANS = (radar_vista == VISTA_HIBRIDA) ? AN * 54 / 100 : AN;

    // El encabezado no cambia casi nunca, y borrarlo y rehacerlo en cada
    // cuadro era casi todo lo que costaban las dos bandas de arriba, que son
    // las que menos margen tienen contra el haz. Se rehace solo cuando su
    // texto cambia.
    char meta[72];
    snprintf(meta, sizeof meta, "%d en radar - %d km", radar_cantidad, radar_apt.radio_km);
    static char meta_visto[72];
    if (gfx_banda_y0 == 0 && strncmp(meta, meta_visto, sizeof meta)) {
        snprintf(meta_visto, sizeof meta_visto, "%s", meta);
        encabezado_sucio = 1;
    }
    const int y_bajo_enc = Y + 30;

    // Se limpia solo la banda que se esta dibujando, y solo el ancho del
    // scope: la columna de las tarjetas queda como estaba.
    {
        int ly0 = gfx_banda_y0;
        if (!encabezado_sucio && ly0 < y_bajo_enc) ly0 = y_bajo_enc;
        if (ly0 <= gfx_banda_y1)
            vga_limpiar_rect(0, ly0, X + ANS + 4, gfx_banda_y1, fondo);
    }
    const int cx = X + ANS / 2;
    const int cy = Y + 30 + (AL - 30) * 52 / 100;
    const int semi = (ANS / 2 < (AL - 30) * 52 / 100 ? ANS / 2 : (AL - 30) * 52 / 100);
    const int R = semi * 86 / 100;

    // El encabezado va primero: esta en las filas de mas arriba y el haz
    // llega ahi antes que a ninguna otra cosa. Dibujarlo al final de la
    // banda lo dejaba llegando tarde, y titilaba.
    // Barra de arriba con el resumen, igual que la web.
    {
        // En la vista hibrida la barra ocupa solo el ancho del scope: si no,
        // el resumen de la derecha cae encima de la primera tarjeta.
        if (encabezado_sucio) encabezado(meta, ANS);
        // El aviso va con el encabezado, no al final: esta en las filas de
        // arriba y dibujarlo despues de los anillos lo dejaba llegando tarde,
        // cortado por la mitad.
        if (senda_hay && radar_pistas_on)
            gfx_texto(X + ANS - gfx_ancho_texto(senda_cartel, 1) - 10, Y + 32,
                      senda_cartel, radar_tono(245), 1);
    }

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

    // Etiquetas de alcance sobre el eje horizontal.
    char km[12];
    for (int i = 1; i <= 4; i++) {
        snprintf(km, sizeof km, "%d", radar_apt.radio_km * i / 4);
        gfx_texto(cx + R * i / 4 - gfx_ancho_texto(km, 1) - 2, cy + 3, km, rejilla, 1);
    }
    gfx_texto(cx + R - gfx_ancho_texto("km", 1) - 2, cy - 14, "km", rejilla, 1);

    // Grados de 1e-4 que entran en el radio del scope: 1 grado = 111 km.
    const int32_t span = (int32_t)radar_apt.radio_km * 10000 / 111;

    // La longitud se achica por el coseno de la latitud. Sin esto las
    // distancias este-oeste salen infladas y los rumbos se ven torcidos: la
    // aproximacion a la 11 de Ezeiza parecia entrar por la 09.
    const int coslat = geo_coslat(radar_apt.lat);

    #define PROY_X(la, lo) (cx + (int)((int64_t)((lo) - radar_apt.lon) * coslat / TRIG_UNO * R / span))
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

        // Senda de aproximacion. Se dibuja extendiendo el eje de la pista tal
        // como quedo en pantalla, no proyectando por lat/lon: proyectar
        // corrige por el coseno de la latitud y la pista no, asi que la senda
        // salia torcida respecto de la pista. La web tiene el mismo defecto.
        if (senda_hay) {
            const int lado = senda_es_b;
            const char *ident = lado ? senda_pista->ident_b : senda_pista->ident_a;
            // Punta donde se aterriza y la de enfrente.
            int tx2 = lado ? PROY_X(senda_pista->lat_b, senda_pista->lon_b)
                           : PROY_X(senda_pista->lat_a, senda_pista->lon_a);
            int ty2 = lado ? PROY_Y(senda_pista->lat_b, senda_pista->lon_b)
                           : PROY_Y(senda_pista->lat_a, senda_pista->lon_a);
            // La direccion se saca en subpixeles y no de las puntas ya
            // redondeadas: a 80 km de alcance la pista mide 13 px en
            // pantalla, y un pixel de error en la punta torcia la senda
            // hasta cuatro grados. Eso es lo que se veia como que la
            // aproximacion no coincidia con el rumbo magnetico.
            #define SUB 256
            #define PROY_XS(la, lo) ((int)((int64_t)((lo) - radar_apt.lon) * coslat * SUB * R / TRIG_UNO / span))
            #define PROY_YS(la, lo) (-(int)((int64_t)((la) - radar_apt.lat) * SUB * R / span))
            int sxb = lado ? PROY_XS(senda_pista->lat_b, senda_pista->lon_b)
                           : PROY_XS(senda_pista->lat_a, senda_pista->lon_a);
            int syb = lado ? PROY_YS(senda_pista->lat_b, senda_pista->lon_b)
                           : PROY_YS(senda_pista->lat_a, senda_pista->lon_a);
            int sxa = lado ? PROY_XS(senda_pista->lat_a, senda_pista->lon_a)
                           : PROY_XS(senda_pista->lat_b, senda_pista->lon_b);
            int sya = lado ? PROY_YS(senda_pista->lat_a, senda_pista->lon_a)
                           : PROY_YS(senda_pista->lat_b, senda_pista->lon_b);

            // El avion viene por detras de la cabecera donde aterriza: la
            // senda sale hacia el lado opuesto al que apunta la pista desde
            // esa punta. Con el signo al reves salia del lado de adentro de
            // la pista, apuntando a donde no va nadie.
            int vx = sxb - sxa, vy = syb - sya;
            // Rumbo con el que sale la senda, para poder compararlo con el
            // numero de la cabecera sin depender de una foto del monitor.
            radar_senda_rumbo = (int)lrintf(atan2f((float)-vx, (float)vy)
                                            * 180.0f / (float)M_PI + 360.0f) % 360;
            int largo_pista = 0;
            {   // Raiz entera por bisección: en subpixeles el largo llega a
                // varios miles y sumar de a uno costaba milisegundos.
                int hi = 1 << 15, lo2 = 0;
                int64_t q = (int64_t)vx * vx + (int64_t)vy * vy;
                while (lo2 < hi) {
                    int m = (lo2 + hi + 1) / 2;
                    if ((int64_t)m * m <= q) lo2 = m; else hi = m - 1;
                }
                largo_pista = lo2;
            }
            if (largo_pista > 0) {
                // Largo de la senda igual que approachKm() en la web: 35 por
                // ciento del alcance, entre 18 y 32 km; y nunca menos de 64
                // pixeles, que si no no se lee.
                int apxkm = radar_apt.radio_km * 35 / 100;
                if (apxkm < 18) apxkm = 18;
                if (apxkm > 32) apxkm = 32;
                int largo_px = apxkm * R / radar_apt.radio_km;
                if (largo_px < 64) largo_px = 64;
                int sx = tx2 + vx * largo_px / largo_pista;
                int sy = ty2 + vy * largo_px / largo_pista;

                const uint8_t c = radar_tono(245);
                gfx_linea_punteada(sx, sy, tx2, ty2, 8, 6, c);
                gfx_linea_punteada(sx, sy + 1, tx2, ty2 + 1, 8, 6, c);
                char cartel[32];
                snprintf(cartel, sizeof cartel, "%s", ident);
                gfx_texto(sx + 6, sy - 16, cartel, c, 1);
            }
        }
    }

    for (int i = 0; i < radar_cantidad; i++) {
        avion_t *a = &radar_aviones[i];
        int x = PROY_X(a->lat, a->lon);
        int y = PROY_Y(a->lat, a->lon);
        // Solo adentro del circulo: si no, quedan aviones sueltos flotando
        // en las esquinas, fuera del alcance que dice el radar.
        int rdx = x - cx, rdy = y - cy;
        if (rdx * rdx + rdy * rdy > R * R) continue;
        // Si ni el avion ni su etiqueta caen en esta banda, no hay nada que
        // hacer: el cuadro se pinta una vez por banda y esto se salta cuatro
        // quintos del trabajo.
        if (y + 8 < gfx_banda_y0 || y - 14 > gfx_banda_y1) continue;

        // El que pasa sobre la casa va de otro color, para que salte a la
        // vista sin taparle nada alrededor.
        const uint8_t c = (i == casa_encima) ? vga_rgb(0xff, 0xc8, 0x3c)
                                             : radar_tono(a->brillo);

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

    // Sobre la casa: el vuelo que esta pasando por encima de la ubicacion
    // propia se marca con un circulo y saca su tarjeta chica abajo a la
    // izquierda, hasta que se va del radio. La casa se dibuja siempre, para
    // saber donde esta respecto del aeropuerto.
    if (radar_casa_on) {

        int hx = PROY_X(radar_casa_lat, radar_casa_lon);
        int hy = PROY_Y(radar_casa_lat, radar_casa_lon);
        int hdx = hx - cx, hdy = hy - cy;
        if (hdx * hdx + hdy * hdy <= R * R) {
            const uint8_t c3 = radar_tono(200);
            // Una casita: cuadrado con techo.
            gfx_rect(hx - 4, hy - 2, 9, 7, c3);
            gfx_linea(hx - 6, hy - 2, hx, hy - 7, c3);
            gfx_linea(hx, hy - 7, hx + 6, hy - 2, c3);
            // Ni nombre ni circulo de radio: la casita sola se entiende, y
            // el circulo ensuciaba el scope. Quien esta encima se sabe por el
            // color del avion y por el aviso de abajo a la izquierda.
        }

        const int encima = casa_encima;
        if (encima >= 0) {
            const avion_t *a = &radar_aviones[encima];
            const uint8_t c2 = vga_rgb(0xff, 0xc8, 0x3c);
            char aviso[40];
            snprintf(aviso, sizeof aviso, "SOBRE %s", radar_casa_nombre);

            // Tarjeta chica abajo a la izquierda, mientras siga encima.
            const int tw = 250, th = 92;
            const int tx3 = X + 8, ty3 = Y + AL - th - 8;
            vga_limpiar_rect(tx3, ty3 > gfx_banda_y0 ? ty3 : gfx_banda_y0, tw,
                             (ty3 + th < gfx_banda_y1) ? ty3 + th : gfx_banda_y1, fondo);
            gfx_rect(tx3, ty3, tw, th, c2);
            gfx_texto(tx3 + 8, ty3 + 6, aviso, c2, 1);
            const uint8_t *logo = logo_buscar(a->aerolinea);
            if (logo) gfx_blit(tx3 + 8, ty3 + 22, LOGO_LADO, LOGO_LADO, logo);
            gfx_texto(tx3 + 8 + LOGO_LADO + 8, ty3 + 24, a->vuelo, c2, 1);
            char linea[40];
            snprintf(linea, sizeof linea, "%s > %s", a->origen, a->destino);
            gfx_texto(tx3 + 8 + LOGO_LADO + 8, ty3 + 40, linea, radar_tono(190), 1);
            if (a->alt <= 0) snprintf(linea, sizeof linea, "EN TIERRA");
            else snprintf(linea, sizeof linea, "FL%03d  %d kt  RUMBO %03d",
                          (int)(a->alt / 100), a->gs, a->track);
            gfx_texto(tx3 + 8, ty3 + 66, linea, radar_tono(190), 1);
        }
    }

    // Las tarjetas NO se dibujan aca: ver radar_cuadro. Estan arriba, y
    // dibujarlas mientras el haz esta llegando arriba lo hacia alcanzar al
    // dibujo por hasta dos milisegundos: esa franja salia rota y titilando.
}

// La columna, o la pared entera. Muestra la pagina que toca del carrusel, o
// la tabla de llegadas si esta elegido el formato aeropuerto.
void radar_pintar_tarjetas(int x, int y, int an, int al, int grande) {
    const int por_pagina = radar_tarjetas < 1 ? 1 : radar_tarjetas;
    const int cuantas = radar_cantidad < por_pagina ? radar_cantidad : por_pagina;
    if (cuantas <= 0) return;

    if (radar_lista == LISTA_FIDS) {
        void fids_dibujar(int x, int y, int an, int al, const avion_t **v, int n,
                          int paso, int pasos);
        const avion_t *v[RADAR_MAX_AVIONES];
        int n = radar_cantidad < 12 ? radar_cantidad : 12;
        for (int i = 0; i < n; i++) v[i] = &radar_aviones[lista_n ? lista[i] : i];
        // Cuatro pasadas, una por cuadro, para no hacer toda la tabla junta.
        if (tarjeta_en_curso == 0)
            vga_limpiar_rect(x - 4, gfx_banda_y0, an + 8, gfx_banda_y1, radar_tono(0));
        fids_dibujar(x, y, an, al, v, n, tarjeta_en_curso % 8, 8);
        return;
    }
    const int sep = 6;
    const int tal = (al - (cuantas - 1) * sep) / cuantas;
    for (int i = 0; i < cuantas; i++) {
        // De a una por cuadro, salvo que se pidan todas (cambio de vista).
        if (tarjeta_en_curso < cuantas && i != tarjeta_en_curso) continue;
        int k = (pagina * por_pagina + i) % (lista_n ? lista_n : 1);
        int ty = y + i * (tal + sep);
        if (ty + tal < gfx_banda_y0 || ty > gfx_banda_y1) continue;
        vga_limpiar_rect(x - 4, ty < gfx_banda_y0 ? gfx_banda_y0 : ty,
                         an + 8, (ty + tal > gfx_banda_y1) ? gfx_banda_y1 : ty + tal,
                         radar_tono(0));
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
    // Solo en la primera pasada: el dibujo se reparte en varios cuadros y
    // limpiar en todos borraba lo que habian dibujado los anteriores.
    if (tarjeta_en_curso == 0)
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

uint32_t radar_us_banda[RADAR_BANDAS];
int32_t radar_us_antes;
int32_t radar_margen_banda[RADAR_BANDAS] = {
    0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff,
    0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };

// Las bandas no son todas iguales: arriba son finas y abajo anchas.
// Al arrancar solo se lleva de ventaja el borrado vertical, mil cuatrocientos
// microsegundos, y una banda pareja de cuarenta y ocho filas ya costaba mas
// que eso. Cuanto mas abajo, mas ventaja acumulada y menos falta afinar.
// Finas arriba y anchas abajo: al arrancar el cuadro el dibujo solo se lleva
// de ventaja el borrado vertical (1400 us), y despues va ganando terreno. La
// segunda banda es la mas ajustada de todas, asi que se le dan 8 px de mas a
// costa de la ultima, que sobra con 6500 us de margen.
static const uint8_t BANDA_ALTO[RADAR_BANDAS] = { 24, 40, 32, 40, 56, 64, 64, 56, 56, 48 };

// El mapa de la vista de seguimiento se dibuja entero una sola vez y despues
// no se toca: son 30 ms de trabajo, imposible de repetir sesenta veces por
// segundo. Lo unico que se mueve es el avion, y para moverlo sin rehacer el
// mapa se guarda el pedazo de fondo que tapa y se repone antes de correrlo.
#define VIAJE_SPRITE_AN 150
#define VIAJE_SPRITE_AL 40
static uint8_t viaje_fondo[VIAJE_SPRITE_AN * VIAJE_SPRITE_AL];
static int viaje_fondo_x = -1, viaje_fondo_y = -1;
static int viaje_mapa_listo = 0;

void radar_viaje_rehacer(void) { viaje_mapa_listo = 0; viaje_fondo_x = -1; }

static void cuadro_viaje(void) {
    extern int avion_x, avion_y;

    if (!viaje_mapa_listo) {
        gfx_banda(0, VGA_ALTO - 1);          // de una sola pasada, no por bandas
        radar_pintar_viaje();
        viaje_mapa_listo = 1;
        viaje_fondo_x = -1;
        return;
    }

    // Reponer el fondo donde estaba el avion.
    if (viaje_fondo_x >= 0)
        gfx_reponer(viaje_fondo_x, viaje_fondo_y, VIAJE_SPRITE_AN, VIAJE_SPRITE_AL, viaje_fondo);

    // Volver a calcular donde va ahora, dibujarlo, y guardar lo que tapa.
    gfx_banda(0, VGA_ALTO - 1);
    int px = avion_x - VIAJE_SPRITE_AN / 4;
    int py = avion_y - VIAJE_SPRITE_AL / 2;
    viaje_fondo_x = px;
    viaje_fondo_y = py;
    gfx_guardar(px, py, VIAJE_SPRITE_AN, VIAJE_SPRITE_AL, viaje_fondo);
    radar_viaje_avion();
}

void radar_cuadro(void) {
    if (limpiar_todo) {
        // La limpieza se lleva un cuadro entero para ella sola: sumada al
        // dibujo se pasaba del tiempo que tarda el haz en bajar.
        gfx_banda(0, VGA_ALTO - 1);
        vga_limpiar(radar_tono(0));
        limpiar_todo = 0;
        return;
    }

    if (radar_vista == VISTA_SEGUIR || radar_vista == VISTA_SEGUIR_HIBRIDA) {
        cuadro_viaje();
        gfx_banda(0, VGA_ALTO - 1);
        return;
    }

    {   // Cuanto se comio el trabajo de antes de empezar a dibujar.
        int32_t usado = (int32_t)(time_us_32() - vga_us_vsync);
        if (usado > radar_us_antes) radar_us_antes = usado;
    }
    int y0 = 0;
    for (int b = 0; b < RADAR_BANDAS; b++) {
        int y1 = y0 + BANDA_ALTO[b] - 1;
        if (b == RADAR_BANDAS - 1 || y1 > VGA_ALTO - 1) y1 = VGA_ALTO - 1;
        gfx_banda(y0, y1);
        uint32_t tb = time_us_32();
        switch (radar_vista) {
            case VISTA_LOGOS:  break;   // se dibuja despues del bucle, ver abajo
            case VISTA_PARED:  break;   // se dibuja despues del bucle, ver abajo
            case VISTA_SEGUIR:
            case VISTA_SEGUIR_HIBRIDA: break;   // se dibuja aparte, ver abajo
            default:           radar_pintar(); break;
        }
        radar_us_banda[b] = time_us_32() - tb;
        if (y1 >= VGA_ALTO - 1) encabezado_sucio = 0;
        // Margen contra el haz: cuanto falta para que llegue al final de esta
        // banda, en el momento en que se termino de dibujar. Si da negativo,
        // el haz ya paso por ahi y esa franja sale rota.
        {
            int32_t haz = 1400 + (int32_t)((int64_t)(y1 + 1) * 15200 / VGA_ALTO);
            int32_t margen = haz - (int32_t)(time_us_32() - vga_us_vsync);
            if (margen < radar_margen_banda[b]) radar_margen_banda[b] = margen;
        }
        y0 = y1 + 1;
    }
    gfx_banda(0, VGA_ALTO - 1);

    // La tarjeta que toca se dibuja recien ahora, con el cuadro ya pintado y
    // el haz abajo del todo: la columna de tarjetas esta arriba, asi que el
    // haz ya paso por ahi y lo que se escriba se vera entero en el cuadro que
    // viene. Mientras estaba adentro del bucle de bandas costaba mas de lo
    // que el haz tardaba en llegar y rompia las bandas 1 a 7.
    // La pared va por el mismo camino: su tabla esta arriba y dibujarla
    // mientras el haz llega arriba lo hacia alcanzar al dibujo por 800 us.
    // Repartida en ocho pasadas, cada una cuesta poco y entra holgada aca.
    if (radar_vista == VISTA_PARED) pintar_pared();
    if (radar_vista == VISTA_LOGOS) { void logos_pantalla_pintar(void); logos_pantalla_pintar(); }

    if (radar_vista == VISTA_HIBRIDA && tarjetas_sucias) {
        const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
        const int ANS = AN * 54 / 100;
        radar_pintar_tarjetas(X + ANS + 8, Y + 32, AN - ANS - 16, AL - 36, 0);
    }

    if (tarjetas_sucias) {
        // Ocho pasadas y no cuatro: con cuatro, la pasada que cae en las
        // bandas de arriba costaba mas de lo que el haz tardaba en llegar.
        const int pasos = (radar_lista == LISTA_FIDS)
                        ? 8 : (radar_tarjetas < 1 ? 1 : radar_tarjetas);
        if (++tarjeta_en_curso >= pasos) { tarjetas_sucias = 0; tarjeta_en_curso = 0; }
    }
}
