#include "trig.h"
#include <math.h>

int16_t trig_sen_tab[TRIG_VUELTA];

// Arcotangente del primer octante: atan(i/128), de 0 a 45 grados.
static int16_t atan_tab[129];

void trig_init(void) {
    for (int i = 0; i < TRIG_VUELTA; i++)
        trig_sen_tab[i] = (int16_t)lrintf(sinf(2.0f * (float)M_PI * i / TRIG_VUELTA) * TRIG_UNO);
    for (int i = 0; i <= 128; i++)
        atan_tab[i] = (int16_t)lrintf(atanf((float)i / 128.0f) * TRIG_VUELTA / (2.0f * (float)M_PI));
}

int trig_atan2(int y, int x) {
    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;
    if (!ax && !ay) return 0;
    // Se resuelve en el primer octante y despues se refleja.
    int a = (ax >= ay) ? atan_tab[ay * 128 / ax]
                       : TRIG_VUELTA / 4 - atan_tab[ax * 128 / ay];
    if (x < 0) a = TRIG_VUELTA / 2 - a;
    if (y < 0) a = -a;
    return ((a % TRIG_VUELTA) + TRIG_VUELTA) % TRIG_VUELTA;
}
