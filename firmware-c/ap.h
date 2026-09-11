// Los servicios que la Pico presta mientras hace de router para el celular:
// repartir una IP y contestar cualquier nombre. Ver ap.c.
#ifndef AP_H
#define AP_H
#include <stdbool.h>

bool ap_servicios_arrancar(void);
void ap_servicios_parar(void);

#endif
