#!/bin/bash
# Prueba el camino de los datos reales sin la placa: pide un lote al proxy (o
# usa el archivo que se le pase), lo hace pasar por el mismo parser y los
# mismos calculos que el firmware, y muestra las tarjetas que saldrian.
#
#   ./herramientas/probar_sky.sh              pide un lote nuevo a Ezeiza
#   ./herramientas/probar_sky.sh guardado.txt usa una respuesta ya guardada
set -e
cd "$(dirname "$0")/.."
SALIDA=$(mktemp -d)
trap 'rm -rf "$SALIDA"' EXIT

if [ -n "$1" ]; then
  RESPUESTA="$1"
else
  RESPUESTA="$SALIDA/respuesta.txt"
  echo "== pidiendo un lote al proxy =="
  node - > "$RESPUESTA" <<'JS'
const { default: handler } = await import("./sky-proxy/api/pico.js");
const res = { setHeader() {}, status() { return this; },
              send(b) { process.stdout.write(b); }, json(b) { process.stdout.write(JSON.stringify(b)); }, end() {} };
await handler({ method: "GET", query: { lat: -34.8220, lon: -58.5360, dist: 118, n: 32, tz: -180 } }, res);
JS
  echo "   $(wc -c < "$RESPUESTA" | tr -d ' ') bytes"
fi

cc -std=c11 -Wall -Wextra -Wno-unused-parameter -O1 \
   -o "$SALIDA/probar" \
   herramientas/probar_sky.c \
   firmware-c/sky_parse.c firmware-c/vivo.c \
   firmware-c/geo.c firmware-c/trig.c firmware-c/aeropuertos.c firmware-c/aerolineas.c

"$SALIDA/probar" "$RESPUESTA"
