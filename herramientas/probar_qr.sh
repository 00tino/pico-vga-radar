#!/bin/bash
# Comprueba que el QR que dibuja el firmware se LEE de verdad.
#
# No alcanza con mirar que la matriz sea igual a la de otra implementacion:
# las dos pueden estar mal. Aca se arma la imagen y se la hace leer por el
# lector de codigos de macOS, que es el mismo que usa la camara del telefono.
#
#   ./herramientas/probar_qr.sh                  prueba la del portal
#   ./herramientas/probar_qr.sh http://otra.cosa  prueba otra
set -e
cd "$(dirname "$0")/.."
TEXTO="${1:-http://192.168.4.1}"
SALIDA=$(mktemp -d)
trap 'rm -rf "$SALIDA"' EXIT

cat > "$SALIDA/main.c" <<'C'
#include "qr.h"
#include <stdio.h>
int main(int c, char **v) {
    uint8_t m[QR_LADO][QR_LADO];
    if (!qr_armar(c > 1 ? v[1] : "http://192.168.4.1", m)) { puts("NO ENTRA"); return 1; }
    for (int j = 0; j < QR_LADO; j++) {
        for (int i = 0; i < QR_LADO; i++) putchar(m[j][i] ? '#' : '.');
        putchar('\n');
    }
    return 0;
}
C
cc -O1 -Ifirmware-c -o "$SALIDA/qr" "$SALIDA/main.c" firmware-c/qr.c
"$SALIDA/qr" "$TEXTO" > "$SALIDA/qr.txt"

python3 - "$SALIDA" <<'PY'
import sys
from PIL import Image
d = sys.argv[1]
filas = [l.strip() for l in open(d + "/qr.txt") if l.strip()]
q, borde = 8, 32
lado = len(filas) * q + 2 * borde
img = Image.new("L", (lado, lado), 255)
px = img.load()
for j, fila in enumerate(filas):
    for i, ch in enumerate(fila):
        if ch == "#":
            for dy in range(q):
                for dx in range(q):
                    px[borde + i * q + dx, borde + j * q + dy] = 0
img.save(d + "/qr.png")
PY

cat > "$SALIDA/leer.swift" <<'SWIFT'
import Foundation
import CoreImage
let url = URL(fileURLWithPath: CommandLine.arguments[1])
guard let img = CIImage(contentsOf: url) else { print("no se pudo abrir"); exit(2) }
let det = CIDetector(ofType: CIDetectorTypeQRCode, context: nil,
                     options: [CIDetectorAccuracy: CIDetectorAccuracyHigh])!
let leidos = det.features(in: img).compactMap { ($0 as? CIQRCodeFeature)?.messageString }
if leidos.isEmpty { print("NO SE DETECTO NINGUN QR"); exit(1) }
for l in leidos { print(l) }
SWIFT

LEIDO=$(swift "$SALIDA/leer.swift" "$SALIDA/qr.png")
echo "esperado: $TEXTO"
echo "leido:    $LEIDO"
[ "$LEIDO" = "$TEXTO" ] && echo "todo bien" || { echo "HAY FALLAS"; exit 1; }
