// Trafico real, en el nucleo 1.
//
// Por que el nucleo 1: el video se arma con PIO y DMA, pero la interrupcion
// que arranca cada cuadro corre en el nucleo 0 con la prioridad mas alta que
// hay, y ahi mismo corre el dibujo. Cualquier cosa que se quede pensando un
// milisegundo de mas en ese nucleo se ve como una franja en la pantalla, y un
// handshake de TLS se queda pensando bastante mas que eso. Asi que la radio,
// lwIP y mbedTLS viven enteros en el nucleo 1, que no toca el framebuffer, y
// lo unico que cruza entre nucleos es un lote de aviones ya parseado.
//
// lwIP va en modo poll y no en el de fondo, justamente por lo mismo: en el de
// fondo el driver se cuelga de una interrupcion y se atiende cuando quiere.
// En poll se atiende cuando este nucleo lo pide, y no hay forma de que
// interrumpa al otro.
//
// El pedido es un GET a /api/pico, que devuelve texto: una linea por avion.
// No hay parser de JSON en la placa; ver sky-proxy/api/pico.js.
#include "sky.h"
#include "instalacion.h"
#include "portal.h"
#include "ap.h"
#include "config.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PROXY_HOST  "pico-vga-radar-sky.vercel.app"
#define PROXY_RUTA  "/api/pico"
#define PROXY_PUERTO 443

// Cada cuanto se pide un lote nuevo. Las fuentes de ADS-B actualizan cada
// pocos segundos y el proxy cachea doce, asi que bajar de quince no trae
// nada nuevo y solo castiga a las fuentes.
#define CADA_SEGUNDOS   20
#define REINTENTO_SEGUNDOS 8

// Cuantas veces se intenta entrar a la red guardada antes de rendirse y
// mostrar el QR. Cada intento son veinte segundos mas cuatro de espera: con
// cuatro, el equipo se pasa un minuto y medio probando antes de molestar al
// cliente. Un router que tarda en levantar despues de un corte de luz entra
// comodo en ese rato.
#define INTENTOS_ANTES_DEL_PORTAL 4

// La clave del wifi propio del equipo. Es la misma para todos y va impresa
// junto al QR: no protege nada, solo evita que el telefono se queje de estar
// entrando a una red abierta.
#define AP_CLAVE "radar1234"

// El cuerpo entero de la respuesta. Con 32 aviones son unos 2 kB; 6 kB deja
// margen de sobra y avisa por consola si alguna vez no alcanzara.
#define CUERPO_MAX  6144

typedef enum { P_LIBRE, P_RESOLVIENDO, P_CONECTANDO, P_LEYENDO, P_LISTO, P_FALLO } pedido_t;

// --- lo que cruza entre nucleos -------------------------------------------
// Un mutex y no una cola: el nucleo 0 lee esto una vez por cuadro y el 1
// escribe cada veinte segundos, asi que pelearse es rarisimo y cuando pasa
// son microsegundos.
static mutex_t candado;
static sky_avion_t lote[SKY_MAX];
static int         lote_n;
static bool        lote_nuevo;
static volatile sky_estado_t estado = SKY_APAGADO;
static volatile uint32_t     lote_ms;        // cuando llego, en ms de arranque
static volatile int          hora_min = -1;  // hora local, en minutos
static volatile uint32_t     hora_ms;        // cuando se leyo esa hora

// Que mirar. Lo escribe el nucleo 0 al cambiar de pantalla.
static volatile int32_t mirar_lat = -348220, mirar_lon = -585360;
static volatile int     mirar_km  = 220;

// --- estado del pedido, solo lo toca el nucleo 1 --------------------------
static struct altcp_pcb *pcb;
static ip_addr_t         proxy_ip;
static bool              proxy_ip_lista;
static volatile pedido_t pedido;
static uint32_t          pedido_ms;     // cuando arranco el que esta en curso
static char              cuerpo[CUERPO_MAX];
static int               cuerpo_n;
static bool              encabezado_pasado;

static void cerrar(void) {
    if (!pcb) return;
    altcp_arg(pcb, NULL);
    altcp_recv(pcb, NULL);
    altcp_err(pcb, NULL);
    if (altcp_close(pcb) != ERR_OK) altcp_abort(pcb);
    pcb = NULL;
}

// --- parseo ---
// El parser esta en sky_parse.c, que se compila igual fuera de la placa: asi
// se prueba contra una respuesta de verdad sin cargar el firmware.

// Convierte el cuerpo entero en el lote y lo publica. Devuelve cuantos
// aviones entraron.
static int digerir(void) {
    sky_avion_t nuevos[SKY_MAX];
    int hora = -1;
    const int n = sky_parse(cuerpo, nuevos, SKY_MAX, &hora);
    if (hora >= 0) {
        hora_min = hora;
        hora_ms  = to_ms_since_boot(get_absolute_time());
    }

    mutex_enter_blocking(&candado);
    memcpy(lote, nuevos, sizeof(sky_avion_t) * n);
    lote_n = n;
    lote_nuevo = true;
    mutex_exit(&candado);
    lote_ms = to_ms_since_boot(get_absolute_time());
    return n;
}

// --- callbacks de lwIP ----------------------------------------------------
static err_t al_recibir(void *arg, struct altcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    if (!p) {                      // el servidor cerro: lo que llego es todo
        cuerpo[cuerpo_n < CUERPO_MAX ? cuerpo_n : CUERPO_MAX - 1] = 0;
        pedido = cuerpo_n ? P_LISTO : P_FALLO;
        return ERR_OK;
    }
    if (err != ERR_OK) { pbuf_free(p); pedido = P_FALLO; return err; }

    for (struct pbuf *q = p; q; q = q->next) {
        const char *d = (const char *)q->payload;
        int largo = q->len;
        // La respuesta viene con encabezados HTTP adelante. No se parsean:
        // lo unico que interesa es donde termina, que es el renglon en
        // blanco. El estado ya se mira en la linea "#1" del cuerpo.
        if (!encabezado_pasado) {
            for (int i = 0; i + 3 < largo; i++) {
                if (d[i] == '\r' && d[i+1] == '\n' && d[i+2] == '\r' && d[i+3] == '\n') {
                    d += i + 4;
                    largo -= i + 4;
                    encabezado_pasado = true;
                    break;
                }
            }
            if (!encabezado_pasado) continue;
        }
        int cabe = CUERPO_MAX - 1 - cuerpo_n;
        if (largo > cabe) {
            largo = cabe;
            printf("sky: la respuesta no entra en %d bytes, se corta\n", CUERPO_MAX);
        }
        if (largo > 0) { memcpy(cuerpo + cuerpo_n, d, largo); cuerpo_n += largo; }
    }
    altcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void al_fallar(void *arg, err_t err) {
    (void)arg;
    printf("sky: se corto la conexion (%d)\n", err);
    pcb = NULL;                    // lwIP ya lo libero
    pedido = P_FALLO;
}

static err_t al_conectar(void *arg, struct altcp_pcb *tpcb, err_t err) {
    (void)arg;
    if (err != ERR_OK) { pedido = P_FALLO; return err; }

    char get[320];
    int n = snprintf(get, sizeof get,
        "GET %s?lat=%ld.%04ld&lon=%ld.%04ld&dist=%d&n=%d&tz=%d HTTP/1.1\r\n"
        "Host: " PROXY_HOST "\r\n"
        "User-Agent: pico-radar/1.0\r\n"
        "Connection: close\r\n\r\n",
        PROXY_RUTA,
        (long)(mirar_lat / 10000), (long)labs(mirar_lat % 10000),
        (long)(mirar_lon / 10000), (long)labs(mirar_lon % 10000),
        // El proxy pide millas nauticas; el firmware piensa en kilometros.
        (int)(mirar_km * 100 / 1852), SKY_MAX, INSTALACION_TZ_MINUTOS);

    if (altcp_write(tpcb, get, n, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        pedido = P_FALLO;
        return ERR_MEM;
    }
    altcp_output(tpcb);
    pedido = P_LEYENDO;
    return ERR_OK;
}

static void al_resolver(const char *nombre, const ip_addr_t *ip, void *arg) {
    (void)nombre; (void)arg;
    if (!ip) { printf("sky: no se pudo resolver " PROXY_HOST "\n"); pedido = P_FALLO; return; }
    proxy_ip = *ip;
    proxy_ip_lista = true;
    pedido = P_LIBRE;              // el ciclo lo vuelve a arrancar ya con IP
}

// --- el pedido ------------------------------------------------------------
static void arrancar_pedido(void) {
    pedido_ms = to_ms_since_boot(get_absolute_time());
    cuerpo_n = 0;
    encabezado_pasado = false;

    if (!proxy_ip_lista) {
        pedido = P_RESOLVIENDO;
        err_t e = dns_gethostbyname(PROXY_HOST, &proxy_ip, al_resolver, NULL);
        if (e == ERR_OK) { proxy_ip_lista = true; pedido = P_LIBRE; }
        else if (e != ERR_INPROGRESS) pedido = P_FALLO;
        return;
    }

    // Sin certificado de raiz: no se verifica contra quien se habla. Es una
    // decision tomada, no un olvido. La placa no tiene reloj con fecha (por
    // eso MBEDTLS_HAVE_TIME_DATE esta en 0), asi que no puede decir si un
    // certificado vencio, y guardar la cadena de raices se come flash y se
    // rompe sola cuando la CA rota. Lo que viaja es publico y de solo
    // lectura: posiciones de aviones que cualquiera baja de adsb.fi. El dia
    // que por aca pase algo del cliente (las credenciales del portal, por
    // ejemplo) esto tiene que cambiar.
    struct altcp_tls_config *tls = altcp_tls_create_config_client(NULL, 0);
    if (!tls) { pedido = P_FALLO; return; }
    pcb = altcp_tls_new(tls, IPADDR_TYPE_V4);
    if (!pcb) { altcp_tls_free_config(tls); pedido = P_FALLO; return; }

    // SNI: Vercel sirve muchos dominios en la misma IP y sin esto devuelve
    // el certificado equivocado.
    mbedtls_ssl_set_hostname(altcp_tls_context(pcb), PROXY_HOST);

    altcp_recv(pcb, al_recibir);
    altcp_err(pcb, al_fallar);
    pedido = P_CONECTANDO;
    if (altcp_connect(pcb, &proxy_ip, PROXY_PUERTO, al_conectar) != ERR_OK) {
        cerrar();
        pedido = P_FALLO;
    }
}

// --- el nucleo 1 ----------------------------------------------------------
// Tres momentos, y el equipo pasa de uno a otro solo:
//
//   CONECTAR  intenta entrar a la red guardada. Si no lo logra despues de
//             unos cuantos intentos, se rinde y se va a PORTAL.
//   NORMAL    ya esta adentro: pide lotes al proxy. El portal sigue
//             atendiendo, para que la web pueda mandarle la configuracion.
//   PORTAL    hace de router: levanta su propio wifi, reparte una IP,
//             contesta cualquier nombre y sirve el formulario de la red. El
//             nucleo 0 mientras tanto muestra el QR en el monitor.
typedef enum { M_CONECTAR, M_NORMAL, M_PORTAL } modo_t;

static volatile modo_t modo;
static volatile bool   portal_pedido;     // el gesto de los tres cortes
static char            ap_nombre[20];
static char            url_portal[32];

// El nombre del wifi propio lleva los ultimos cuatro del MAC, para que dos
// equipos en la misma casa no se pisen. Es el mismo criterio que ap_name() en
// pico/wifi.py.
static void armar_nombre_ap(void) {
    uint8_t mac[6] = {0};
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, mac);
    snprintf(ap_nombre, sizeof ap_nombre, "RADAR-%02X%02X", mac[4], mac[5]);
}

static void entrar_al_portal(void) {
    printf("sky: no hay red, se abre el portal\n");
    cyw43_arch_disable_sta_mode();
    armar_nombre_ap();
    cyw43_arch_enable_ap_mode(ap_nombre, AP_CLAVE, CYW43_AUTH_WPA2_AES_PSK);

    // La Pico es el router de su propia red.
    ip4_addr_t ip, mascara;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&mascara, 255, 255, 255, 0);
    netif_set_addr(netif_default, &ip, &mascara, &ip);

    snprintf(url_portal, sizeof url_portal, "http://192.168.4.1");
    ap_servicios_arrancar();
    portal_arrancar(true, "192.168.4.1");
    portal_barrer();               // para que la lista de redes este lista
    estado = SKY_PORTAL;
    modo = M_PORTAL;
}

static void nucleo1(void) {
    // Sin esto, cuando el nucleo 0 escribe en la flash (arranques.c, config.c)
    // este nucleo sigue ejecutando desde flash y se cuelga la placa.
    multicore_lockout_victim_init();

    if (cyw43_arch_init_with_country(INSTALACION_WIFI_PAIS)) {
        printf("sky: no arranco la radio\n");
        estado = SKY_SIN_RED;
        return;
    }

    uint32_t proximo = 0;
    int fallos = 0;

    // El gesto de los tres cortes, o directamente no tener red cargada,
    // mandan al portal sin siquiera intentar conectarse.
    if (portal_pedido || !config_hay_wifi()) {
        cyw43_arch_enable_sta_mode();      // hace falta para leer el MAC
        entrar_al_portal();
    } else {
        cyw43_arch_enable_sta_mode();
        netif_set_hostname(netif_default, "radar");
        modo = M_CONECTAR;
    }

    for (;;) {
        cyw43_arch_poll();

        if (modo == M_PORTAL) {
            portal_atender();
            const portal_pasa_t que = portal_paso();
            if (que == PORTAL_WIFI) {
                // La red quedo guardada en la flash. Reiniciar es la forma
                // mas limpia de pasar de hacer de router a ser un cliente
                // mas: no hay que desarmar nada a mano.
                printf("sky: red cargada, reiniciando\n");
                sleep_ms(1200);            // que el celular alcance a ver el aviso
                watchdog_reboot(0, 0, 50);
                for (;;) tight_loop_contents();
            }
            if (que == PORTAL_PANTALLAS) {
                portal_paso_limpiar();
                config_guardar();
                pantallas_rehacer_pedido = true;
            }
            sleep_ms(20);
            continue;
        }

        if (modo == M_CONECTAR) {
            estado = SKY_CONECTANDO;
            printf("sky: conectando a \"%s\"\n", config_ssid);
            const int r = cyw43_arch_wifi_connect_timeout_ms(
                config_ssid, config_pass, CYW43_AUTH_WPA2_AES_PSK, 20000);
            if (r) {
                fallos++;
                printf("sky: no se pudo conectar (%d), intento %d de %d\n",
                       r, fallos, INTENTOS_ANTES_DEL_PORTAL);
                estado = SKY_SIN_RED;
                if (fallos >= INTENTOS_ANTES_DEL_PORTAL) {
                    // Se rinde y le muestra el QR al cliente, que es lo unico
                    // que puede hacer algo al respecto: capaz cambio el
                    // router, o la clave.
                    entrar_al_portal();
                    continue;
                }
                sleep_ms(4000);
                continue;
            }
            fallos = 0;
            proxy_ip_lista = false;
            const char *ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
            snprintf(url_portal, sizeof url_portal, "http://%s", ip);
            printf("sky: en la red, IP %s\n", ip);
            // El portal sigue atendiendo con red: es por donde la web le
            // manda la configuracion al equipo.
            portal_arrancar(false, ip);
            modo = M_NORMAL;
            continue;
        }

        // --- modo normal ---
        if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
            printf("sky: se perdio la red\n");
            cerrar();
            pedido = P_LIBRE;
            estado = SKY_SIN_RED;
            modo = M_CONECTAR;
            continue;
        }

        if (portal_paso() == PORTAL_PANTALLAS) {
            portal_paso_limpiar();
            config_guardar();
            pantallas_rehacer_pedido = true;
        }

        const uint32_t ahora = to_ms_since_boot(get_absolute_time());

        switch (pedido) {
        case P_LIBRE:
            if (ahora >= proximo) { estado = SKY_PIDIENDO; arrancar_pedido(); }
            break;
        case P_LISTO: {
            cerrar();
            const int n = digerir();
            printf("sky: %d aviones\n", n);
            estado = n ? SKY_ANDANDO : SKY_PROXY_CAIDO;
            pedido = P_LIBRE;
            proximo = ahora + CADA_SEGUNDOS * 1000;
            break;
        }
        case P_FALLO:
            cerrar();
            estado = SKY_PROXY_CAIDO;
            pedido = P_LIBRE;
            proximo = ahora + REINTENTO_SEGUNDOS * 1000;
            break;
        default:
            // Conectando, resolviendo o leyendo: si tarda demasiado, se corta
            // y se reintenta. Se mide desde que arranco ESTE pedido: si no,
            // el primero de todos nace con el reloj ya corrido.
            if (ahora - pedido_ms > 25000) {
                printf("sky: el pedido tardo demasiado\n");
                cerrar();
                pedido = P_FALLO;
            }
            break;
        }
        sleep_ms(5);
    }
}

// --- lo que ve el nucleo 0 ------------------------------------------------
// Lo pone el nucleo 1 cuando el cliente guarda pantallas nuevas; lo mira y lo
// baja el nucleo 0, que es el unico que puede tocar el dibujo.
volatile bool pantallas_rehacer_pedido;

void sky_init(bool pedir_portal) {
    mutex_init(&candado);
    portal_pedido = pedir_portal;
    // La radio se prende siempre: aunque no haya ninguna red cargada, hace
    // falta para levantar el portal y que el cliente pueda cargar una.
    multicore_launch_core1(nucleo1);
}

bool sky_en_portal(void) { return modo == M_PORTAL; }
const char *sky_ap_nombre(void) { return ap_nombre; }
const char *sky_ap_clave(void) { return AP_CLAVE; }
const char *sky_portal_url(void) { return url_portal; }

void sky_mirar(int32_t lat, int32_t lon, int radio_km) {
    mirar_lat = lat;
    mirar_lon = lon;
    mirar_km  = radio_km;
}

int sky_tomar(sky_avion_t *destino, int tope) {
    if (!lote_nuevo) return -1;
    mutex_enter_blocking(&candado);
    int n = lote_n < tope ? lote_n : tope;
    memcpy(destino, lote, sizeof(sky_avion_t) * n);
    lote_nuevo = false;
    mutex_exit(&candado);
    return n;
}

sky_estado_t sky_estado(void) { return estado; }

const char *sky_estado_texto(void) {
    switch (estado) {
    case SKY_APAGADO:     return "SIN WIFI - TRAFICO DE PRUEBA";
    case SKY_CONECTANDO:  return "CONECTANDO AL WIFI";
    case SKY_SIN_RED:     return "SIN WIFI";
    case SKY_PIDIENDO:    return "BUSCANDO VUELOS";
    case SKY_ANDANDO:     return "EN VIVO";
    case SKY_PROXY_CAIDO: return "SIN DATOS";
    case SKY_PORTAL:      return "MODO CONFIGURACION";
    }
    return "";
}

int sky_hora_local(void) {
    if (hora_min < 0) return -1;
    uint32_t corridos = (to_ms_since_boot(get_absolute_time()) - hora_ms) / 60000u;
    return (int)((hora_min + corridos) % 1440u);
}

uint32_t sky_segundos_desde_el_ultimo(void) {
    if (!lote_ms) return 0xFFFFFFFFu;
    return (to_ms_since_boot(get_absolute_time()) - lote_ms) / 1000u;
}
