// Contador de arranques cortos seguidos. Ver arranques.c.
#ifndef ARRANQUES_H
#define ARRANQUES_H
#include <stdbool.h>

// Cuantos cortes de corriente seguidos piden la pantalla de configuracion, y
// cuantos segundos tiene que andar el equipo para que la cuenta se limpie.
//
// Los tres minutos son la ventana del gesto: el cliente corta, prende, corta,
// prende, corta, y mientras cada tramo dure menos de eso, los tres cuentan
// como seguidos. Si en alguno se queda mirando el radar mas de tres minutos,
// es que no estaba haciendo el gesto y la cuenta vuelve a cero.
//
// El otro extremo es el arranque en si: entre que se le da corriente y que la
// placa alcanza a anotar el arranque pasan unos tres segundos (main.c espera
// a que la Mac tome el puerto USB). Cortar antes de eso no cuenta.
#define ARRANQUES_ACTIVO      1
#define ARRANQUES_PARA_CONFIG 3
#define ARRANQUES_SEGUNDOS    180

bool arranques_contar(void);      // al arrancar, antes de encender el video
void arranques_fue_largo(void);   // pasados ARRANQUES_SEGUNDOS andando
int  arranques_cuenta(void);

#endif
