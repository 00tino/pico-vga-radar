# Pinout VGA — Pico 2 W + PicoVGA

El Wi-Fi del Pico 2 W usa GP23, GP24, GP25 y GP29 (internos). No conectes nada ahi.
PicoVGA usa GP0–GP8. No hay conflicto.

## Tabla

| Senal | Pin VGA DE-15 | GPIO Pico | Resistencia |
|--------|----------------|-----------|-------------|
| Blue bit0 | 3 Blue | GP0 | 1 kΩ |
| Blue bit1 | 3 Blue | GP1 | 390 Ω |
| Green bit0 | 2 Green | GP2 | 2.2 kΩ |
| Green bit1 | 2 Green | GP3 | 1 kΩ |
| Green bit2 | 2 Green | GP4 | 470 Ω |
| Red bit0 | 1 Red | GP5 | 2.2 kΩ |
| Red bit1 | 1 Red | GP6 | 1 kΩ |
| Red bit2 | 1 Red | GP7 | 470 Ω |
| HSYNC / CSYNC | 13 | GP8 | 100 Ω |
| VSYNC (si el monitor no acepta CSYNC) | 14 | GP9 | 100 Ω |
| GND | 5, 6, 7, 8, 10 | GND | directo |

Los tres bits de cada color se unen **despues** de las resistencias y van al pin de color.

Modo de video inicial: 640x480 @ 60 Hz.
