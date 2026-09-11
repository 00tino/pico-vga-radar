// QR minimo para la pantalla de configuracion. Ver qr.c.
#ifndef QR_H
#define QR_H
#include <stdint.h>
#include <stdbool.h>

#define QR_LADO      29    // version 3
#define QR_TEXTO_MAX 53    // lo que entra en esa version con correccion L

// Arma el QR de ese texto. Devuelve false si no entra. En la matriz, 1 es un
// modulo oscuro.
bool qr_armar(const char *texto, uint8_t m[QR_LADO][QR_LADO]);

#endif
