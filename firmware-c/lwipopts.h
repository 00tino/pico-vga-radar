// Configuracion de lwIP para el radar.
//
// El equipo hace una sola cosa en la red: un GET por HTTPS cada tantos
// segundos al proxy. No recibe conexiones, no manda nada mas. Asi que todo
// esto esta recortado al minimo, porque cada buffer que lwIP reserva sale de
// los 150 kB que quedan libres despues del framebuffer.
//
// Sin sistema operativo: lwIP corre en el nucleo 1, en modo poll, y es ese
// nucleo el unico que lo toca. Ver wifi.c.
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
// El grueso de la memoria de lwIP. Con una sola conexion TCP y la ventana de
// abajo, 8 kB alcanzan y sobran.
#define MEM_SIZE                    8000

#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN_SEM_PER_THREAD 0

#define LWIP_IPV4                   1
#define LWIP_IPV6                   0

#define LWIP_TCP                    1
#define TCP_TTL                     255
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE          1

#define LWIP_UDP                    1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define DNS_MAX_SERVERS             2

#define LWIP_CHKSUM_ALGORITHM       3
#define CHECKSUM_GEN_IP             1
#define CHECKSUM_GEN_UDP            1
#define CHECKSUM_GEN_TCP            1
#define CHECKSUM_CHECK_IP           1
#define CHECKSUM_CHECK_UDP          1
#define CHECKSUM_CHECK_TCP          1

#define LWIP_STATS                  0
#define LWIP_STATS_DISPLAY          0

// TLS. La capa altcp es la que deja enchufar mbedTLS abajo del TCP de
// siempre, asi que el codigo de sky.c no sabe si va cifrado o no.
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1

// Los mensajes de lwIP no van a ningun lado: la consola USB es para lo que
// imprime el firmware, y si se llena no se lee nada.
#define LWIP_DEBUG                  0

#endif
