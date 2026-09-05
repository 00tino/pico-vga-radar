// Seno y coseno en punto fijo. El angulo va en 1/1024 de vuelta y el
// resultado en 1/1024, o sea que sen(256) == 1024.
//
// La tabla se llena una sola vez al arrancar con la libreria de matematica.
// De ahi en mas no se usan flotantes: el dibujo corre 60 veces por segundo y
// la Pico no tiene coprocesador.
#ifndef TRIG_H
#define TRIG_H
#include <stdint.h>

#define TRIG_VUELTA 1024
#define TRIG_UNO    1024

extern int16_t trig_sen_tab[TRIG_VUELTA];

void trig_init(void);

// Angulo del vector (x,y) en unidades de vuelta, con el mismo convenio que el
// canvas de la web: x a la derecha, y hacia abajo, 0 apuntando a la derecha.
int trig_atan2(int y, int x);

static inline int trig_sen(int a) { return trig_sen_tab[((a % TRIG_VUELTA) + TRIG_VUELTA) % TRIG_VUELTA]; }
static inline int trig_cos(int a) { return trig_sen(a + TRIG_VUELTA / 4); }

// Grados de rumbo (0 = norte, crece al este) al angulo de la tabla.
static inline int trig_de_grados(int g) { return g * TRIG_VUELTA / 360; }

#endif
