// Logos de aerolineas, generado por herramientas/logos_a_c.py. NO EDITAR.
// 824 logos de 36x36, un byte RRRGGGBB por pixel.
#ifndef LOGOS_H
#define LOGOS_H
#include <stdint.h>

#define LOGO_LADO 36
#define LOGO_BYTES (36 * 36)
#define LOGO_MASCARA_BYTES ((36 * 36 + 7) / 8)
#define LOGOS_CANT 824

typedef struct {
    char codigo[3];       // IATA de dos letras
    uint32_t offset;      // donde empiezan sus pixeles
} logo_t;

extern const logo_t logos_indice[LOGOS_CANT];
extern const uint8_t logos_datos[];

// Un bit por pixel: 1 es logo, 0 es el fondo de la caja. Sin esto el logo se
// dibuja como un cuadrado blanco sobre el radar, que es oscuro, y queda un
// parche alrededor de cada uno.
extern const uint8_t logos_mascaras[];

// Devuelve los pixeles del logo, o NULL si esa aerolinea no tiene.
const uint8_t *logo_buscar(const char *codigo_iata);

// La mascara del mismo logo, para dibujarlo sin su fondo.
const uint8_t *logo_mascara(const char *codigo_iata);

#endif
