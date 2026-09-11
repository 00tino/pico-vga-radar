// El servidor del portal de configuracion.
//
// Es el mismo reparto que ya tenia pico/portal.py y por la misma razon: lo
// pesado (elegir aeropuerto sobre un mapa, ver los logos, la vista previa)
// vive en la web de GitHub Pages, que corre en el celular y tiene toda la
// memoria del mundo. El equipo solo recibe el resultado, que son unos cientos
// de bytes. Meter esa pagina adentro de la Pico seria pelear por cada kilobyte
// contra el framebuffer, y para nada.
//
// Entonces hay dos momentos:
//
//   1. El equipo no tiene red todavia. Levanta su propio wifi, muestra el QR
//      en el monitor, y sirve el UNICO formulario que vive en la placa: el de
//      elegir la red de casa. Como ademas contesta cualquier nombre (ver
//      ap.c), el telefono abre esa pagina solo, sin que nadie escriba nada.
//
//   2. El equipo ya esta en la red de casa. El QR lleva a la web completa,
//      con la IP del equipo colgada de la direccion, y esa web le manda la
//      configuracion a /save.
//
// La lista de redes sale de un barrido de la radio: el cliente elige la suya
// de una lista en vez de escribirla. No se puede hacer que el celular le
// "pase" su wifi al equipo como cuando compartis una red entre dos telefonos;
// eso necesita que el que se conecta lea un QR con la camara, y la Pico no
// tiene camara.
#include "portal.h"
#include "config.h"
#include "pantallas.h"
#include "radar.h"
#include "portal_parse.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PUERTO 80
#define PEDIDO_MAX 2048
#define REDES_MAX 16

// La web completa, la misma que apunta pico/portal.py.
#define WEB "https://00tino.github.io/pico-vga-radar/setup.html"

static struct tcp_pcb *escucha;
static bool  modo_ap;
static char  mi_ip[16];
static volatile portal_pasa_t paso;

// --- barrido de redes -----------------------------------------------------
static char     redes[REDES_MAX][33];
static int16_t  redes_rssi[REDES_MAX];
static int      redes_n;
static bool     barriendo;

static int barrido_resultado(void *arg, const cyw43_ev_scan_result_t *r) {
    (void)arg;
    if (!r || !r->ssid_len) return 0;
    char ssid[33];
    const int largo = r->ssid_len < 32 ? r->ssid_len : 32;
    memcpy(ssid, r->ssid, largo);
    ssid[largo] = 0;

    // La misma red aparece muchas veces (una por canal y por repetidor): se
    // guarda una sola vez y con la senal mas fuerte que se haya visto.
    for (int i = 0; i < redes_n; i++) {
        if (!strcmp(redes[i], ssid)) {
            if (r->rssi > redes_rssi[i]) redes_rssi[i] = r->rssi;
            return 0;
        }
    }
    if (redes_n >= REDES_MAX) return 0;
    snprintf(redes[redes_n], sizeof redes[0], "%s", ssid);
    redes_rssi[redes_n] = r->rssi;
    redes_n++;
    return 0;
}

void portal_barrer(void) {
    if (barriendo) return;
    redes_n = 0;
    cyw43_wifi_scan_options_t opciones = {0};
    if (cyw43_wifi_scan(&cyw43_state, &opciones, NULL, barrido_resultado) == 0)
        barriendo = true;
}

void portal_atender(void) {
    if (barriendo && !cyw43_wifi_scan_active(&cyw43_state)) {
        barriendo = false;
        printf("portal: %d redes a la vista\n", redes_n);
        // Con el nombre y la senal de cada una: cuando el equipo dice que no
        // encuentra una red, esto es lo unico que despeja si el problema es
        // que no la ve o que no logra entrar.
        for (int i = 0; i < redes_n; i++)
            printf("   \"%s\"  %d dBm\n", redes[i], redes_rssi[i]);
    }
}

bool portal_barriendo(void) { return barriendo; }

bool portal_vio(const char *ssid) {
    for (int i = 0; i < redes_n; i++)
        if (!strcmp(redes[i], ssid)) return true;
    return false;
}

// --- lo que se contesta ---------------------------------------------------
static void mandar(struct tcp_pcb *pcb, const char *tipo, const char *cuerpo) {
    char cabecera[160];
    const int largo = (int)strlen(cuerpo);
    const int n = snprintf(cabecera, sizeof cabecera,
        "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\nConnection: close\r\n\r\n", tipo, largo);
    tcp_write(pcb, cabecera, n, TCP_WRITE_FLAG_COPY);
    // De a pedazos: el buffer de salida de lwIP no se traga la pagina entera.
    int puesto = 0;
    while (puesto < largo) {
        int cabe = tcp_sndbuf(pcb);
        if (cabe <= 0) break;
        int trozo = largo - puesto;
        if (trozo > cabe) trozo = cabe;
        if (tcp_write(pcb, cuerpo + puesto, trozo, TCP_WRITE_FLAG_COPY) != ERR_OK) break;
        puesto += trozo;
    }
    tcp_output(pcb);
}

// El unico formulario que vive en la placa. Sin imagenes, sin fuentes, sin
// nada que haya que bajar: tiene que verse con el telefono desconectado de
// internet, que es justo la situacion en la que se usa.
static const char FORMULARIO[] =
"<!doctype html><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Radar</title><style>"
"body{background:#0a0a0b;color:#eceae4;font:16px system-ui;margin:0;padding:28px}"
"h1{font-size:20px;margin:0 0 4px}p{color:#9a958b;margin:0 0 22px;font-size:14px}"
"label{display:block;font-size:12px;text-transform:uppercase;color:#6f6a62;margin:16px 0 6px}"
"input,select{width:100%;box-sizing:border-box;height:46px;border-radius:10px;"
"border:1px solid #2a2926;background:#000;color:#fff;padding:0 12px;font-size:16px}"
"button{width:100%;height:46px;margin-top:24px;border:0;border-radius:10px;"
"background:#f2542d;color:#fff;font-size:16px}"
"a{color:#6f6a62;font-size:13px;display:block;margin-top:18px;text-align:center}"
"</style>"
"<h1>Conectar el radar</h1>"
"<p>Elegi tu red de casa. El equipo se reinicia y arranca solo.</p>"
"<form action='/wifi'>"
"<label>Red WiFi (2.4 GHz)</label>"
"<select name=ssid id=lista></select>"
"<label>Contrase&ntilde;a</label><input name=pass type=password>"
"<button>Guardar y reiniciar</button></form>"
"<a href='/'>Volver a buscar redes</a>"
"<script>fetch('/redes').then(r=>r.json()).then(l=>{"
"var s=document.getElementById('lista');"
"if(!l.length){s.outerHTML=\"<input name=ssid autocapitalize=off required>\";return;}"
"l.forEach(function(r){var o=document.createElement('option');"
"o.textContent=r.n+' ('+r.s+')';o.value=r.n;s.appendChild(o);});});</script>";

static void aviso(struct tcp_pcb *pcb, const char *titulo, const char *detalle) {
    char pagina[512];
    snprintf(pagina, sizeof pagina,
        "<!doctype html><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<body style='background:#0a0a0b;color:#eceae4;font:17px system-ui;"
        "padding:40px;text-align:center'>"
        "<h1 style='color:#f2542d'>%s</h1><p>%s</p>", titulo, detalle);
    mandar(pcb, "text/html", pagina);
}

// --- lectura del pedido ---
// Las funciones que leen parametros y pantallas estan en portal_parse.c, que
// se compila igual fuera de la placa: asi el contrato con docs/setup.html se
// prueba sin cargar el firmware.

static void guardar_pantallas(struct tcp_pcb *pcb, const char *consulta) {
    pantalla_t nuevas[PANTALLAS_MAX];
    const int puestas = portal_leer_pantallas(consulta, nuevas, PANTALLAS_MAX);
    if (!puestas) {
        aviso(pcb, "No se pudo", "Ninguna pantalla vino completa.");
        return;
    }

    // Cuantos puntos van en el circulo. Es uno solo para todo el equipo, no
    // por pantalla, asi que viaja suelto y no adentro de cada una.
    config_solo_aerolineas = portal_numero(consulta, "com", config_solo_aerolineas ? 1 : 0) != 0;

    const int tope = portal_numero(consulta, "max", config_max_puntos);
    if (tope >= 1 && tope <= RADAR_MAX_AVIONES) {
        config_max_puntos = tope;      // para que se guarde en la flash
        radar_max_puntos  = tope;      // y para que se vea ya mismo
    }

    memcpy(pantallas, nuevas, sizeof(pantalla_t) * puestas);
    pantallas_n = puestas;
    // Se contesta ANTES de escribir la flash: el borrado frena todo unas
    // decenas de milisegundos y el celular no tiene por que esperar eso.
    aviso(pcb, "Listo", "El monitor ya se esta actualizando.");
    paso = PORTAL_PANTALLAS;
}

// --- el pedido ------------------------------------------------------------
static err_t al_llegar(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    (void)arg;
    if (!p) { tcp_close(pcb); return ERR_OK; }
    if (err != ERR_OK) { pbuf_free(p); tcp_close(pcb); return err; }

    char pedido[PEDIDO_MAX];
    const int largo = p->tot_len < PEDIDO_MAX - 1 ? p->tot_len : PEDIDO_MAX - 1;
    pbuf_copy_partial(p, pedido, largo, 0);
    pedido[largo] = 0;
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    // Solo interesa la primera linea: "GET /lo/que/sea HTTP/1.1".
    char *fin_linea = strstr(pedido, "\r\n");
    if (fin_linea) *fin_linea = 0;
    char *ruta = strchr(pedido, ' ');
    if (!ruta) { tcp_close(pcb); return ERR_OK; }
    ruta++;
    char *fin_ruta = strchr(ruta, ' ');
    if (fin_ruta) *fin_ruta = 0;

    // Se imprime solo hasta el "?" a proposito: en /wifi lo que viene despues
    // es la red y su contrasena, y una contrasena no se escribe en ningun
    // registro, ni siquiera en la consola de depuracion.
    {
        const char *interrogante = strchr(ruta, '?');
        const int hasta = interrogante ? (int)(interrogante - ruta) : (int)strlen(ruta);
        printf("portal: piden %.*s%s\n", hasta, ruta, interrogante ? "?..." : "");
    }

    if (!strncmp(ruta, "/wifi?", 6)) {
        char ssid[33], pass[64];
        portal_parametro(ruta, "ssid", ssid, sizeof ssid);
        portal_parametro(ruta, "pass", pass, sizeof pass);
        if (ssid[0]) {
            char limpio[200], detalle[256];
            portal_escapar(ssid, limpio, sizeof limpio);
            // Se guarda ANTES de contestar: si la flash falla, el cliente
            // tiene que enterarse en vez de ver un "guardado" mentiroso y
            // quedarse esperando un equipo que nunca se conecto.
            if (config_guardar_wifi(ssid, pass)) {
                snprintf(detalle, sizeof detalle, "Reiniciando y conectando a %s...", limpio);
                aviso(pcb, "Guardado", detalle);
                paso = PORTAL_WIFI;
            } else {
                aviso(pcb, "No se pudo guardar",
                      "El equipo no pudo escribir la red en su memoria. Proba de nuevo.");
            }
        } else {
            aviso(pcb, "Falta la red", "Elegi una red de la lista.");
        }
    } else if (!strncmp(ruta, "/save?", 6)) {
        guardar_pantallas(pcb, ruta);
    } else if (!strcmp(ruta, "/redes")) {
        // Un SSID de 32 caracteres, todos escapados, ocupa seis veces mas:
        // el lugar se calcula para el peor caso y no para el habitual.
        char json[REDES_MAX * (33 * 6 + 24) + 8];
        int n = snprintf(json, sizeof json, "[");
        for (int i = 0; i < redes_n; i++) {
            // snprintf devuelve lo que HABRIA escrito, no lo que escribio: si
            // se suma sin mirar, n se pasa del buffer y el tamano que queda,
            // que no tiene signo, da vuelta y se vuelve enorme.
            if (n >= (int)sizeof json - 2) break;
            char limpio[33 * 6 + 1];
            portal_escapar_json(redes[i], limpio, sizeof limpio);
            const int puesto = snprintf(json + n, sizeof json - n,
                                        "%s{\"n\":\"%s\",\"s\":%d}",
                                        i ? "," : "", limpio, redes_rssi[i]);
            if (puesto < 0 || puesto >= (int)sizeof json - n) break;
            n += puesto;
        }
        snprintf(json + n, sizeof json - n, "]");
        mandar(pcb, "application/json", json);
    } else if (modo_ap) {
        mandar(pcb, "text/html", FORMULARIO);
    } else {
        // Ya hay internet: la configuracion de verdad la hace la web, que
        // necesita saber a que equipo mandarle lo que el cliente elija.
        char pagina[384];
        snprintf(pagina, sizeof pagina,
            "<!doctype html><meta charset=utf-8>"
            "<meta http-equiv=refresh content='0;url=" WEB "?pico=%s'>"
            "<a href='" WEB "?pico=%s'>Abrir la configuracion</a>", mi_ip, mi_ip);
        mandar(pcb, "text/html", pagina);
    }

    tcp_close(pcb);
    return ERR_OK;
}

static err_t al_conectarse(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)arg;
    if (err != ERR_OK || !pcb) return ERR_VAL;

    // Cada conexion aceptada ocupa un lugar de la cola de espera, y el lugar
    // NO se devuelve solo: hay que avisar que ya se atendio. Sin esto el
    // servidor atiende tantas conexiones como lugares tenga la cola y despues
    // deja de aceptar para siempre.
    //
    // Es facil no darse cuenta probando de a un pedido por vez, y con un
    // telefono no pasa nunca: apenas se conecta a una red, iOS y Android
    // abren varias conexiones sueltas para ver si hay internet o si hay un
    // portal. Esas se comen la cola entera antes de que el cliente llegue a
    // abrir la pagina.
    tcp_accepted(escucha);

    tcp_recv(pcb, al_llegar);
    // Que no se quede una conexion colgada ocupando memoria si el telefono
    // se va sin cerrar.
    tcp_poll(pcb, NULL, 8);
    return ERR_OK;
}

bool portal_arrancar(bool es_ap, const char *ip) {
    modo_ap = es_ap;
    snprintf(mi_ip, sizeof mi_ip, "%s", ip ? ip : "192.168.4.1");
    paso = PORTAL_NADA;
    if (escucha) return true;

    // Todo lo que se le pide a lwIP desde el bucle va entre begin y end: lwIP
    // corre adentro de una interrupcion. Ver sky.c.
    cyw43_arch_lwip_begin();
    struct tcp_pcb *p = tcp_new();
    if (p && tcp_bind(p, IP_ANY_TYPE, PUERTO) != ERR_OK) { tcp_close(p); p = NULL; }
    cyw43_arch_lwip_end();
    if (!p) return false;
    // Lugares en la cola de espera. Un telefono abre varias conexiones a la
    // vez apenas entra a la red, asi que dos quedan cortos aunque la cola se
    // libere bien.
    cyw43_arch_lwip_begin();
    escucha = tcp_listen_with_backlog(p, 8);
    if (escucha) tcp_accept(escucha, al_conectarse);
    else tcp_close(p);
    cyw43_arch_lwip_end();
    if (!escucha) return false;
    printf("portal: atendiendo en http://%s\n", mi_ip);
    return true;
}

void portal_parar(void) {
    if (!escucha) return;
    tcp_close(escucha);
    escucha = 0;
}

portal_pasa_t portal_paso(void) { return paso; }
void portal_paso_limpiar(void) { paso = PORTAL_NADA; }
