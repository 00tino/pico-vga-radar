#!/bin/bash
# Saca una foto del monitor VGA usando la GoPro, sin tener que pedirla.
#
# Preparacion, una vez por sesion:
#   1. Enchufar la GoPro por USB y ponerla en modo webcam.
#   2. Abrir QuickTime Player y hacer Archivo > Nueva grabacion de video.
#      Toma la HERO12 sola. NO hay que apretar grabar.
#   3. Averiguar el id de la ventana y pasarlo como argumento.
#
# Por que QuickTime y no otra cosa:
#   - Photo Booth espeja la imagen y se pausa cuando no esta al frente:
#     devuelve un cuadro viejo y parece que el monitor estuviera apagado.
#   - ffmpeg desde la terminal se cuelga: macOS no le da permiso de camara.
#   - Capturar la ventana por id (-l) anda aunque este en otro escritorio;
#     capturar por region no, porque macOS congela lo que no se ve.
#
# Uso: ./ver_monitor.sh <id_de_ventana> [salida.png]
set -e
ID="${1:?falta el id de la ventana de QuickTime}"
OUT="${2:-monitor.png}"
TMP="$(mktemp -t monitor).png"
screencapture -x -l "$ID" "$TMP"
ffmpeg -y -loglevel error -i "$TMP" -vf "scale=1400:-1" "$OUT"
rm -f "$TMP"
echo "$OUT"
