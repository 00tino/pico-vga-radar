# Cableado Pico 2 W → monitor VGA

**Este cableado reemplaza al de `hardware/PINOUT.md`.** Aquel era para un
esquema de 8 bits de color que hay que programar de cero. Este usa la
librería de video que ya viene probada, y son 5 cables en vez de 10.

Necesitás: un conector VGA macho (DE-15) o un cable VGA viejo cortado,
3 resistencias de 220 Ω y 2 de 47 Ω.

| Señal | Pin del VGA | GPIO de la Pico | Resistencia |
|---|---|---|---|
| Rojo | 1 | GP18 | 220 Ω |
| Verde | 2 | GP19 | 220 Ω |
| Azul | 3 | GP20 | 220 Ω |
| VSYNC | 14 | GP21 | 47 Ω |
| HSYNC | 13 | GP22 | 47 Ω |
| Masa (GND) | 5, 6, 7, 8, 10 | cualquier GND | cable directo |

La resistencia va **en serie**: sale del GPIO, pasa por la resistencia, y de
ahí al pin del VGA.

Los pines 5, 6, 7, 8 y 10 del VGA van todos juntos a un mismo GND de la Pico.

## Cosas a tener en cuenta

- **No uses GP23, GP24, GP25 ni GP29.** Son del chip de WiFi, están adentro
  de la placa. Si les colgás algo, se cae la conexión.
- Son 8 colores (un bit por canal), no hay tonos intermedios. Para un radar
  ámbar sobre negro sobra.
- El modo de video es **800x600 a 60 Hz**. Cualquier monitor VGA lo toma.
- Si el monitor dice "sin señal", el problema casi siempre es el GND o que
  HSYNC y VSYNC están cruzados (13 y 14).
