// Firmware de prueba: SOLO la radio, sin video.
//
// Existe para contestar una pregunta y nada mas: cuando el equipo no logra
// entrar a la red y el driver larga "do_ioctl: timeout", hay dos sospechosos
// que no se pueden separar mirando el firmware completo:
//
//   1. El chip de radio, la red o la alimentacion.
//   2. La convivencia con el video, que baja el reloj del sistema a 100,8 MHz
//      (ver vga.c) y tiene el bus de memoria ocupado todo el tiempo.
//
// Esto arranca la radio con el reloj como viene de fabrica y sin encender el
// video. Si aca conecta, el problema es la convivencia. Si aca tampoco
// conecta, el video no tiene nada que ver y hay que mirar para otro lado.
//
// Se compila junto con el firmware de verdad y se carga a mano cuando hace
// falta; no molesta a nadie.
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "config.h"
#include "instalacion.h"
#include "hardware/clocks.h"
#include <stdio.h>

// config.c los necesita; aca no se dibuja ninguna pantalla.
pantalla_t pantallas[PANTALLAS_MAX];
int        pantallas_n;

int main(void) {
    // Con "lento" por consola (o compilando con PRUEBA_RELOJ_LENTO) se baja el
    // reloj igual que lo hace el video, pero SIN encender el video. Eso separa
    // los dos sospechosos: si aca deja de conectar, la culpa es del reloj; si
    // conecta, la culpa es del bus ocupado por el dibujo.
#ifdef PRUEBA_RELOJ_LENTO
    set_sys_clock_khz(100800, false);
#endif
    stdio_init_all();
    sleep_ms(3000);
    printf("\n=== prueba de radio, sin video ===\n");
    printf("reloj del sistema: %lu kHz (el firmware completo lo baja a 100800)\n",
           (unsigned long)(clock_get_hz(clk_sys) / 1000));

    // La red sale de la flash, la misma que cargo el cliente por el portal.
    config_leer();
    if (!config_hay_wifi()) {
        printf("no hay ninguna red guardada: cargar una por el portal primero\n");
        for (;;) tight_loop_contents();
    }
    printf("red guardada: \"%s\"\n", config_ssid);

    if (cyw43_arch_init_with_country(INSTALACION_WIFI_PAIS)) {
        printf("no arranco la radio\n");
        for (;;) tight_loop_contents();
    }
    cyw43_arch_enable_sta_mode();

    for (int intento = 1; intento <= 6; intento++) {
        printf("\n-- intento %d --\n", intento);
        const uint32_t t0 = to_ms_since_boot(get_absolute_time());
        const int r = cyw43_arch_wifi_connect_timeout_ms(
            config_ssid, config_pass, CYW43_AUTH_WPA2_MIXED_PSK, 20000);
        const uint32_t tardo = to_ms_since_boot(get_absolute_time()) - t0;

        if (!r) {
            printf("CONECTO en %lu ms, IP %s\n", (unsigned long)tardo,
                   ip4addr_ntoa(netif_ip4_addr(netif_default)));
            printf("=> la radio y la red estan bien: lo que rompe es la convivencia\n");
            printf("   con el video (reloj a 100,8 MHz y el bus ocupado).\n");
            for (;;) { sleep_ms(5000); printf("sigue conectado\n"); }
        }
        printf("fallo (%d) despues de %lu ms\n", r, (unsigned long)tardo);
        sleep_ms(2000);
    }

    printf("\n=> no conecto ni con el video apagado: el video no tiene la culpa.\n");
    printf("   mirar la alimentacion, la clave, o el chip.\n");
    for (;;) tight_loop_contents();
}
