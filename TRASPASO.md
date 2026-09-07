# Traspaso — Pico VGA Radar

Documento para retomar el proyecto en una sesión nueva, **sin contexto previo**.
Escrito el **5 de septiembre de 2026**, al final de una sesión larga.

Repo: `~/Desktop/Proyectos/PicoVga` → https://github.com/00tino/pico-vga-radar
Rama `main`. Todo commiteado, nada pusheado (ver sección 9).

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

### La réplica: la web dibuja con el firmware

`web-firmware/` compila **el mismo código C que corre en la placa** a
WebAssembly, y `docs/replica.html` le pide los cuadros y los pinta en un
canvas. La vista previa de la configuración es eso, no una imitación: la
fuente, las esquinas rectas, los logos, los colores y el reparto de la
pantalla salen del firmware. Si mañana cambia cómo se dibuja una tarjeta, la
web cambia sola y no se pueden desincronizar.

```
./web-firmware/compilar.sh     # deja docs/radar-firmware.js y .wasm (1,5 MB)
```

Lo único que se reemplaza es `vga.c`, que en la placa maneja el PIO y el DMA
y en la web es sólo el framebuffer (`web-firmware/vga_web.c`). El resto —gfx,
radar, tarjetas, viaje, pantallas, las fuentes y todos los datos— es el mismo
archivo.

**Emscripten en esta Mac** (se instaló con `brew install emscripten`):
- Pide **Python 3.10 o más** y el del sistema es 3.9, así que hay que exportar
  `EMSDK_PYTHON=/opt/homebrew/bin/python3.12`. Ya lo hace `compilar.sh`.
- Su config vive en el `.emscripten` del Cellar y **hay que escribirla a
  mano**: el `--generate-config` apunta a `/usr/bin`, que no tiene los targets
  wasm. Va el LLVM propio de emscripten, `binaryen` (que se instala aparte) y
  el node de brew.

**Lo que falta**: el firmware está atado a 640×480 y a los 256 colores
(`VGA_ANCHO`, el tamaño de las fuentes, los logos a 36×36). Para el equipo
premium en Full HD hay que hacer eso parametrizable; los generadores de
fuentes y logos ya existen, así que es ordenado.

### La página de configuración

`docs/setup.html`. El cliente arma ahí la **lista de pantallas** (agregar,
quitar, ordenar), y cada una lleva su vista, aeropuerto, alcance, formato de
lista, cuántos vuelos y cuántos segundos dura. Arriba van las cosas generales:
wifi, estilo, la casa y los filtros.

Detalles que costaron y conviene no deshacer:

- **La vista previa va por la URL del simulador, no por `postMessage`.** El
  `postMessage` que había antes no lo escuchaba nadie: `radar.js` sólo lee
  query params, así que la previa nunca cambiaba.
- **El aviso de "menos de 15 segundos" se prende y apaga sin rehacer la
  lista.** Rehacerla en cada tecla le saca el foco al campo que se está
  escribiendo.
- **La web ofrece las seis vistas del equipo**, pero el simulador sólo dibuja
  tres: para las de seguimiento y la de logos, la previa lo dice en vez de
  mostrar cualquier cosa.
- Las duraciones se acotan a 15–120 en la web **y** en el firmware, para que
  lo que se ve sea lo que va a hacer la Pico.
- **La vista previa es el equipo, no el navegador.** Con `real=640x480` el
  simulador dibuja al tamaño exacto de la pantalla, baja los colores a los 256
  del VGA (los mismos 3-3-2 de `vga_rgb`, redondeando) y arma el bloque entero
  a esa medida para agrandarlo después: así el texto de las tarjetas tiene el
  tamaño que va a tener en el monitor. Con `embed=1` se esconde toda la
  interfaz del simulador. **Los dos parámetros existían en la URL desde antes
  pero no los implementaba nadie**, igual que el `postMessage`.
  El equipo lo elige el instalador (`?equipo=premium`), nunca el cliente.
  **Lo que todavía no se replica**: las tarjetas son HTML, así que su texto se
  ve más limpio de lo que se verá en el monitor. Para que sea idéntico habría
  que dibujarlas también en el canvas.
- **La previa se carga UNA vez y los cambios se le mandan por `postMessage`.**
  Pasarle la configuración por la URL la hacía recargar en cada tecla, o sea
  volver a bajar 1,5 MB de firmware: en el celular la página se trababa y
  parecía que no dejaba editar nada.
- **El buscador de aeropuertos** usa `docs/airports.json`, los mismos 5334 que
  tiene la Pico en la flash: escribiendo "syd" o "sydney" aparece Sydney.
- El plano de cableado en PDF sale de `herramientas/cableado_a_pdf.py`, que
  escribe el PDF a mano (sin librerías) y queda vectorial. **Si se cambia un
  cable hay que cambiarlo ahí y en este documento.**
- Todavía **guarda en el navegador** (`localStorage`). Cuando esté el portal
  tiene que guardar en la Pico: es la única forma de que la configuración
  aparezca igual desde cualquier celular.

Lo que falta: el estilo es uno solo para todas las pantallas (el firmware lo
soporta por pantalla), y no hay separación entre lo del instalador y lo del
cliente.

### Las pantallas del cliente

El equipo rota entre una lista de **pantallas** que arma el cliente, cada una
con su propia duración: por ejemplo los vuelos de Ezeiza cinco segundos y un
vuelo seguido otros cinco. Está en `pantallas.c`, y por ahora la lista se arma
en `pantallas_de_ejemplo()` de `main.c`; cuando esté el portal va a llegar de
la web y a guardarse en la flash.

**Lo que hace que esto se sienta bien es que la lista no vuelve a empezar.**
Con veinte vuelos y cuatro por pantalla, la primera visita muestra del 1 al 4,
la siguiente del 5 al 8, y así hasta completarlos. Cada pantalla guarda su
lugar en el carrusel y ese lugar **sólo avanza mientras esa pantalla está a la
vista**. Verificado por consola: la pantalla de Ezeiza recorre los grupos
2, 3, 0, 1, 2, 3 sin repetir.

Cada cambio de pantalla limpia la imagen entera y eso se come un cuadro, o sea
un parpadeo. Por eso la duración se acota entre **15 y 120 segundos**
(`PANTALLA_SEGUNDOS_MIN` y `MAX` en `pantallas.h`): a los quince no se nota, a
los cinco sí. Es un límite del equipo, no algo que el cliente pueda saltear
desde la web.

Ojo con `radar_carrusel_poner()`: **no puede poner `lista_n` en cero**. Eso
entra por la rama que rearma la lista, que resetea el reloj y no incrementa la
página, y entonces cada visita muestra los mismos cuatro vuelos.

El equipo **arranca directo en el radar**: la pantalla de primitivas ya no sale
al encender, queda como herramienta detrás del comando `d`. Valentino quiere
poner ahí un logo propio; todavía no está hecho.

El recorrido de 18 escenas de prueba se sacó de `main.c` al entrar el motor de
pantallas; está en el historial de git si hace falta volver a mirarlo.

### Lo que falta del proyecto

WiFi, portal de configuración, QR y datos ADS-B reales. **Nada de eso está
empezado.** El tráfico es simulado.

---

## 3. Cómo trabajar

### Armar un equipo con otro monitor

**Todo lo que se toca está en `firmware-c/instalacion.h`**: el nombre del
monitor, los cuatro márgenes y los valores de fábrica de la casa. Si hay que
cambiar algo del monitor y no está ahí, está mal puesto.

1. Cargar el firmware y mandar `c` por la consola: sale la pantalla de
   calibración, con reglas numeradas contra los cuatro bordes (otra `c` la
   saca).
2. **Mirar el monitor de verdad**, con la GoPro. `mirar.py` no sirve acá: el
   framebuffer siempre está entero, el que recorta es el monitor.
3. Leer, en cada borde, desde qué número se empieza a ver la regla. Ese número
   es el margen de ese borde.
4. Cargarlos en `instalacion.h`, recompilar y cargar.

El firmware sale **640×480 fijo**. Un monitor de otra resolución nativa la
escala y se ve igual, sólo más blando. **Cambiar de resolución de verdad es
otro trabajo**: hay que rehacer los timings del PIO y el reloj, y los 100,8 MHz
están elegidos para que los tres divisores den enteros (bug 6).

### Compilar y cargar

```
./herramientas/cargar.sh
```

Compila y carga sin tocar el botón BOOTSEL (que está tapado por la protoboard).
Va por **picotool** (`brew install picotool`), que habla PICOBOOT por USB:
reinicia la placa solo y escribe la flash directo.

**El disco `NO NAME` ya no se usa y no hay que usarlo.** macOS a veces no lo
monta aunque la placa esté perfecta: aparece como `RP2350 Boot` en
`system_profiler SPUSBDataType` y `/Volumes` está vacío. Por ese camino no
había forma de cargar; por PICOBOOT eso no importa. `picotool info -f` sirve
para ver si la placa responde.

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

### Ver lo que dibuja la Pico, sin cámara

```
python3 herramientas/mirar.py vista.png -e 3    # la escena 3
```

Pide el framebuffer por el puerto serie y lo guarda como PNG exacto, píxel por
píxel. **Es el primer recurso**, no la cámara: no tiene el estiramiento del
monitor, ni la exposición de la GoPro, ni el negro levantado, y deja medir
colores y ángulos con un script.

El firmware lo atiende en `volcado.c`; el bucle de `main.c` lee comandos por
consola: `v` vuelca, `n` salta a la escena siguiente, `p` reinicia el reloj de
la escena, `c` muestra la pantalla de calibración y `d` el patrón de
primitivas (las dos se salen con la misma tecla). `mirar.py -e N` salta hasta la escena N y la congela antes de
volcar. **Hay que vaciar el buffer del puerto antes de buscar la línea
`ESCENA`**: si no, la línea de la escena anterior da un falso positivo y se
vuelca una vista que no es la pedida.

### Ver el monitor con la GoPro

Sirve para ver lo que el monitor hace con la señal. Está autorizado usar la
**GoPro HERO12** como ojos:

1. Abrir QuickTime Player → Archivo → **Nueva grabación de vídeo**. Toma la
   HERO12 sola. **No** hay que apretar grabar.
2. `perl -e 'alarm 20; exec @ARGV' screencapture -x -l <id_ventana> foto.png`
3. `./herramientas/bajar_brillo.sh foto.png vista.png` y leer `vista.png`.

**Dos capturas seguidas**: macOS sólo redibuja la ventana cuando algo la toca,
así que la primera devuelve el cuadro anterior. La buena es la segunda.

El id de ventana se saca con las herramientas de control de aplicaciones
(`app_list_windows` sobre `com.apple.QuickTimePlayerX`). En esta sesión fue
`11641`, pero **cambia cada vez que se reabre la grabación**.

**Si el Space actual es una app en pantalla completa, esto no anda**: la
ventana de la grabación queda en otro Space, macOS no la redibuja y las
capturas salen todas negras. No se puede arreglar por software (no se puede
mover una ventana a un Space de pantalla completa): hay que pedirle a
Valentino que pase a un escritorio normal. Mientras tanto, `mirar.py`.

Por qué así y no de otra forma:
- **Photo Booth no sirve**: espeja la imagen y se pausa cuando no está al
  frente, devolviendo un cuadro viejo.
- **ffmpeg desde la terminal se cuelga**: macOS no le da permiso de cámara.
- El brillo: la GoPro expone para el cuarto oscuro y quema la pantalla.
  `bajar_brillo.sh` recupera el detalle. Fue idea de Valentino.

### Regenerar los datos

```
python3 herramientas/fuente_grande_a_c.py  # las fuentes de escala 2 y 3
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
| `GP10` | DDC datos (SDA) | 1,95 kΩ | 12 |
| `GP11` | DDC reloj (SCL) | 1,95 kΩ | 15 |
| `GND` | masa | directo | **5, 6, 7, 8 y 10** |

Los dos del DDC son de un experimento que **no funcionó** (ver más abajo). No
molestan y sirven si algún día se prueba otro monitor. **El agujero 9 (+5V)
queda sin conectar**; las resistencias en serie no son opcionales, porque ese
bus trabaja a 5 V y los pines de la Pico no toleran 5.

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

### El EDID: la placa sabe qué monitor tiene

Con los agujeros 12 y 15 cableados, la Pico lee el EDID y dice:
`VSC VA1703wSERIES (codigo 121F), preferida 1440x900`.

**Corrección de una conclusión anterior**: se había dado por muerto porque un
barrido de direcciones I2C no encontraba nada. El barrido era un mal test: esa
memoria no contesta a una lectura suelta, hay que escribirle el puntero primero
(`write(0)` y después `read(128)`), que es lo que hace `leer_edid()`.

Sirve para reconocer un monitor nuevo solo, y confirma que este es panorámico
(1440x900 es 16:10), que es por qué estira el 4:3.

### El botón del monitor: probado y descartado

Valentino quería apretar un botón del monitor para que apareciera el QR de
configuración. **Los botones del menú (brillo, fuente, OSD) son imposibles**:
el VGA lleva video en un solo sentido y esa información no sale del monitor.

Quedaba una vía para el **botón de encendido**: los agujeros 12 y 15 son un bus
I2C (DDC) con las resistencias de pull-up adentro del monitor. La idea era que
al apagarlo se cayeran y la placa lo notara. Se cableó y se midió:

- Las dos líneas suben solas en 2-3 µs: el bus está bien y el monitor las
  levanta.
- El EDID **sí** contesta (ver arriba); lo que fallaba era el barrido.
- **Apagando y prendiendo el monitor varias veces, la placa no vio ni un
  cambio**: las pull-ups del ViewSonic siguen alimentadas en standby.

O sea: **con este monitor no hay ningún botón que la placa pueda ver.** El
código quedó en `monitor.c` con los comandos `m` (leer EDID), `M`
(diagnóstico del bus, con tiempos de subida) y `W` (vigilar el encendido), por
si otro modelo se porta distinto.

Lo que sí funciona y falta hacer: **contar arranques cortos en la flash**. El
cliente prende y apaga el equipo tres veces seguidas y sale el QR. No necesita
hardware y no se hace sin querer.

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
10b. **Lo que va arriba hay que dibujarlo cuando el haz esta abajo.** Las
    tarjetas y la tabla del formato aeropuerto estan en la mitad de arriba, y
    se dibujaban adentro del bucle de bandas, o sea justo cuando el haz venia
    llegando ahi: las alcanzaba por hasta 2 ms y esas franjas salian rotas y
    titilando. **Esas eran las "franjas negras" que Valentino veia.** Ahora se
    dibujan despues del bucle, con el haz abajo del todo: lo que se escribe se
    ve entero en el cuadro siguiente.

10c. **El encabezado no se redibuja si no cambio.** Borrarlo y rehacerlo cada
    cuadro era casi todo lo que costaban las dos bandas de arriba, que son las
    que menos margen tienen.

10d. **Medir el margen contra el haz, no el tiempo por banda.** Comparar cada
    banda contra 1520 us no dice nada: las bandas son de distinto alto y el
    dibujo arranca con la ventaja del borrado vertical. Lo que importa es
    cuanto falta para que el haz llegue al final de la banda en el momento en
    que se termino de dibujar. El firmware lo imprime por escena; en negativo
    esa franja se rompe.

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

18b. **La letra grande no se agranda: se rasteriza aparte.** A 7x14 cada
    pixel es un rasgo entero de la letra, y multiplicarlo por dos o por tres
    solo agranda los escalones — se probó rellenar los rincones al estilo
    Scale2x y seguía viéndose mal. Las escalas 2 y 3 tienen su **propia
    fuente rasterizada** de Inconsolata, generada por
    `herramientas/fuente_grande_a_c.py` (`fuente_grande.c`, 17 KB de flash).
    El script se alinea contra la fuente chica midiendo dónde cae la 'A', y
    el avance de cada carácter es el de la chica por la escala: así ningún
    texto se corre de lugar. **`gfx_ancho_texto` tiene que leer la misma
    tabla que el dibujo**, o los centrados y las columnas se desalinean.
    El camino de la letra chica tiene que seguir salteando las columnas
    vacías: sin eso, el texto chico (que es casi todo lo que se dibuja) se
    encarece y el haz vuelve a alcanzar al dibujo.

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

26b. **Las longitudes de un contorno hay que envolverlas en cadena**, cada
    punto respecto del anterior y sólo el primero respecto del centro del
    mapa. Envolviéndolas sueltas contra un ancla, un contorno que cruza el
    antimeridiano relativo queda partido en dos mitades separadas por media
    vuelta: sus lados atraviesan el mapa entero, el relleno sale **en franjas
    a lo ancho** y aparecen continentes donde no van. Vale igual para las
    fronteras. Era lo que hacía "raro" al mapa de Ezeiza a Miami.

26c. **El recorte de `gfx_banda` era sólo vertical.** Con la tarjeta al
    costado del mapa, las fronteras y la ruta se dibujaban encima de ella.
    Ahora hay `gfx_banda_ancho(x0, x1)` y el mapa lo usa.

### Geometría del radar

26. **La proyección del scope corrige por el coseno de la latitud.** Sin eso los
    rumbos se ven torcidos: la aproximación a la 11 de Ezeiza parecía la 09.
    (La web tiene este defecto; acá está corregido.)
27. **La senda de aproximación se dibuja extendiendo el eje de la pista tal
    como quedó en pantalla**, no proyectando por lat/lon.
28. **El avión viene por detrás de la cabecera donde aterriza**, no por
    delante. Con el signo al revés la senda salía para el lado de adentro.

28b. **La dirección de la senda se sacaba de las puntas ya redondeadas a
    píxel.** A 80 km de alcance la pista mide 13 px, así que un píxel de
    error torcía la senda hasta 4°, y el rumbo cambiaba según el alcance.
    Ahora se calcula en subpíxeles (×256) y da **102° en todos los
    alcances**: el rumbo verdadero de la 11 de Ezeiza.

29b. **El número de la cabecera es magnético; el mapa es verdadero.** En
    Ezeiza la declinación es de unos 8° oeste, así que la "11" (110°
    magnéticos) apunta a 102° verdaderos y la senda sale hacia 282°. Esa
    diferencia es correcta y no hay que "arreglarla". El firmware la imprime
    por consola en cada escena.

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

18b. **La letra grande no se agranda: se rasteriza aparte.** A 7x14 cada
    pixel es un rasgo entero de la letra, y multiplicarlo por dos o por tres
    solo agranda los escalones — se probó rellenar los rincones al estilo
    Scale2x y seguía viéndose mal. Las escalas 2 y 3 tienen su **propia
    fuente rasterizada** de Inconsolata, generada por
    `herramientas/fuente_grande_a_c.py` (`fuente_grande.c`, 17 KB de flash).
    El script se alinea contra la fuente chica midiendo dónde cae la 'A', y
    el avance de cada carácter es el de la chica por la escala: así ningún
    texto se corre de lugar. **`gfx_ancho_texto` tiene que leer la misma
    tabla que el dibujo**, o los centrados y las columnas se desalinean.
    El camino de la letra chica tiene que seguir salteando las columnas
    vacías: sin eso, el texto chico (que es casi todo lo que se dibuja) se
    encarece y el haz vuelve a alcanzar al dibujo.

### Mapas (dijo "urgente, sí o sí")

1. **El mapa de Ezeiza a Miami quedó muy raro** — *arreglado y verificado*:
   eran las longitudes envueltas punto por punto (bug 26b). Faltaban México,
   Centroamérica y el Caribe, y había franjas a lo ancho. Ahora se ve entero.
1c. **La ruta del mapa no pasaba por los puntos que ella misma marca** —
   *arreglado*: iba derecho del origen al destino, así que de Miami a Ezeiza
   pasaba lejos de API y de TRQ. Ahora va de tramo en tramo por los puntos de
   paso, cada tramo por su propio círculo máximo. Medido: la ruta pasa a 4 px
   o menos de cada marca, contra 12 antes. Y se le subió el contraste: con
   3-3-2 bits el relleno de tierra y el tono de la ruta caían a dos escalones
   y la línea se perdía encima del continente.

1b. **La tarjeta del mapa quedaba medio vacía** — *arreglado*: la tarjeta
   admite hasta siete filas en vez de cinco, y las dos nuevas son el tiempo
   volando con el avance y la duración, y los aeropuertos por los que va
   pasando (los mismos que marca el mapa). El sobrante se reparte entre
   `filas` huecos y no entre `filas - 1`: así el estirado llega hasta abajo y
   queda un margen al pie, en vez de la última fila pegada al borde.

2. **Layout adaptativo** — *hecho y verificado en los dos casos*: para rutas
   anchas (Sydney-Buenos Aires) el mapa va arriba y la tarjeta abajo; para rutas
   norte-sur (Ezeiza-Madrid, Ezeiza-Miami) el mapa a la izquierda y la tarjeta
   al costado. El código está en `radar_pintar_viaje()`, decide por la forma de
   la ruta. Hubo que agregar el recorte lateral (bug 26c): las fronteras se
   metían adentro de la tarjeta. La escena 11 es la que muestra el caso
   norte-sur.
3. **Los puntos que sigue el vuelo** — *hecho y verificado* (API, TRQ, CBB,
   SDE en el mapa a Miami): en cinco lugares
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

6b. **El cartel de "sobre mi casa" no aparecía nunca** — *arreglado*: la
    variable `casa_encima` se leía en tres lugares y **no se le asignaba en
    ninguno**. Ahora se calcula en `radar_avanzar()`.

6c. **Fuera el círculo del radio de la casa y el rótulo "CASA"** — *hecho*:
    queda sólo la casita. El círculo ensuciaba el scope.

6d. **Fuera el círculo alrededor del avión en el mapa** — *hecho*: llegando al
    destino se juntaba con los dos anillos del aeropuerto y se veían tres
    círculos y un triángulo amontonados. El rótulo del avión ahora va debajo y
    el del aeropuerto arriba.

7. **El rumbo del avión sobre la casa cambiaba todo el tiempo** — *arreglado*:
   el vuelo de prueba ahora pasa recto con rumbo fijo, como uno de verdad.
8. **El círculo alrededor del avión no le gusta** — *cambiado*: ahora se marca
   con color (ámbar) en vez de encerrarlo.
9. **Falta**: que el cliente pueda fijar la ubicación de la casa desde la web.
   Va junto con el portal de configuración.

### Colores de puntualidad

9b. **Los temas verde, rojo y azul chocaban con los colores de estado** —
    *arreglado en los dos lados*. Con 3-3-2 bits `crt_green` caía casi en el
    mismo tono que el verde de "a tiempo", `red` en el del "demorado" y `navy`
    en el del "adelantado", así que el estado no cantaba. Se corrieron los
    tres temas (no los colores de estado) en `docs/radar.js` **y** en
    `radar.c` con los mismos valores, para que la web y la Pico sigan
    mostrando lo mismo. **Al tocar un tema hay que tocar los dos archivos.**
    `atc_dark` también queda cerca del azul de "adelantado": si molesta, es el
    próximo a correr.

10. **Hecho y verificado**: la barra mantiene el color del tema durante casi
    todo el vuelo. Sólo cerca de llegar cambia — verde a tiempo, rojo atrasado,
    **azul adelantado**. En vuelos de 2 h o más, en la última hora; en los
    cortos, en los últimos 30 minutos.

### Formato aeropuerto

11. **Hecho y verificado** (escena 7, la de ancho completo; en la columna
    angosta esas columnas no entran, igual que `compact` en la web):
    acrónimos (`A HORA`, `ADEL 10`, `DEM 25`) y una
    columna nueva con la **hora de llegada actualizada** (la planificada corrida
    por la demora), además de la planificada.

### Pendiente de revisar a ojo

12. Los logos uno por uno en la pantalla de logos (ahora escena 17).
    *Revisado*: los que hay se ven bien. Pero **84 de los 824 PNG de
    `docs/logos/` son el mismo archivo**, un placeholder a cuadros, y por eso
    en la página 1 casi todas las casillas son un damero. Es dato de origen,
    no firmware: la web muestra exactamente lo mismo. Decidir qué hacer es de
    Valentino — se podrían volver a bajar, o dejar uno solo en la flash y
    ahorrar los 109 KB que hoy se gastan repitiéndolo 84 veces.

---

## 9. Estado del árbol

Todo commiteado, **nada pusheado**. El trabajo que había quedado sin probar
(layout adaptativo, puntos intermedios, colores de puntualidad, acrónimos del
FIDS, el vuelo recto sobre la casa) ya está cargado y verificado en la placa,
escena por escena, con `mirar.py`.

`main.c` recorre ahora **18 escenas**: se agregó la 11, "seguir AA954 con
tarjeta", que es la única que muestra el layout norte-sur.

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
> Está todo commiteado y nada pusheado. Para ver lo que dibuja la Pico usá
> `python3 herramientas/mirar.py vista.png -e N`, que trae el framebuffer por
> el puerto serie: es más fiel que la cámara. La GoPro queda para ver qué hace
> el monitor con la señal. Para cargar firmware usá `./herramientas/cargar.sh`,
> no me pidas apretar BOOTSEL. Si `picotool info -f` no encuentra la placa,
> avisame: eso sí necesita que la desenchufe.
