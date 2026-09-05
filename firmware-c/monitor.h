// Lectura del monitor por el canal DDC del VGA. Ver monitor.c.
#ifndef MONITOR_H
#define MONITOR_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char     marca[4];      // las tres letras del fabricante, por ejemplo VSC
    uint16_t producto;      // codigo de modelo
    char     nombre[16];    // nombre legible, si el monitor lo trae
    uint16_t ancho, alto;   // resolucion preferida
} monitor_t;

void monitor_init(void);
bool monitor_leer(monitor_t *m);
bool monitor_prendido(void);
void monitor_informe(void);
void monitor_diagnostico(void);
bool monitor_hay_senal(void);
void monitor_vigilar(int segundos);

#endif
