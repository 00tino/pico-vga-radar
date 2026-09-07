#!/bin/bash
# Compila el codigo de dibujo del firmware para el navegador.
#
# Es el mismo C que corre en la Pico: gfx, radar, tarjetas, viaje, los datos y
# las fuentes. Lo unico que se reemplaza es vga.c, que en la placa maneja el
# PIO y el DMA y aca es solo el framebuffer.
#
# Dos cosas de esta Mac:
#   - emscripten pide python 3.10 o mas y el del sistema es 3.9, asi que hay
#     que apuntarle al de brew con EMSDK_PYTHON.
#   - su configuracion vive en el .emscripten del Cellar, con el LLVM propio
#     de emscripten (el de las Command Line Tools no tiene los targets wasm) y
#     binaryen aparte.
set -e
cd "$(dirname "$0")"
export EMSDK_PYTHON=/opt/homebrew/bin/python3.12

FW=../firmware-c
SALIDA=../docs/radar-firmware.js

emcc \
  wasm.c vga_web.c \
  $FW/gfx.c $FW/fuente.c $FW/fuente_grande.c $FW/area.c $FW/trig.c $FW/geo.c \
  $FW/radar.c $FW/tarjetas.c $FW/viaje.c $FW/logos_pantalla.c $FW/pantallas.c \
  $FW/logos.c $FW/pistas.c $FW/aeropuertos.c $FW/costas.c $FW/demo.c \
  -I. -I$FW \
  -O2 \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=RadarFirmware \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT=web \
  -o "$SALIDA"

echo "generado $SALIDA"
ls -lh "$SALIDA" "${SALIDA%.js}.wasm" | awk '{print "  " $9 "  " $5}'
