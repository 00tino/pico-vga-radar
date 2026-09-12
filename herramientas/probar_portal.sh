#!/bin/bash
# Prueba que lo que manda docs/setup.html sea lo que entiende el firmware.
#
# La direccion NO se escribe aca: se saca del propio setup.html, ejecutando su
# codigo de verdad. Si alguien le cambia el formato a la pagina, esto se
# entera; si se copiara el codigo, no.
set -e
cd "$(dirname "$0")/.."
SALIDA=$(mktemp -d)
trap 'rm -rf "$SALIDA"' EXIT

# Se recorta de la pagina el pedazo que arma la direccion y se lo corre con
# una configuracion de prueba.
node - > "$SALIDA/consulta.txt" <<'JS'
import { readFileSync } from "node:fs";
const html = readFileSync("docs/setup.html", "utf8");

// Desde donde se declaran los numeros de vista hasta el final de paraLaPlaca.
const desde = html.indexOf("const VISTA_N");
const hasta = html.indexOf("$(\"guardar\").onclick");
if (desde < 0 || hasta < 0) {
  console.error("no se encontro el codigo que arma la direccion en setup.html");
  process.exit(2);
}
const codigo = html.slice(desde, hasta);

const cfg = {
  tema: "crt_amber",
  casa: { on: true, lat: "-34.5586", lon: "-58.4134", km: 5 },
  pantallas: [
    { nombre: "Vuelos de Ezeiza", vista: "hybrid", apt: "EZE", km: 220,
      lista: "fa", vuelos: 4, seguir: "", segundos: 15 },
    { nombre: "Siguiendo AA954", vista: "follow_hybrid", apt: "eze", km: 220,
      lista: "fa", vuelos: 1, seguir: "aa954", segundos: 30 },
    { nombre: "Aeroparque", vista: "wall", apt: "AEP", km: 80,
      lista: "fids", vuelos: 6, seguir: "", segundos: 120 },
  ],
};
const paraElEquipo = () => JSON.parse(JSON.stringify(cfg));
const armar = new Function("paraElEquipo", codigo + "; return paraLaPlaca();");
process.stdout.write(armar(paraElEquipo));
JS

cc -std=c11 -Wall -Wextra -O1 -o "$SALIDA/probar" \
   herramientas/probar_portal.c firmware-c/portal_parse.c

"$SALIDA/probar" "$(cat "$SALIDA/consulta.txt")"
