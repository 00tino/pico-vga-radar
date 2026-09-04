#!/bin/bash
# Copia todo el radar a la Pico de una. Reemplaza el arrastrar archivos en Thonny.
#
# Uso:  ./cargar.sh
#
# Antes: la Pico tiene que tener MicroPython instalado (Paso 1 del README) y
# estar enchufada por USB.

set -e
cd "$(dirname "$0")"

MP="$HOME/Library/Python/3.9/bin/mpremote"
[ -x "$MP" ] || MP="mpremote"

if ! "$MP" devs 2>/dev/null | grep -qi "MicroPython\|2e8a"; then
  echo "No encuentro la Pico."
  echo
  echo "  - Fijate que este enchufada por USB."
  echo "  - Si recien le pusiste MicroPython, desenchufala y volve a enchufarla"
  echo "    SIN apretar el boton BOOTSEL."
  exit 1
fi

echo "Pico encontrada. Copiando..."

"$MP" fs mkdir :VGA 2>/dev/null || true
"$MP" fs mkdir :fonts 2>/dev/null || true

for f in main.py ui.py qr.py wifi.py portal.py store.py sky.py render.py \
         test_video.py config.json install.json; do
  echo "  $f"
  "$MP" fs cp "$f" ":$f"
done

for f in VGA/*.py; do
  echo "  $f"
  "$MP" fs cp "$f" ":$f"
done

for f in fonts/*.c; do
  echo "  $f"
  "$MP" fs cp "$f" ":$f"
done

echo
echo "Listo. Archivos en la Pico:"
"$MP" fs ls

echo
echo "Para probar el video:   $MP run test_video.py"
echo "Para ver que hace:      $MP repl     (salis con Ctrl-])"
