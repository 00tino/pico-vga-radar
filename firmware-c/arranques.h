// Contador de arranques cortos seguidos. Ver arranques.c.
#ifndef ARRANQUES_H
#define ARRANQUES_H
#include <stdbool.h>

// Cuantos cortes de corriente seguidos piden la pantalla de configuracion, y
// cuantos segundos tiene que andar el equipo para que la cuenta se limpie.
// Apagado: el QR va a salir cuando la Pico no encuentre su wifi, no por un
// gesto. Queda escrito y probado por si alguna vez se quiere un reset de
// emergencia; con poner 1 aca alcanza.
#define ARRANQUES_ACTIVO      1
#define ARRANQUES_PARA_CONFIG 3
#define ARRANQUES_SEGUNDOS    12

bool arranques_contar(void);      // al arrancar, antes de encender el video
void arranques_fue_largo(void);   // pasados ARRANQUES_SEGUNDOS andando
int  arranques_cuenta(void);

#endif
