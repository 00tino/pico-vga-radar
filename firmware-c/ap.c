// Los dos servicios minimos que necesita la Pico cuando hace de router: uno
// que le de una IP al celular y otro que le conteste cualquier nombre.
//
// No vienen con el SDK (estan en pico-examples, que aca no esta), y son
// cortos, asi que se escriben. Los dos son lo mas chico que funciona y nada
// mas: hablan con un telefono por vez durante un minuto, no son servidores de
// verdad.
//
//   DHCP: el celular pregunta "que IP me toca" y se le contesta siempre lo
//   mismo, 192.168.4.16, con la Pico como router y como DNS. Una sola
//   direccion alcanza: al portal entra una persona.
//
//   DNS: se le contesta 192.168.4.1 a CUALQUIER nombre que pregunte. Eso es
//   lo que hace que el telefono, al ver que la red no lleva a ningun lado,
//   abra la pagina de configuracion solo, sin que nadie tipee una direccion.
//   Es el mismo truco que el wifi de un hotel.
#include "ap.h"
#include "lwip/udp.h"
#include "pico/cyw43_arch.h"
#include <string.h>
#include <stdio.h>

#define PUERTO_DHCP_SERVIDOR 67
#define PUERTO_DHCP_CLIENTE  68
#define PUERTO_DNS           53

// La Pico es la .1 y al telefono le toca la .16.
#define IP_PICO     PP_HTONL(LWIP_MAKEU32(192, 168, 4, 1))
#define IP_CLIENTE  PP_HTONL(LWIP_MAKEU32(192, 168, 4, 16))
#define MASCARA     PP_HTONL(LWIP_MAKEU32(255, 255, 255, 0))

static struct udp_pcb *pcb_dhcp, *pcb_dns;

// --- DHCP -----------------------------------------------------------------
// El mensaje de DHCP tiene una cabecera fija y despues una lista de opciones.
// Solo interesan dos cosas: que tipo de mensaje es y a quien contestarle.
typedef struct {
    uint8_t  op, htype, hlen, hops;
    uint32_t xid;
    uint16_t secs, flags;
    uint32_t ciaddr, yiaddr, siaddr, giaddr;
    uint8_t  chaddr[16];
    uint8_t  relleno[192];      // sname y file, que no usamos
    uint32_t galleta;           // el numero magico que marca que hay opciones
    uint8_t  opciones[312];
} __attribute__((packed)) dhcp_t;

#define GALLETA_DHCP    PP_HTONL(0x63825363)
#define DHCP_DESCUBRIR  1
#define DHCP_OFRECER    2
#define DHCP_PEDIR      3
#define DHCP_ACEPTAR    5

// Busca una opcion por su numero. Devuelve NULL si no esta.
static const uint8_t *opcion(const dhcp_t *m, int largo, uint8_t cual) {
    const uint8_t *p = m->opciones;
    const uint8_t *fin = (const uint8_t *)m + largo;
    while (p + 1 < fin && *p != 255) {
        if (*p == 0) { p++; continue; }       // relleno
        if (*p == cual) return p;
        p += 2 + p[1];
    }
    return 0;
}

static uint8_t *poner(uint8_t *p, uint8_t cual, const void *datos, uint8_t largo) {
    *p++ = cual;
    *p++ = largo;
    memcpy(p, datos, largo);
    return p + largo;
}

static void dhcp_llego(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *desde, u16_t puerto) {
    (void)arg; (void)desde; (void)puerto;
    if (!p) return;
    if (p->tot_len < 240 || p->tot_len > sizeof(dhcp_t)) { pbuf_free(p); return; }

    dhcp_t m;
    memset(&m, 0, sizeof m);
    pbuf_copy_partial(p, &m, p->tot_len < sizeof m ? p->tot_len : sizeof m, 0);
    const int largo = p->tot_len;
    pbuf_free(p);

    if (m.galleta != GALLETA_DHCP) return;
    const uint8_t *tipo = opcion(&m, largo, 53);
    if (!tipo) return;

    uint8_t respuesta;
    if (tipo[2] == DHCP_DESCUBRIR)   respuesta = DHCP_OFRECER;
    else if (tipo[2] == DHCP_PEDIR)  respuesta = DHCP_ACEPTAR;
    else return;                      // los demas mensajes no nos importan

    // Se contesta el mismo paquete con los campos cambiados: asi no hay que
    // armar uno de cero ni acordarse del xid.
    m.op = 2;                         // respuesta
    m.yiaddr = IP_CLIENTE;
    m.siaddr = IP_PICO;
    memset(m.opciones, 0, sizeof m.opciones);

    const uint32_t ip_pico = IP_PICO, mascara = MASCARA;
    const uint32_t arriendo = PP_HTONL(24 * 60 * 60);   // un dia
    uint8_t *o = m.opciones;
    o = poner(o, 53, &respuesta, 1);              // que tipo de respuesta es
    o = poner(o, 54, &ip_pico, 4);                // quien es el servidor
    o = poner(o, 51, &arriendo, 4);               // cuanto dura
    o = poner(o, 1,  &mascara, 4);                // mascara de red
    o = poner(o, 3,  &ip_pico, 4);                // router
    o = poner(o, 6,  &ip_pico, 4);                // DNS: la Pico misma
    *o++ = 255;                                   // fin de las opciones

    const int salida = (int)(o - (uint8_t *)&m);
    struct pbuf *r = pbuf_alloc(PBUF_TRANSPORT, salida, PBUF_RAM);
    if (!r) return;
    memcpy(r->payload, &m, salida);
    // A la de broadcast, porque el telefono todavia no tiene IP puesta.
    udp_sendto(pcb, r, IP_ADDR_BROADCAST, PUERTO_DHCP_CLIENTE);
    pbuf_free(r);
}

// --- DNS ------------------------------------------------------------------
static void dns_llego(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                      const ip_addr_t *desde, u16_t puerto) {
    (void)arg;
    if (!p) return;
    // Cabecera de 12 bytes y por lo menos un nombre con su tipo y clase.
    if (p->tot_len < 12 + 5 || p->tot_len > 512) { pbuf_free(p); return; }

    uint8_t pregunta[512];
    const int largo = p->tot_len;
    pbuf_copy_partial(p, pregunta, largo, 0);
    const ip_addr_t quien = *desde;
    pbuf_free(p);

    if (pregunta[2] & 0x80) return;                 // ya era una respuesta
    if ((pregunta[4] << 8 | pregunta[5]) != 1) return;   // una sola consulta

    // Donde termina el nombre: una secuencia de trozos que cierra con un cero.
    int fin = 12;
    while (fin < largo && pregunta[fin]) {
        if (pregunta[fin] & 0xC0) return;           // nombre comprimido: no
        fin += pregunta[fin] + 1;
    }
    fin += 1 + 4;                                   // el cero, el tipo y la clase
    if (fin > largo) return;

    uint8_t r[sizeof pregunta + 16];
    memcpy(r, pregunta, fin);
    r[2] = 0x84;                  // es respuesta, y la damos nosotros
    r[3] = 0x00;
    r[6] = 0; r[7] = 1;           // una respuesta
    r[8] = 0; r[9] = 0;           // sin autoridad
    r[10] = 0; r[11] = 0;         // sin extras

    uint8_t *a = r + fin;
    *a++ = 0xC0; *a++ = 0x0C;     // el nombre, apuntando al de la pregunta
    *a++ = 0; *a++ = 1;           // tipo A
    *a++ = 0; *a++ = 1;           // clase IN
    *a++ = 0; *a++ = 0; *a++ = 0; *a++ = 60;   // que no lo guarde mucho
    *a++ = 0; *a++ = 4;           // cuatro bytes de direccion
    const uint32_t ip = IP_PICO;
    memcpy(a, &ip, 4);
    a += 4;

    const int salida = (int)(a - r);
    struct pbuf *resp = pbuf_alloc(PBUF_TRANSPORT, salida, PBUF_RAM);
    if (!resp) return;
    memcpy(resp->payload, r, salida);
    udp_sendto(pcb, resp, &quien, puerto);
    pbuf_free(resp);
}

// --- prender y apagar -----------------------------------------------------
bool ap_servicios_arrancar(void) {
    // Entre begin y end, como todo lo que se le pide a lwIP desde el bucle.
    cyw43_arch_lwip_begin();
    pcb_dhcp = udp_new();
    pcb_dns  = udp_new();
    bool bien = pcb_dhcp && pcb_dns &&
                udp_bind(pcb_dhcp, IP_ANY_TYPE, PUERTO_DHCP_SERVIDOR) == ERR_OK &&
                udp_bind(pcb_dns,  IP_ANY_TYPE, PUERTO_DNS) == ERR_OK;
    if (bien) {
        udp_recv(pcb_dhcp, dhcp_llego, NULL);
        udp_recv(pcb_dns,  dns_llego,  NULL);
    }
    cyw43_arch_lwip_end();
    if (!bien) { ap_servicios_parar(); return false; }
    printf("ap: dando direcciones y contestando nombres\n");
    return true;
}

void ap_servicios_parar(void) {
    if (pcb_dhcp) { udp_remove(pcb_dhcp); pcb_dhcp = 0; }
    if (pcb_dns)  { udp_remove(pcb_dns);  pcb_dns  = 0; }
}
