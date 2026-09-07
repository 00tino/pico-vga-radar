// Lo minimo del SDK de la Pico para que el codigo de dibujo compile tambien
// para el navegador. El dibujo no toca el hardware: lo unico que usa de aca
// es el reloj, y en la web alcanza con uno de mentira.
#ifndef PICO_STDLIB_WEB_H
#define PICO_STDLIB_WEB_H
#include <stdint.h>
#include <stdbool.h>

uint32_t time_us_32(void);
static inline void tight_loop_contents(void) {}
static inline void sleep_ms(uint32_t ms) { (void)ms; }

#endif
