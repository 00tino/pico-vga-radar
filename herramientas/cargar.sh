#!/bin/bash
# Compila y carga el firmware en la Pico, sin tocar el boton BOOTSEL.
#
# El reinicio se hace abriendo el puerto serie a 1200 baudios. Despues hay que
# esperar a que el disco NO NAME termine de montarse: copiar apenas aparece
# falla en silencio y la placa se queda en BOOTSEL, con el monitor sin señal.
# Por eso se espera, se copia y se verifica que el disco se haya desmontado,
# que es lo que hace la Pico cuando arranca el firmware nuevo.
set -e
cd "$(dirname "$0")/../firmware-c"
./compilar.sh > /dev/null

python3 - <<'PY'
import sys, time, glob
sys.path.insert(0, "/Users/valentino/Library/Python/3.9/lib/python/site-packages")
import serial
p = glob.glob("/dev/cu.usbmodem*")
if p:
    s = serial.Serial(p[0], 1200); s.dtr = False; time.sleep(0.4); s.close()
PY

for i in $(seq 1 20); do [ -d "/Volumes/NO NAME" ] && break; sleep 1; done
[ -d "/Volumes/NO NAME" ] || { echo "no aparecio el disco NO NAME"; exit 1; }
sleep 3                                  # que termine de montar

cat build/radar.uf2 > "/Volumes/NO NAME/radar.uf2" 2>/dev/null || true
sync
for i in $(seq 1 20); do [ -d "/Volumes/NO NAME" ] || break; sleep 1; done
if [ -d "/Volumes/NO NAME" ]; then
  echo "la copia no tomo, reintentando"
  cat build/radar.uf2 > "/Volumes/NO NAME/radar.uf2" 2>/dev/null || true
  sync
  for i in $(seq 1 20); do [ -d "/Volumes/NO NAME" ] || break; sleep 1; done
fi
[ -d "/Volumes/NO NAME" ] && { echo "FALLO: sigue en BOOTSEL"; exit 1; }

for i in $(seq 1 20); do ls /dev/cu.usbmodem* >/dev/null 2>&1 && break; sleep 1; done
stty -f /dev/cu.usbmodem11201 115200 2>/dev/null || true
echo "cargado"
