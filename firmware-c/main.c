// El equipo arranca directo en el radar: la pantalla de primitivas ya no sale
// al encender, porque no es algo que el cliente tenga que ver. Sigue estando
// como herramienta: se pide con 'd' por la consola.
//
// Patron que demuestra las primitivas de dibujo: texto, lineas, circulos,
// rectangulos y logos. Usa el tema crt_amber de la web (#e8b86d sobre #0a0805).
//
// Todo se dibuja relativo al area util, nunca a 0,0: asi se ve igual en un
// monitor que recorta y en uno que no.
#include "vga.h"
#include "gfx.h"
#include "area.h"
#include "logos.h"
#include "radar.h"
#include "trig.h"
#include "instalacion.h"
#include "monitor.h"
#include "arranques.h"
#include "sky.h"
#include "config.h"
#include "qr.h"
#include "vivo.h"
#include "pantallas.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>

// Mide cuanto tiempo pasa alto cada pin: con esto se vio en su momento que
// los cables estaban en pines muertos. Sirve para saber si la señal sigue
// saliendo bien mientras el radar redibuja.
void demo_init(void);
void demo_aeropuerto(const char *iata);
void demo_avanzar(void);
void volcado_fb(void);

static uint8_t fondo, ambar, tenue, apagado, blanco;

static void dibujar_patron(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    vga_limpiar(fondo);
    gfx_rect(X, Y, AN, AL, apagado);          // borde del area util

    // --- TEXTO: tres escalas y la fuente completa ---
    gfx_texto_centrado(X + AN / 2, Y + 6, "PICO VGA RADAR", ambar, 2);
    gfx_texto_centrado(X + AN / 2, Y + 36,
                       "primitivas de dibujo en C  -  640x480  -  256 colores", tenue, 1);
    gfx_hlinea(X + 16, Y + 54, AN - 32, apagado);
    gfx_texto(X + 16, Y + 62, "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", ambar, 1);
    gfx_texto(X + 16, Y + 78, "0123456789  !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", tenue, 1);

    // Tres paneles repartidos a lo ancho del area.
    const int py = Y + 96, pal = AL - 96 - 74;
    const int pan = (AN - 32 - 16) / 3;
    const int p0 = X + 16, p1 = p0 + pan + 8, p2 = p1 + pan + 8;

    // --- LINEAS: abanico de 24 rayos, mide los ocho octantes de Bresenham ---
    gfx_rect(p0, py, pan, pal, apagado);
    gfx_texto(p0 + 8, py + 4, "LINEAS", tenue, 1);
    const int lx = p0 + pan / 2, ly = py + pal / 2;
    const int lr = (pan < pal ? pan : pal) / 2 - 20;
    static const int sen90[7] = {0, 66, 128, 181, 222, 247, 256};
    for (int i = 0; i < 24; i++) {
        int q = i % 6, cuad = i / 6;
        int s = sen90[q], co = sen90[6 - q];
        int dx, dy;
        switch (cuad) {
            case 0: dx =  co; dy = -s;  break;
            case 1: dx =  s;  dy =  co; break;
            case 2: dx = -co; dy =  s;  break;
            default:dx = -s;  dy = -co; break;
        }
        gfx_linea(lx, ly, lx + dx * lr / 256, ly + dy * lr / 256,
                  (i % 6 == 0) ? ambar : tenue);
    }
    gfx_texto(p0 + 8, py + pal - 18, "24 rayos, 8 octantes", apagado, 1);

    // --- CIRCULOS: anillos de alcance y blips, como el radar ---
    gfx_rect(p1, py, pan, pal, apagado);
    gfx_texto(p1 + 8, py + 4, "CIRCULOS", tenue, 1);
    const int cx = p1 + pan / 2, cy = py + pal / 2;
    const int cr = (pan < pal ? pan : pal) / 2 - 20;
    for (int i = 4; i >= 1; i--)
        gfx_circulo(cx, cy, cr * i / 4, i == 4 ? ambar : tenue);
    gfx_linea(cx - cr, cy, cx + cr, cy, apagado);
    gfx_linea(cx, cy - cr, cx, cy + cr, apagado);
    gfx_circulo_lleno(cx, cy, 4, ambar);
    gfx_circulo_lleno(cx + cr * 5 / 8, cy - cr / 2, 5, blanco);
    gfx_circulo_lleno(cx - cr * 3 / 4, cy + cr / 3, 5, blanco);
    gfx_texto(p1 + 8, py + pal - 18, "contorno y relleno", apagado, 1);

    // --- RECTANGULOS: tarjeta de vuelo con logo real, como la de la web ---
    gfx_rect(p2, py, pan, pal, apagado);
    gfx_texto(p2 + 8, py + 4, "RECTANGULOS", tenue, 1);
    const int tx = p2 + 8, ty = py + 24, tan = pan - 16;
    gfx_rect(tx, ty, tan, 86, tenue);
    const uint8_t *logo = logo_buscar("AR");
    if (logo) gfx_blit(tx + 8, ty + 6, LOGO_LADO, LOGO_LADO, logo);
    gfx_texto(tx + 8 + LOGO_LADO + 8, ty + 10, "AR1301", ambar, 1);
    gfx_texto(tx + 8 + LOGO_LADO + 8, ty + 26, "EZE > MAD", tenue, 1);
    gfx_rect(tx + 8, ty + 52, tan - 16, 8, apagado);
    gfx_rect_lleno(tx + 9, ty + 53, (tan - 18) * 62 / 100, 6, ambar);
    gfx_texto(tx + 8, ty + 66, "FL350  842 km/h", tenue, 1);
    for (int i = 0; i < 6; i++)
        gfx_rect(tx + i * 6, ty + 104 + i * 6, tan - i * 12,
                 pal - 128 - i * 12, (i & 1) ? tenue : apagado);
    gfx_texto(p2 + 8, py + pal - 18, "contorno y relleno", apagado, 1);

    // --- LOGOS: reales, sacados de la flash ---
    const int ly2 = py + pal + 6;
    gfx_texto(X + 16, ly2 + 10, "LOGOS", tenue, 1);
    static const char *muestra[] = {"AR", "LA", "AA", "IB", "JL", "NH",
                                    "AF", "BA", "UA", "DL", "EK", "QF"};
    const int lx0 = X + 66, paso = (AN - 82) / 12;
    for (int i = 0; i < 12; i++) {
        const uint8_t *l = logo_buscar(muestra[i]);
        if (l) gfx_blit(lx0 + i * paso, ly2, LOGO_LADO, LOGO_LADO, l);
    }

    // --- RAMPA: el degradado con dither, el que se ve parejo ---
    const int ry = ly2 + LOGO_LADO + 6;
    gfx_texto(X + 16, ry + 2, "RAMPA", tenue, 1);
    const int ran = AN - 82 - 70;
    for (int i = 0; i < ran; i++) {
        int n = 255 * i / (ran - 1);
        gfx_rect_dither(lx0 + i, ry, 1, 14,
                        0xe8 * n / 255, 0xb8 * n / 255, 0x6d * n / 255);
    }
    gfx_texto(X + AN - 16 - gfx_ancho_texto("824 logos", 1), ry + 2, "824 logos", apagado, 1);
}

// Lo que se ve mientras el equipo esta esperando que lo configuren: el QR
// grande en el medio, y abajo el nombre del wifi propio y la clave, por si el
// telefono no lee el codigo.
//
// El QR se dibuja con modulos bien gordos a proposito. La camara de un
// telefono lo lee de lejos y a traves de un monitor que puede estar sucio o
// mal enfocado, asi que conviene que sobre tamano: seis pixeles por modulo
// dan un codigo de 222 px de lado, que en 640x480 entra comodo y deja lugar
// abajo para el nombre de la red y la clave.
static void pantalla_configuracion(void) {
    const int X = area.x, Y = area.y, AN = area.an, AL = area.al;
    const uint8_t fondo = vga_rgb(0x0a, 0x08, 0x05);
    const uint8_t ambar = vga_rgb(0xe8, 0xb8, 0x6d);
    const uint8_t tenue = vga_rgb(0x7a, 0x60, 0x38);
    const uint8_t blanco = vga_color(7, 7, 3);

    vga_limpiar(fondo);
    gfx_rect(X, Y, AN, AL, tenue);
    gfx_texto_centrado(X + AN / 2, Y + 26, "CONFIGURAR EL RADAR", ambar, 2);

    uint8_t m[QR_LADO][QR_LADO];
    const char *url = sky_portal_url();
    if (url[0] && qr_armar(url, m)) {
        // Sobre blanco y con margen alrededor: un QR sin borde claro no lo
        // lee ninguna camara.
        const int lado = 6;
        const int borde = 4 * lado;
        const int caja = QR_LADO * lado + 2 * borde;
        const int qx = X + (AN - caja) / 2, qy = Y + 58;
        gfx_rect_lleno(qx, qy, caja, caja, blanco);
        for (int j = 0; j < QR_LADO; j++)
            for (int i = 0; i < QR_LADO; i++)
                if (m[j][i])
                    gfx_rect_lleno(qx + borde + i * lado, qy + borde + j * lado,
                                   lado, lado, fondo);

        const int abajo = qy + caja + 22;
        char linea[64];
        if (sky_en_portal()) {
            gfx_texto_centrado(X + AN / 2, abajo,
                "1. Entra con el celular a la red:", tenue, 1);
            snprintf(linea, sizeof linea, "%s", sky_ap_nombre());
            gfx_texto_centrado(X + AN / 2, abajo + 20, linea, ambar, 2);
            snprintf(linea, sizeof linea, "clave: %s", sky_ap_clave());
            gfx_texto_centrado(X + AN / 2, abajo + 48, linea, tenue, 1);
            gfx_texto_centrado(X + AN / 2, abajo + 70,
                "2. Apunta la camara al codigo de arriba.", tenue, 1);
        } else {
            gfx_texto_centrado(X + AN / 2, abajo,
                "Apunta la camara al codigo para configurar el radar.", tenue, 1);
            snprintf(linea, sizeof linea, "o entra a %s", url);
            gfx_texto_centrado(X + AN / 2, abajo + 22, linea, tenue, 1);
        }
    } else {
        gfx_texto_centrado(X + AN / 2, Y + AL / 2, "PREPARANDO EL CODIGO...", tenue, 1);
    }

    gfx_texto_centrado(X + AN / 2, Y + AL - 26,
        "Para volver al radar, cortar la corriente una vez", tenue, 1);
}

// Las pantallas del cliente. Hasta que este el portal se arman aca; despues
// van a llegar de la web y a guardarse en la flash.
//
// Esta es la que describio Valentino: los vuelos de Ezeiza y un vuelo que
// sigue, cinco segundos cada uno. Con cuatro tarjetas por vez y veinte vuelos
// en la lista, la primera vuelta muestra del 1 al 4, la siguiente del 5 al 8,
// y asi hasta completarlos: el carrusel no se reinicia al volver.
static void pantallas_de_ejemplo(void) {
    pantallas_n = 0;
    pantalla_t *p;

    p = &pantallas[pantallas_n++];
    snprintf(p->nombre, sizeof p->nombre, "vuelos de Ezeiza");
    p->vista = VISTA_HIBRIDA;  p->lista = LISTA_TARJETAS;
    snprintf(p->apt, sizeof p->apt, "EZE");
    p->radio_km = 220;  p->tarjetas = 4;
    snprintf(p->tema, sizeof p->tema, "crt_amber");
    p->seguir[0] = 0;   p->segundos = PANTALLA_SEGUNDOS_MIN;

    p = &pantallas[pantallas_n++];
    snprintf(p->nombre, sizeof p->nombre, "siguiendo AA954");
    p->vista = VISTA_SEGUIR_HIBRIDA;  p->lista = LISTA_TARJETAS;
    snprintf(p->apt, sizeof p->apt, "EZE");
    p->radio_km = 220;  p->tarjetas = 1;
    snprintf(p->tema, sizeof p->tema, "crt_amber");
    snprintf(p->seguir, sizeof p->seguir, "AA954");
    p->segundos = PANTALLA_SEGUNDOS_MIN;

    // Que la lista pase de pagina justo cuando la pantalla se va: asi cada
    // visita muestra el grupo siguiente y no repite el mismo.
    radar_rotacion_s = PANTALLA_SEGUNDOS_MIN;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2500);                       // margen para que el Mac tome el puerto
    printf("arrancando firmware en C\n");
    // Antes del video: si hubo un borrado de flash, aca no molesta a nadie.
    const bool pedir_config = arranques_contar();

    // El reloj primero de todo. Tiene que quedar en su valor final antes de
    // que arranque la radio, porque el bus del chip de WiFi se temporiza a
    // partir de este reloj y cambiarselo despues lo desacomoda.
    vga_reloj();

    // Lo guardado se lee ANTES de largar el otro nucleo, y el orden importa:
    // lo primero que hace ese nucleo es preguntar si hay una red cargada. Si
    // todavia no se leyo la flash, no la ve, y el equipo se va al portal a
    // pedir una red que en realidad ya tenia.
    bool hay_pantallas = config_leer();

    // El gesto de los tres cortes borra lo guardado: la red y las pantallas.
    // Es la unica forma que tiene el cliente de empezar de cero si cambio de
    // router o le regalo el equipo a otro.
    if (pedir_config) {
        printf("tres cortes seguidos: se olvida la red y las pantallas\n");
        config_borrar();
        hay_pantallas = false;
    }

    // Y esto antes tambien: cuando el otro nucleo escriba la flash (al
    // guardar la red o las pantallas) va a tener que congelar a este, y un
    // nucleo no se puede congelar si antes no dijo que se deja. El caso al
    // reves ya estaba contemplado del otro lado, en sky.c.
    multicore_lockout_victim_init();

    // Si el cliente hizo el gesto de los tres cortes, va derecho al portal.
    sky_init(pedir_config);

    // Y se le da tiempo a la radio ANTES de encender el video.
    //
    // Se midio en la placa: con el video apagado la radio entra a la red en
    // diez segundos; con el video andando no entra nunca, y el driver larga
    // "do_ioctl: timeout". El dibujo le come el bus al chip de WiFi justo
    // mientras esta negociando, que es cuando menos aguanta esperar.
    //
    // El costo es que el monitor arranca en negro unos segundos. Pasa una vez
    // por encendido y es preferible a un equipo que no se conecta nunca.
    {
        const uint32_t hasta = to_ms_since_boot(get_absolute_time()) + 40000;
        while (sky_intentando_conectar() &&
               to_ms_since_boot(get_absolute_time()) < hasta) {
            sleep_ms(50);
        }
        printf("la radio ya se acomodo: se enciende el video\n");
    }

    vga_init();
    printf("video inicializado: %dx%d, 256 colores\n", VGA_ANCHO, VGA_ALTO);

    fondo   = vga_rgb(0x0a, 0x08, 0x05);
    ambar   = vga_rgb(0xe8, 0xb8, 0x6d);
    tenue   = vga_rgb(0x7a, 0x60, 0x38);
    apagado = vga_rgb(0x3d, 0x30, 0x1c);
    blanco  = vga_color(7, 7, 3);

    // Los margenes del monitor de este equipo salen de instalacion.h, que es
    // el unico lugar donde se toca para armar uno con otra pantalla.
    printf("monitor segun instalacion.h: %s\n", INSTALACION_MONITOR);
    monitor_init();
    monitor_informe();
    area_set(INSTALACION_MARGEN_ARRIBA, INSTALACION_MARGEN_ABAJO,
             INSTALACION_MARGEN_IZQUIERDA, INSTALACION_MARGEN_DERECHA);
    printf("area util: %dx%d en %d,%d\n", area.an, area.al, area.x, area.y);

    radar_init();
    demo_init();
    { void demo_casa(void); demo_casa(); }
    printf("radar andando\n");

    // Modo normal: rota entre las pantallas del cliente. El recorrido de las
    // 18 escenas de prueba sigue disponible con la tecla 'e'.
    // Manda lo que haya guardado el cliente; si no guardo nada todavia, las
    // de fabrica.
    if (!hay_pantallas) {
        printf("no hay pantallas guardadas: se usan las de fabrica\n");
        pantallas_de_ejemplo();
    }
    pantallas_init();

    uint32_t cuadros = 0, us_total = 0, us_peor = 0;
    const uint32_t encendido = time_us_32();
    bool marcado_largo = false;
    uint32_t desde_informe = time_us_32();
    char portal_puesto[32] = "";
    // Mientras el cliente no haya elegido sus pantallas, el equipo muestra el
    // QR en vez del radar: es el segundo paso del armado, despues de cargar
    // la red. Con pantallas guardadas esto no aparece nunca y el equipo
    // arranca derecho en lo que el cliente dejo configurado.
    bool esperando_config = !hay_pantallas;
    if (esperando_config) printf("sin pantallas del cliente: se muestra el QR para configurar\n");
    for (;;) {
        // Mientras el equipo espera que lo configuren no hay radar que
        // dibujar: esta la pantalla del QR y nada mas. Se redibuja solo
        // cuando cambia la direccion, que es una vez, al terminar de
        // levantar el wifi propio.
        // La web mando pantallas nuevas: el nucleo 1 ya las guardo, aca solo
        // hay que empezar a mostrarlas. Y si el equipo estaba esperando que
        // lo configuraran, ese era el paso que faltaba.
        if (pantallas_rehacer_pedido) {
            pantallas_rehacer_pedido = false;
            printf("pantallas nuevas desde la web: %d\n", pantallas_n);
            pantallas_init();
            esperando_config = false;
            portal_puesto[0] = 0;
            radar_marcar_sucio();
        }

        // Las dos situaciones en las que se ve el QR y no el radar: cuando el
        // equipo esta pidiendo una red, y cuando ya la tiene pero todavia no
        // le dijeron que mostrar.
        if (sky_en_portal() || esperando_config) {
            if (strcmp(portal_puesto, sky_portal_url())) {
                snprintf(portal_puesto, sizeof portal_puesto, "%s", sky_portal_url());
                pantalla_configuracion();
            }
            vga_esperar_cuadro();
            continue;
        }
        if (portal_puesto[0]) {
            portal_puesto[0] = 0;
            radar_marcar_sucio();
        }

        int c = getchar_timeout_us(0);
        if (c == 'v') volcado_fb();
        else if (c == 'm') monitor_informe();
        else if (c == 'M') monitor_diagnostico();
        else if (c == 'W') monitor_vigilar(180);
        else if (c == 'n') pantallas_forzar_siguiente();
        // Sin portal todavia, estas dos son la unica forma de probar que las
        // pantallas sobreviven al apagado.
        else if (c == 'g') config_guardar();
        else if (c == 'G') config_borrar();
        else if (c == 'd') {
            dibujar_patron();
            while (getchar_timeout_us(0) != 'd') tight_loop_contents();
            radar_marcar_sucio();
        }
        else if (c == 'c') {
            area_calibrar(vga_rgb(0x0a, 0x08, 0x05), vga_rgb(0xe8, 0xb8, 0x6d),
                          vga_color(7, 7, 3));
            printf("calibracion en pantalla: leer desde que numero se ve cada regla\n");
            while (getchar_timeout_us(0) != 'c') tight_loop_contents();
            radar_marcar_sucio();
        }

        if (!marcado_largo &&
            (time_us_32() - encendido) / 1000000u >= ARRANQUES_SEGUNDOS) {
            marcado_largo = true;
            arranques_fue_largo();
        }

        vga_esperar_cuadro();
        uint32_t t0 = time_us_32();
        // Si el nucleo 1 dejo un lote nuevo, entra aca. Mientras no haya
        // llegado ninguno, los que se mueven son los inventados.
        vivo_avanzar();
        if (!vivo_hay_datos()) demo_avanzar();
        radar_avanzar();
        pantallas_avanzar();
        radar_cuadro();
        uint32_t d = time_us_32() - t0;
        us_total += d;
        if (d > us_peor) us_peor = d;
        cuadros++;

        // Un informe cada quince segundos, para no llenar la consola.
        if (time_us_32() - desde_informe >= 15000000u) {
            desde_informe = time_us_32();
            printf("  %lu cuadros, %lu por segundo | dibujo %lu us promedio, %lu us el peor%s\n",
                   (unsigned long)cuadros, (unsigned long)(cuadros / 15),
                   (unsigned long)(cuadros ? us_total / cuadros : 0), (unsigned long)us_peor,
                   us_peor > 15200 ? "  <-- SE PASA" : "");
            {
                extern int32_t radar_margen_banda[];
                printf("    margen contra el haz (us):");
                int roto = 0;
                for (int b = 0; b < 10; b++) {
                    printf(" %ld", (long)radar_margen_banda[b]);
                    if (radar_margen_banda[b] < 0) roto = 1;
                    radar_margen_banda[b] = 0x7fffffff;
                }
                printf("%s\n", roto ? "   <-- EL HAZ ALCANZA AL DIBUJO" : "");
            }
            cuadros = us_total = us_peor = 0;
        }
    }
}

