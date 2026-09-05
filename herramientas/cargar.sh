#!/bin/bash
# Compila y carga el firmware en la Pico, sin tocar el boton BOOTSEL (que esta
# tapado por la protoboard).
#
# Va por picotool, que habla el protocolo PICOBOOT por USB: reinicia la placa
# el solo y escribe la flash directo. El camino viejo era copiar el .uf2 al
# disco NO NAME, pero ese disco a veces no lo monta macOS aunque la placa este
# bien: aparece como "RP2350 Boot" en el bus USB y en /Volumes no hay nada, y
# entonces no habia forma de cargar. Por PICOBOOT eso no importa.
#
# picotool viene de brew (brew install picotool).
set -e
cd "$(dirname "$0")/../firmware-c"
./compilar.sh > /dev/null
picotool load -f -x build/radar.uf2

for i in $(seq 1 20); do ls /dev/cu.usbmodem* >/dev/null 2>&1 && break; sleep 1; done
# El puerto queda en 1200 baudios despues del reinicio: abrirlo asi sin el
# stty manda la placa de vuelta a BOOTSEL y corta el video.
stty -f /dev/cu.usbmodem11201 115200 2>/dev/null || true
echo "cargado"
