# Firmware en C — Pico 2 W

Reescritura del radar en C, para que el monitor muestre lo mismo que la web.

**Por qué C:** en MicroPython dibujar un cuadro entero tarda cientos de
milisegundos. No alcanza para las tarjetas con logos, el barrido fluido ni los
256 colores. En C el mismo dibujo va decenas de veces mas rapido y el
framebuffer entra en RAM con margen.

## Objetivo

- **640x480 con 256 colores** (3 bits de rojo, 3 de verde, 2 de azul)
- Framebuffer de 307 KB en RAM, de los 520 KB de la placa
- WiFi, portal de configuracion y QR, igual que la version MicroPython
- Render con el diseño de la web: tarjetas con logo, ruta, horarios, barra de
  progreso, pistas, senda de aproximacion, temas y barrido animado

## Cableado

El de `hardware/PINOUT.md` (GP0 a GP9), **mas las masas del sync**: los
agujeros 5 y 10 del VGA tambien van a masa, no alcanza con el 6. Eso lo
aprendimos a los golpes con la version MicroPython.

## Compilar

    ./compilar.sh

Genera el `.uf2` en `build/`. Para cargarlo: enchufar la Pico con BOOTSEL
apretado y copiar el archivo al disco `RP2350` que aparece.

## Herramientas (ya instaladas en esta Mac)

- `cmake` por Homebrew
- SDK de la Pico en `~/pico-sdk`
- Toolchain oficial de ARM 14.3 en `~/arm-toolchain`

Ver los comentarios de `compilar.sh`: esta Mac necesita dos rodeos por un
Command Line Tools incompleto.
