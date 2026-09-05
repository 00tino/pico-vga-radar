// Quien es el monitor y si esta prendido, leido por el canal DDC del VGA.
//
// Los agujeros 12 y 15 del VGA son un bus I2C con una memoria adentro del
// monitor (direccion 0x50) que guarda el EDID: marca, modelo y resoluciones.
// Es lo unico que viaja desde el monitor hacia la placa; los botones del menu
// no salen de ahi, asi que no hay forma de recibirlos.
//
// El agujero 9 (+5V) queda SIN CONECTAR a proposito. Con el conectado, la
// memoria queda alimentada por la placa y contesta aunque el monitor este
// apagado. Sin el, se alimenta del monitor: contesta prendido y se calla
// apagado. Eso convierte el boton de encendido del monitor en algo que la
// placa puede ver, que es lo mas parecido a "apretar un boton del monitor".
//
// Las lineas van con 1,95k en serie: el bus del monitor trabaja a 5 V y los
// pines de la placa son de 3,3 V y no toleran 5.
#include "monitor.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define DDC_I2C   i2c1
#define DDC_SDA   10
#define DDC_SCL   11
#define DDC_ADDR  0x50

static int ddc_hz = 50 * 1000;

void monitor_init(void) {
    // Bien lento: con casi 2k en serie la linea sube despacio, y el estandar
    // de DDC ya es lento de por si.
    i2c_init(DDC_I2C, ddc_hz);
    gpio_set_function(DDC_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DDC_SCL, GPIO_FUNC_I2C);
    // Por si el monitor no trae las suyas: sin ninguna, el bus queda al aire.
    gpio_pull_up(DDC_SDA);
    gpio_pull_up(DDC_SCL);
}

// Lee los 128 bytes del EDID. Devuelve 0 si el monitor no contesta.
static int leer_edid(uint8_t *dst) {
    uint8_t desde = 0;
    if (i2c_write_timeout_us(DDC_I2C, DDC_ADDR, &desde, 1, true, 20000) != 1)
        return 0;
    if (i2c_read_timeout_us(DDC_I2C, DDC_ADDR, dst, 128, false, 60000) != 128)
        return 0;
    // La cabecera del EDID es fija: sirve para saber que lo que llego es un
    // EDID y no ruido del bus.
    static const uint8_t CAB[8] = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0 };
    if (memcmp(dst, CAB, 8)) return 0;
    int suma = 0;
    for (int i = 0; i < 128; i++) suma += dst[i];
    return (suma & 0xFF) == 0;
}

bool monitor_prendido(void) {
    uint8_t buf[128];
    return leer_edid(buf) != 0;
}

// Marca y modelo, para reconocer si el equipo cambio de pantalla.
bool monitor_leer(monitor_t *m) {
    uint8_t e[128];
    memset(m, 0, sizeof *m);
    if (!leer_edid(e)) return false;

    // La marca son tres letras metidas en dos bytes, de a cinco bits.
    const uint16_t f = (uint16_t)((e[8] << 8) | e[9]);
    m->marca[0] = (char)('A' - 1 + ((f >> 10) & 31));
    m->marca[1] = (char)('A' - 1 + ((f >> 5) & 31));
    m->marca[2] = (char)('A' - 1 + (f & 31));
    m->marca[3] = 0;
    m->producto = (uint16_t)(e[10] | (e[11] << 8));

    // El nombre esta en uno de los cuatro descriptores del final, el que
    // arranca con el tipo 0xFC. No todos los monitores lo traen.
    for (int d = 0; d < 4; d++) {
        const uint8_t *p = &e[54 + d * 18];
        if (p[0] || p[1] || p[3] != 0xFC) continue;
        int n = 0;
        for (int i = 5; i < 18 && n < (int)sizeof m->nombre - 1; i++) {
            if (p[i] == 0x0A) break;
            m->nombre[n++] = (char)p[i];
        }
        while (n > 0 && m->nombre[n - 1] == ' ') n--;
        m->nombre[n] = 0;
        break;
    }
    if (!m->nombre[0]) snprintf(m->nombre, sizeof m->nombre, "(sin nombre)");

    // Resolucion preferida, del primer descriptor de tiempos.
    m->ancho = (uint16_t)(e[56] | ((e[58] & 0xF0) << 4));
    m->alto  = (uint16_t)(e[59] | ((e[61] & 0xF0) << 4));
    return true;
}

// Diagnostico del bus: sirve para separar "el monitor no contesta" de "el
// cable esta mal". Si las lineas no estan en alto con las resistencias de
// arriba en juego, no hay bus y no tiene sentido mirar el EDID.
void monitor_diagnostico(void) {
    // Sueltas y sin pull-up interno: si algo las sostiene en alto, es el
    // monitor. Si quedan en bajo, no hay quien las levante.
    gpio_set_function(DDC_SDA, GPIO_FUNC_SIO);
    gpio_set_function(DDC_SCL, GPIO_FUNC_SIO);
    gpio_set_dir(DDC_SDA, GPIO_IN);
    gpio_set_dir(DDC_SCL, GPIO_IN);
    gpio_disable_pulls(DDC_SDA);
    gpio_disable_pulls(DDC_SCL);
    sleep_ms(5);
    const int sda_libre = gpio_get(DDC_SDA), scl_libre = gpio_get(DDC_SCL);

    gpio_pull_up(DDC_SDA);
    gpio_pull_up(DDC_SCL);
    sleep_ms(5);
    const int sda_pu = gpio_get(DDC_SDA), scl_pu = gpio_get(DDC_SCL);

    printf("bus DDC: sueltas SDA=%d SCL=%d | con pull-up interno SDA=%d SCL=%d\n",
           sda_libre, scl_libre, sda_pu, scl_pu);

    // La prueba que separa un bus de verdad de un cable al aire: se baja la
    // linea, se suelta y se mide cuanto tarda en volver a subir. Con una
    // resistencia del otro lado sube en microsegundos; al aire se queda en
    // cero un buen rato, porque lo unico que la levanta es la fuga.
    for (int k = 0; k < 2; k++) {
        const int pin = k ? DDC_SCL : DDC_SDA;
        const char *nom = k ? "SCL" : "SDA";
        gpio_disable_pulls(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
        sleep_ms(2);
        const int bajo_ok = (gpio_get(pin) == 0);
        gpio_set_dir(pin, GPIO_IN);
        uint32_t t = time_us_32(), subio = 0;
        while (time_us_32() - t < 50000) {
            if (gpio_get(pin)) { subio = time_us_32() - t; break; }
        }
        if (!bajo_ok)     printf("  %s: no baja ni forzandola, hay algo raro\n", nom);
        else if (!subio)  printf("  %s: NO sube sola (mas de 50 ms) -> la linea esta al aire\n", nom);
        else              printf("  %s: sube sola en %lu us -> hay resistencia del otro lado\n",
                                 nom, (unsigned long)subio);
    }
    if (!sda_libre && !scl_libre)
        printf("  las dos sueltas en bajo: el monitor no las levanta, se alimentan del agujero 9\n");
    else if (sda_libre && scl_libre)
        printf("  las dos en alto sin ayuda: el monitor tiene sus resistencias, el bus esta vivo\n");

    // Barrer todas las direcciones a varias velocidades: con las resistencias
    // en serie la linea sube despacio y a 50 kHz puede no llegar a tiempo.
    static const int HZ[] = { 100000, 50000, 20000, 10000, 5000 };
    for (int v = 0; v < 5; v++) {
        ddc_hz = HZ[v];
        monitor_init();
        sleep_ms(5);
        printf("  a %d Hz contestan:", ddc_hz);
        int hay = 0;
        for (int a = 0x08; a < 0x78; a++) {
            uint8_t x;
            if (i2c_read_timeout_us(DDC_I2C, (uint8_t)a, &x, 1, false, 8000) >= 0) {
                printf(" 0x%02X", a); hay = 1;
            }
        }
        printf("%s\n", hay ? "" : " ninguna");
    }
    ddc_hz = 50000;
    monitor_init();
}

// Si el monitor esta prendido, sin usar el EDID.
//
// Las resistencias que levantan las dos lineas del DDC estan adentro del
// monitor y se alimentan de el. Con el monitor prendido, una linea que se
// suelta vuelve a alto en microsegundos; con el monitor apagado no la levanta
// nadie y se queda en cero. Eso alcanza para saber si esta prendido, y es lo
// que convierte el boton de encendido del monitor en algo que la placa ve.
//
// No hace falta el EDID, que en este monitor no contesta porque su memoria se
// alimenta del agujero 9 y ese quedo sin conectar a proposito.
bool monitor_hay_senal(void) {
    const int pin = DDC_SCL;
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_disable_pulls(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
    sleep_us(200);
    gpio_set_dir(pin, GPIO_IN);
    uint32_t t = time_us_32();
    int subio = 0;
    while (time_us_32() - t < 2000)
        if (gpio_get(pin)) { subio = 1; break; }
    return subio != 0;
}

// Sigue el estado un rato e imprime cada vez que cambia. Sirve para probar el
// gesto de apagar y prender el monitor con su propio boton.
void monitor_vigilar(int segundos) {
    printf("mirando el monitor %d s: apagalo y prendelo con su boton\n", segundos);
    int antes = -1, cambios = 0;
    uint32_t t0 = time_us_32();
    while ((time_us_32() - t0) / 1000000u < (uint32_t)segundos) {
        const int ahora = monitor_hay_senal() ? 1 : 0;
        if (ahora != antes) {
            if (antes >= 0) cambios++;
            printf("  %2lu s: monitor %s\n",
                   (unsigned long)((time_us_32() - t0) / 1000000u),
                   ahora ? "PRENDIDO" : "APAGADO");
            antes = ahora;
        }
        sleep_ms(100);
    }
    printf("listo: %d cambios de estado\n", cambios);
    monitor_init();
}

void monitor_informe(void) {
    monitor_t m;
    if (!monitor_leer(&m)) {
        printf("monitor: no contesta por DDC (apagado, sin cable, o no lo soporta)\n");
        return;
    }
    printf("monitor: %s %s (codigo %04X), preferida %dx%d\n",
           m.marca, m.nombre, m.producto, m.ancho, m.alto);
}
