# Traspaso — Pico VGA Radar

Documento para retomar el proyecto en una sesión nueva, **sin contexto previo**.
Escrito el **5 de septiembre de 2026**, al final de una sesión larga.

Repo: `~/Desktop/Proyectos/PicoVga` → https://github.com/00tino/pico-vga-radar
Rama `main`. **Hay cambios sin commitear** (ver sección 9).

---

## 1. Qué es el proyecto

Un **radar de tráfico aéreo en un monitor VGA**, impulsado por una Raspberry Pi
Pico 2 W. Valentino lo va a **vender armado**, así que hay dos roles:

- **Él es el instalador**: define pulgadas y resolución del monitor de cada
  equipo. Queda grabado en la placa y el cliente no lo ve.
- **El cliente** recibe el equipo, lo enchufa, aparece un QR, lo escanea con el
  celular y elige aeropuerto, vista, filtros y estilo.

| Parte | Dónde | Qué hace |
|---|---|---|
| **Web** | `docs/` → GitHub Pages | Simulador del radar y página de configuración |
| **Proxy** | `sky-proxy/` → Vercel | Sirve datos ADS-B ya masticados |
| **Firmware** | `firmware-c/` | Lo que corre en la placa (en C) |

La web publicada: https://00tino.github.io/pico-vga-radar/sim.html

### El requisito que manda

> **"Lo que se ve en la web se tiene que ver en la Pico."**

Es innegociable y hay que tomarlo al pie de la letra: **todas las opciones de la
web, sí o sí**. Cuando algo no coincide, la referencia es `docs/radar.js`.

`pico/` tiene el firmware viejo en MicroPython. **Está abandonado, no volver
ahí**: 8 colores y dibujado lento.

---

## 2. Estado actual del firmware en C

`firmware-c/`. **1,60 MB de flash de 4 MB**, 365 KB de RAM de 520 KB, **60 fps
verificados** en todas las vistas.

### Lo que ya funciona, todo visto en el monitor

- **Video** (`vga.c`, `vga.pio`): 640×480, 256 colores, framebuffer de 307 KB.
- **Primitivas** (`gfx.c`): texto (fuente Cascadia 7×14 con escalado), líneas,
  círculos, rectángulos, triángulos, sectores, polígonos con relleno y recorte,
  blits, guardar/reponer rectángulos, y color con dithering ordenado.
- **Área útil** (`area.c`): márgenes por borde con pantalla de calibración. En
  el ViewSonic quedó en 4 px por lado.
- **Datos en flash**, todos generados por scripts en `herramientas/`:
  - `logos.c` — los 824 logos de aerolíneas, 36×36, **1,02 MB**
  - `pistas.c` — las 5600 pistas de 4040 aeropuertos, **190 KB**
  - `aeropuertos.c` — 5334 aeropuertos con ciudad y coordenadas, **210 KB**
  - `costas.c` — costas del mundo (53 contornos) y **fronteras de países**
    (333 tramos), **16 KB**
- **El scope** (`radar.c`): barra de encabezado, anillos, cruz, etiquetas de
  alcance, barrido con cuña, aviones con rumbo y efecto fósforo, pistas, senda
  de aproximación, y la casa con su radio de aviso.
- **Tarjetas** (`tarjetas.c`): logo, indicativo, tipo, estado, ruta con barra de
  avance y avioncito, ciudades, horarios, puntualidad y métricas. Reparten el
  alto disponible y sacan filas cuando no entran.
- **Formato aeropuerto (FIDS)**: tabla con logo, vuelo, ruta, horarios, estado,
  velocidad/rumbo y puntualidad.
- **Vista de seguimiento** (`viaje.c`): mapa mundial con costas y fronteras,
  ruta por círculo máximo, rejilla, rastro recorrido, origen y destino.
- **Seis vistas**: sólo radar, híbrida, pared de tarjetas, seguimiento,
  seguimiento con tarjeta, y pantalla de logos.
- **Los 14 temas de color** de la web.
- **Tráfico de prueba** (`demo.c`): 15 vuelos que se mueven de verdad.

`main.c` recorre **17 escenas de 15 segundos** para poder revisarlas todas.
Cuando esté el portal, la vista saldrá de la configuración del cliente.

### Lo que falta del proyecto

WiFi, portal de configuración, QR y datos ADS-B reales. **Nada de eso está
empezado.** El tráfico es simulado.

---

## 3. Cómo trabajar

### Compilar y cargar

```
./herramientas/cargar.sh
```

Compila y carga sin tocar el botón BOOTSEL (que está tapado por la protoboard).
Usa el truco de abrir el puerto serie a 1200 baudios.

**No copiar el .uf2 a mano.** El script espera a que el disco `NO NAME` termine
de montarse y verifica que la placa haya arrancado: copiar apenas aparece el
disco falla en silencio y deja la placa en BOOTSEL, con el monitor sin señal.

Compilar solo: `cd firmware-c && ./compilar.sh`. Contempla dos rarezas de esta
Mac, explicadas en sus comentarios:
- El Command Line Tools tiene los headers de C++ incompletos; se apunta a la
  copia buena del SDK de macOS con `CPLUS_INCLUDE_PATH`.
- Esa variable no puede estar activa al compilar para ARM, así que el build va
  en dos fases: primero las herramientas del host, después el firmware.

Herramientas: `cmake` por brew, SDK en `~/pico-sdk`, toolchain ARM 14.3 en
`~/arm-toolchain`. **El `arm-none-eabi-gcc` de brew NO sirve**: viene sin newlib.

### Leer la consola

```
stty -f /dev/cu.usbmodem11201 115200
perl -e 'alarm 15; exec @ARGV' cat /dev/cu.usbmodem11201
```

**El `stty` no es opcional.** El puerto se queda en 1200 baudios después del
truco de reinicio, y abrirlo así manda la placa a BOOTSEL y corta el video.

### Ver el monitor sin pedirle fotos a Valentino

Está autorizado usar la **GoPro HERO12** como ojos:

1. Abrir QuickTime Player → Archivo → **Nueva grabación de vídeo**. Toma la
   HERO12 sola. **No** hay que apretar grabar.
2. `perl -e 'alarm 20; exec @ARGV' screencapture -x -l <id_ventana> foto.png`
3. `./herramientas/bajar_brillo.sh foto.png vista.png` y leer `vista.png`.

**Dos capturas seguidas**: macOS sólo redibuja la ventana cuando algo la toca,
así que la primera devuelve el cuadro anterior. La buena es la segunda.

El id de ventana se saca con las herramientas de control de aplicaciones
(`app_list_windows` sobre `com.apple.QuickTimePlayerX`). En esta sesión fue
`11641`, pero **cambia cada vez que se reabre la grabación**.

Por qué así y no de otra forma:
- **Photo Booth no sirve**: espeja la imagen y se pausa cuando no está al
  frente, devolviendo un cuadro viejo.
- **ffmpeg desde la terminal se cuelga**: macOS no le da permiso de cámara.
- El brillo: la GoPro expone para el cuarto oscuro y quema la pantalla.
  `bajar_brillo.sh` recupera el detalle. Fue idea de Valentino.

### Regenerar los datos

```
python3 herramientas/logos_a_c.py        # 824 logos
python3 herramientas/pistas_a_c.py       # 5600 pistas
python3 herramientas/aeropuertos_a_c.py  # 5334 aeropuertos
python3 herramientas/costas_a_c.py       # costas y fronteras
```

**El dithering está duplicado** en `logos_a_c.py` y en `gfx.c`: si se toca uno
hay que tocar el otro, porque los dos tienen que cuantizar igual.

---

## 4. Hardware

### Cableado (verificado)

La Pico está en protoboard con la serigrafía visible. **Regla que costó horas:
leer siempre la etiqueta impresa (`GP0`, `GP1`…), nunca contar pines.**

| Pin | Señal | Resistencia | Agujero VGA |
|---|---|---|---|
| `GP0` | azul bit0 | 1 kΩ | 3 |
| `GP1` | azul bit1 | 500 Ω | 3 |
| `GP2` | verde bit0 | 1,95 kΩ | 2 |
| `GP3` | verde bit1 | 1 kΩ | 2 |
| `GP4` | verde bit2 | 500 Ω | 2 |
| `GP5` | rojo bit0 | 1,95 kΩ | 1 |
| `GP6` | rojo bit1 | 1 kΩ | 1 |
| `GP7` | rojo bit2 | 500 Ω | 1 |
| `GP8` | hsync | directo | 13 |
| `GP9` | vsync | directo | 14 |
| `GND` | masa | directo | **5, 6, 7, 8 y 10** |

**Las cinco masas del VGA son obligatorias**: con una sola el monitor perdía el
enganche cada pocos segundos. Del otro lado de la Pico no sale ningún cable: ahí
están `VBUS`, `VSYS`, `3V3` y `3V3_EN`, que apagan la placa si se tocan.

### Monitor de prueba: ViewSonic VA1703wb

- **Estira el 4:3 a pantalla ancha.** Los círculos salen ovalados y los ángulos
  se ven achatados: parte de lo que Valentino ve como "la aproximación apunta
  mal" es este estiramiento. Se corrige en el menú del monitor, o habría que
  compensarlo por software.
- **El negro está muy levantado.** Con una imagen oscura la cámara sube la
  exposición y el fondo se ve gris azulado. No es un problema de la señal.

### Cuando la Pico desaparece del USB

Pasó dos veces en esta sesión. Síntomas y qué hacer:

- **`ls /Volumes` se cuelga** y todo comando que toque el disco se queda
  esperando hasta el timeout: el disco `NO NAME` quedó colgado a mitad de una
  escritura. A veces se destraba solo (queda como "Device not configured").
- **No hay ni puerto serie ni disco**: la placa se fue del bus por completo.
  **Eso no se arregla por software: hay que pedirle a Valentino que desenchufe
  y vuelva a enchufar el cable USB.**

---

## 5. Bugs resueltos — no repetirlos

### Hardware

1. **Los dos cables micro-USB eran de sólo carga.** El LED apagado **no** es
   síntoma de nada: en la Pico 2 W el LED cuelga del chip de WiFi.
2. **La placa estaba montada al revés** y la numeración salía espejada.
3. **Los cables en pines muertos** por confundir "pin físico 21" con `GP21`.
4. **Faltaban las masas del sincronismo** (agujeros 5 y 10 del VGA).
5. **Cruce físico entre `GP2` y `GP3`** en la protoboard.

### Video y dibujo

6. **252 MHz de reloj deja la placa muerta.** Con **100,8 MHz** los tres
   divisores dan enteros y no hace falta sobrefrecuencia.
7. **Los tres programas de PIO suman 31 de 32 instrucciones.** Queda 1 libre.
8. **El DMA debe transferir de a 32 bits**, no de a byte.
9. **Hay que reiniciar el DMA en cada cuadro** desde la interrupción de vsync.
10. **No hay RAM para doble buffer** (307 KB cada uno, hay 520 KB). Se dibuja
    sobre el mismo buffer que el monitor lee, así que **hay que pintar por
    bandas de arriba hacia abajo**, cada una antes de que el haz llegue. Sin
    esto, lo que se dibuja último arriba nunca se ve.
11. **Las bandas no pueden ser todas iguales.** Al arrancar sólo se lleva de
    ventaja el borrado vertical (1400 µs) y una banda pareja ya costaba más.
    Ahora son finas arriba y anchas abajo: `BANDA_ALTO` en `radar.c`.
12. **Dentro de cada banda, el orden importa**: el encabezado se dibuja primero
    porque está en las filas de más arriba.
13. **Lo que no cambia no se redibuja.** El mapa se dibuja una sola vez y el
    avión se mueve como calco (guardar/reponer el fondo). Las tarjetas se
    rehacen de a una por cuadro y el FIDS en cuatro pasadas. Sin esto: 188 ms
    por cuadro en el mapa (5 fps) y picos de 22 ms en la vista híbrida.
14. **Rellenar un sector con rayos desde el centro deja 18% de huecos.** Va por
    filas. Lo mismo los círculos: Bresenham repetido por banda costaba varios
    milisegundos.
15. **La raíz cuadrada por fila es cara.** Entre filas vecinas el radio cambia
    poco: se arranca del valor anterior y se ajusta con dos comparaciones.
16. **Cuidado con premultiplicar por `TRIG_UNO` y dividir una sola vez.** Los
    triángulos salían con vértices a ±6000 px: 12 fps en vez de 60.
17. **Al cambiar de vista hay que limpiar la pantalla entera**, en un cuadro
    para ella sola. Cada vista limpia sólo su parte y quedaban restos.
18. **`vga_rgb` redondea, no trunca.** Y **el azul no se dithera**: tiene 4
    niveles, escalones de 85, y un píxel encendido en zona oscura se ve sucio.

### Mapa

19. **Las longitudes hay que envolverlas respecto de un ancla** (`wrap_lon`).
    Sin eso un Sydney-Buenos Aires cruza el mundo por el lado largo.
20. **`geo_km` es plana**: sirve para el alcance de un radar, no para un vuelo
    intercontinental. Para eso está `km_gc` en `viaje.c`.
21. **El límite de puntos por contorno tiene que dar para los continentes**:
    tienen hasta 776 puntos y con 512 se descartaba América entera.
22. **La escala en píxeles por grado va en centésimas**: con enteros pelados
    2,9 se volvía 2 y el mapa salía mucho más chico.
23. **El rango visible se recalcula después de fijar la escala**, que es la
    misma en los dos ejes. Si no, se descartan continentes que sí se ven.
24. **Los polígonos se recortan contra el recuadro con Sutherland-Hodgman.**
    Acotar los puntos sueltos al borde junta vértices lejanos y el relleno se
    escapa en franjas a lo ancho de la pantalla.
25. **Las latitudes se acotan a ±78°**: más cerca del polo esta proyección
    estira sin fin y la Antártida sale aplastada.

### Geometría del radar

26. **La proyección del scope corrige por el coseno de la latitud.** Sin eso los
    rumbos se ven torcidos: la aproximación a la 11 de Ezeiza parecía la 09.
    (La web tiene este defecto; acá está corregido.)
27. **La senda de aproximación se dibuja extendiendo el eje de la pista tal
    como quedó en pantalla**, no proyectando por lat/lon.
28. **El avión viene por detrás de la cabecera donde aterriza**, no por
    delante. Con el signo al revés la senda salía para el lado de adentro.

### Serie y carga

29. **El puerto se queda en 1200 baudios** después del reinicio: un `cat` sin
    `stty` manda la placa a BOOTSEL.
30. **Copiar el .uf2 apenas aparece el disco falla en silencio.** Usar
    `herramientas/cargar.sh`.

---

## 6. Los logos: resuelto

El planteo viejo decía que no entraban. Estaba mal: los 4,2 MB son el peso del
**formato** PNG a 64×64 con color verdadero. En el formato de la pantalla
—36×36, un byte por píxel— los 824 ocupan **1,02 MB**, medidos. Van todos, y el
cliente puede configurar cualquier aeropuerto del mundo.

**Las esquinas transparentes**: los PNG traen las esquinas vacías porque en la
web el logo va en una caja redondeada. Componerlos sobre blanco dejaba puntas
blancas. Elegir "el color del borde" tampoco sirve, porque British Airways y
Emirates tienen el fondo en **degradado**. La solución que quedó es extender el
color hacia las zonas transparentes antes de escalar (`rellenar_transparente`).

**144 de los 824 logos son casi todos blancos** (fondo blanco con trazo fino):
sobre fondo oscuro parecen vacíos. Por eso la pantalla de logos les pone marco.

---

## 7. Cómo trabaja Valentino

- **Español argentino, de vos, directo.** Sin preámbulos ni adulación.
- **Respuestas cortas.** No narrar cada paso ni repetir lo que dijo.
- Si da instrucciones claras, **trabajar solo y contar al final**.
- **Marcar lo que queda a medias.** Prefiere un "esto NO está" antes que una
  línea optimista.
- **No pushear sin avisar.** El repo es público. **Nada está pusheado todavía.**
- **Medir antes que adivinar.** Cada vez que se midió apareció la causa en
  minutos; cada vez que se adivinó, se perdió una hora.
- **Leer el framebuffer desde el firmware** cuando algo "no se ve": contar
  píxeles distintos del fondo en una región separa lo que dibuja la Pico de lo
  que muestra el monitor y de lo que capta la cámara. Resolvió en un minuto dos
  cosas que por foto eran indistinguibles.
- **Instrumentar el tiempo por vista y por banda** cuando algo titila. El
  presupuesto es **15200 µs**: lo que tarda el haz en bajar la pantalla.
- Cuando algo no se entiende, **hacer un diagrama visual publicado como
  artifact**, no una tabla. Los pidió explícitamente.

---

## 8. Lo que hay que hacer ahora

Esta es la lista viva de Valentino. Lo de arriba es lo urgente.

### Mapas (dijo "urgente, sí o sí")

1. **El mapa de Ezeiza a Miami quedó muy raro.** Hay que mirarlo y arreglarlo.
2. **Layout adaptativo** — *empezado, sin verificar en el monitor*: para rutas
   anchas (Sydney-Buenos Aires) el mapa va arriba y la tarjeta abajo; para rutas
   norte-sur (Ezeiza-Madrid, Ezeiza-Miami) el mapa a la izquierda y la tarjeta
   al costado. El código está en `radar_pintar_viaje()`, decide por la forma de
   la ruta. **Falta ver cómo queda.**
3. **Los puntos que sigue el vuelo** — *hecho, sin verificar*: en cinco lugares
   de la ruta se marca el aeropuerto más cercano, como hace la web. Se calcula
   sólo cuando cambia el vuelo (recorrer 5334 aeropuertos no es para cada
   cuadro).

### Radar

4. **Franjas negras en el radar.** Valentino las ve en el de 80 km. **Todavía no
   se pudo reproducir** — es lo que falta investigar.
5. **La aproximación no coincide con el rumbo magnético.** Ojo con lo que dice
   la sección 4 sobre el estiramiento del monitor: parte de lo que se ve mal es
   eso, no el dibujo.
6. **El cartel dice sólo el número de cabecera** ("11 EN USO") — *hecho*.

### La casa

7. **El rumbo del avión sobre la casa cambiaba todo el tiempo** — *arreglado*:
   el vuelo de prueba ahora pasa recto con rumbo fijo, como uno de verdad.
8. **El círculo alrededor del avión no le gusta** — *cambiado*: ahora se marca
   con color (ámbar) en vez de encerrarlo.
9. **Falta**: que el cliente pueda fijar la ubicación de la casa desde la web.
   Va junto con el portal de configuración.

### Colores de puntualidad

10. **Hecho, sin verificar**: la barra mantiene el color del tema durante casi
    todo el vuelo. Sólo cerca de llegar cambia — verde a tiempo, rojo atrasado,
    **azul adelantado**. En vuelos de 2 h o más, en la última hora; en los
    cortos, en los últimos 30 minutos.

### Formato aeropuerto

11. **Hecho, sin verificar**: acrónimos (`A HORA`, `ADEL 10`, `DEM 25`) y una
    columna nueva con la **hora de llegada actualizada** (la planificada corrida
    por la demora), además de la planificada.

### Pendiente de revisar a ojo

12. Los logos uno por uno en la pantalla de logos (escena 16).

---

## 9. Cambios sin commitear

Estos archivos tienen trabajo hecho **que no se pudo probar en la placa**,
porque la Pico desapareció del USB justo antes:

```
firmware-c/demo.c  main.c  radar.c  radar.h  tarjetas.c  viaje.c
```

Contienen: el layout adaptativo del mapa, los puntos intermedios de la ruta, los
colores de puntualidad cerca de la llegada, los acrónimos y la hora actualizada
del FIDS, el vuelo de prueba que pasa recto sobre la casa, y el marcado por
color en vez de círculo.

**Compilan sin errores.** Lo primero que hay que hacer es cargarlos y mirarlos.

---

## 10. Prompt para la sesión nueva

> Estoy siguiendo el proyecto Pico VGA Radar, en `~/Desktop/Proyectos/PicoVga`.
> Leé `TRASPASO.md` completo antes de hacer nada: tiene todo el contexto, el
> estado, el cableado, cómo compilar y cargar, cómo mirar el monitor con la
> GoPro, los bugs ya resueltos que no hay que repetir, y la lista de lo que
> falta.
>
> El firmware en C ya hace el radar completo a 60 fps: scope con barrido,
> pistas y aproximación, tarjetas de vuelo, formato aeropuerto, mapa mundial
> con costas y fronteras, seguimiento de vuelos, temas y la marca de "sobre mi
> casa". Los datos (logos, pistas, aeropuertos, costas) están todos en la flash.
>
> Hay cambios sin commitear que compilan pero **no se probaron en la placa**
> porque la Pico se desconectó del USB: están listados en la sección 9.
> Empezá por cargarlos con `./herramientas/cargar.sh` y revisarlos en el
> monitor con la GoPro, y después seguí con la lista de la sección 8, que
> arranca por los mapas.
>
> Tenés permiso para usar la GoPro como ojos y no pedirme fotos. Para cargar
> firmware usá el script, no me pidas apretar BOOTSEL. Si la placa no aparece
> ni como puerto ni como disco, avisame: eso sí necesita que la desenchufe.
