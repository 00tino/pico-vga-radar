#!/bin/bash
# Captura la ventana de QuickTime y le baja la exposicion.
#
# La GoPro expone para el cuarto oscuro y quema la pantalla: bajando gamma y
# brillo al procesar se recupera el detalle del monitor, que es lo que
# interesa. Idea de Valentino, que lo probo con el telefono.
IN="${1:?falta la imagen}"
OUT="${2:-vista.png}"
ffmpeg -y -loglevel error -i "$IN" \
  -vf "eq=brightness=-0.09:contrast=1.38:gamma=0.85:saturation=1.2" "$OUT"
echo "$OUT"
