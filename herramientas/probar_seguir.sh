#!/bin/bash
# Prueba a quien sigue el radar cuando se le pide seguir un vuelo. Ver
# herramientas/probar_seguir.c.
set -e
cd "$(dirname "$0")/.."
SALIDA=$(mktemp -d)
trap 'rm -rf "$SALIDA"' EXIT
cc -std=c11 -Wall -Wextra -O1 -o "$SALIDA/probar" \
   herramientas/probar_seguir.c firmware-c/seguir.c firmware-c/aerolineas.c
"$SALIDA/probar"
