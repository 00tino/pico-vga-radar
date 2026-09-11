// Pasa el trafico real que trae el nucleo 1 a los aviones del radar.
// Ver vivo.c.
#ifndef VIVO_H
#define VIVO_H
#include <stdbool.h>

void vivo_avanzar(void);     // una vez por cuadro
bool vivo_hay_datos(void);   // false mientras el trafico sea el de demo.c
void vivo_aeropuerto(const char *iata);

#endif
