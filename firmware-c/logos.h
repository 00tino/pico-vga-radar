// Logos de aerolineas, generado por herramientas/logos_a_c.py. NO EDITAR.
// 824 logos de 36x36, un byte RRRGGGBB por pixel.
#ifndef LOGOS_H
#define LOGOS_H
#include <stdint.h>

#define LOGO_LADO 36
#define LOGO_BYTES (36 * 36)
#define LOGOS_CANT 824

typedef struct {
    char codigo[3];       // IATA de dos letras
    uint32_t offset;      // donde empiezan sus pixeles
} logo_t;

extern const logo_t logos_indice[LOGOS_CANT];
extern const uint8_t logos_datos[];

// Devuelve los pixeles del logo, o NULL si esa aerolinea no tiene.
const uint8_t *logo_buscar(const char *codigo_iata);

#endif
