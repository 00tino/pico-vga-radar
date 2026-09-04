#!/bin/bash
# Compila el firmware en C para la Pico 2 W.
#
# Uso:  ./compilar.sh          compila
#       ./compilar.sh limpio   borra y compila de cero
#
# Dos peculiaridades de esta Mac, ya contempladas:
#  1. El Command Line Tools tiene la carpeta de headers de C++ incompleta
#     (1 archivo en vez de 185). Se apunta a la copia buena del SDK de macOS
#     con CPLUS_INCLUDE_PATH.
#  2. Esa variable no puede estar activa al compilar para ARM, porque el
#     compilador cruzado agarra los headers de macOS y explota. Por eso el
#     build va en dos fases: primero las herramientas del host, despues el
#     firmware.

set -e
cd "$(dirname "$0")"

export PICO_SDK_PATH="$HOME/pico-sdk"
export PICO_TOOLCHAIN_PATH="$HOME/arm-toolchain"
export PATH="$HOME/arm-toolchain/bin:$PATH"
MACSDK="$(xcrun --show-sdk-path)"

[ -d "$PICO_SDK_PATH" ] || { echo "Falta el SDK en $PICO_SDK_PATH"; exit 1; }
[ -x "$PICO_TOOLCHAIN_PATH/bin/arm-none-eabi-gcc" ] || { echo "Falta el toolchain ARM en $PICO_TOOLCHAIN_PATH"; exit 1; }

[ "$1" = "limpio" ] && rm -rf build
mkdir -p build && cd build

echo "== fase 1: herramientas del Mac (pioasm, picotool) =="
CPLUS_INCLUDE_PATH="$MACSDK/usr/include/c++/v1" cmake -DPICO_BOARD=pico2_w .. > cmake.log 2>&1 \
  || { echo "fallo cmake:"; tail -15 cmake.log; exit 1; }
CPLUS_INCLUDE_PATH="$MACSDK/usr/include/c++/v1" make -j4 > host.log 2>&1 || true

echo "== fase 2: firmware ARM =="
make -j4 > build.log 2>&1 \
  || { echo "fallo la compilacion:"; grep -iE "error:|fatal error|cannot find" build.log | head -10; exit 1; }

echo
echo "Listo:"
ls -lh *.uf2 2>/dev/null | awk '{print "  " $9 "  " $5}'
arm-none-eabi-size *.elf 2>/dev/null | tail -n +1
echo
echo "Para cargarlo: enchufa la Pico con BOOTSEL apretado y copia el .uf2 al disco RP2350."
