# Traspaso — Pico VGA Radar

Documento para retomar el proyecto en una sesión nueva, sin contexto previo.
Escrito al final de la sesión del **4 de septiembre de 2026**.

Repo: `~/Desktop/Proyectos/PicoVga` → https://github.com/00tino/pico-vga-radar
Todo lo de esta sesión está commiteado y pusheado. Rama `main`, limpia.

---

## 1. Qué es el proyecto

Un **radar de tráfico aéreo en un monitor VGA**, impulsado por una Raspberry Pi
Pico 2 W. Valentino lo va a **vender armado**, así que hay dos roles distintos:

- **Él es el instalador**: define pulgadas y resolución del monitor de cada
  equipo. Eso queda grabado en la placa y el cliente no lo ve.
- **El cliente** recibe el equipo, lo enchufa, aparece un QR en el monitor, lo
  escanea con el celular y elige aeropuerto, vista, filtros y estilo.

Tres partes:

| Parte | Dónde | Qué hace |
|---|---|---|
| **Web** | `docs/` → GitHub Pages | Simulador del radar y página de configuración |
| **Proxy** | `sky-proxy/` → Vercel | Sirve datos ADS-B ya masticados |
| **Firmware** | `pico/` y `firmware-c/` | Lo que corre en la placa |

---

## 2. Estado actual

### La web — terminada y publicada

https://00tino.github.io/pico-vga-radar/sim.html

Se arreglaron los 5 defectos que Valentino reportó (lista de aeropuerto
centrada, tarjetas sin recorte, aviones sin indicativo, pistas a escala,
aproximación visible) y se agregó el enlace con el equipo: cuando se entra con
`?pico=<ip>` aparece el botón **"Guardar en el radar"** y se oculta la pestaña
Instalador.

### Firmware MicroPython — funciona de punta a punta

`pico/`. **Ya no está cargado en la placa** (lo pisó el firmware en C), pero
está completo y probado. Hace todo el ciclo: arranca solo, se conecta al WiFi,
levanta punto de acceso si no hay red, muestra el QR, sirve el portal, recibe la
configuración del celular, pide datos ADS-B y dibuja el radar con vuelos reales.

Su límite: **8 colores y dibujado lento**. No sirve para lo que Valentino quiere.

### Firmware C — la base de video funcionando

`firmware-c/`. **Es lo que está cargado en la placa ahora.** Da **640×480 con
256 colores**, verificado en el monitor: 8 tonos de rojo, 8 de verde, 4 de azul,
imagen estable, sin rayado. Framebuffer de 307 KB en RAM.

Sólo tiene el driver de video y un patrón de prueba. **Todo lo demás está por
hacerse.**

---

## 3. El requisito que manda

Valentino lo dijo textual y es innegociable:

> **"Lo que se ve en la web se tiene que ver en la Pico."**

No acepta una versión reducida en el monitor. Eso implica portar a C: tarjetas
con logo de aerolínea, ruta origen-destino, horarios, barra de progreso, pistas,
senda de aproximación, temas, y el barrido girando fluido. Más los 256 colores
para que los logos se vean bien.

Por eso se abandonó MicroPython. **No hay que volver atrás.**

---

## 4. Hardware

### Cableado (verificado y funcionando)

La Pico está montada en protoboard con la serigrafía visible. **Regla que costó
horas: leer siempre la etiqueta impresa (`GP0`, `GP1`…), nunca contar pines.**
El pin físico 21 se llama `GP16`; `GP21` es el pin físico 27.

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

Las de 500 Ω son dos de 1 kΩ en paralelo; las de 1,95 kΩ son dos de 3,9 kΩ en
paralelo. Proporción 1:2:4 por canal.

**Las cinco masas del VGA son obligatorias.** Con sólo la del agujero 6 el
monitor perdía el enganche cada pocos segundos. Salen todas del riel `−`.

Del otro lado de la Pico no sale ningún cable. Ahí están `VBUS`, `VSYS`, `3V3` y
`3V3_EN`, que apagan la placa si se tocan.

### Monitor de prueba

**ViewSonic VA1703wb**, pantalla ancha 1440×900. Dos cosas a tener presentes:

- **Estira el 4:3.** Los círculos salen ovalados salvo que se ajuste la relación
  de aspecto en el menú del monitor.
- **Recorta unas 7 filas de abajo.** En MicroPython eso se compensaba con
  `margen` en `install.json`. En C todavía no está implementado.

---

## 5. Cómo trabajar

### Compilar el firmware en C

```
cd firmware-c && ./compilar.sh
```

Genera `build/radar.uf2`. El script contempla dos particularidades de esta Mac,
explicadas en sus comentarios:

- El **Command Line Tools está roto**: `usr/include/c++/v1` tiene 1 archivo en
  vez de 185. Se apunta a la copia buena del SDK de macOS con
  `CPLUS_INCLUDE_PATH`.
- Esa variable **no puede estar activa al compilar para ARM** (el compilador
  cruzado agarra los headers de macOS y explota). Por eso el build va en dos
  fases: primero las herramientas del host, después el firmware.

Herramientas instaladas: `cmake` por brew, SDK en `~/pico-sdk`, toolchain ARM
14.3 en `~/arm-toolchain`. **El `arm-none-eabi-gcc` de brew NO sirve**: viene sin
newlib y no enlaza.

### Cargar firmware sin tocar la placa

El botón BOOTSEL está tapado por la protoboard. Se entra por software abriendo
el puerto serie a **1200 baudios**:

```python
import sys, time, glob
sys.path.insert(0, "/Users/valentino/Library/Python/3.9/lib/python/site-packages")
import serial
p = glob.glob("/dev/cu.usbmodem*")
s = serial.Serial(p[0], 1200); s.dtr = False; time.sleep(0.4); s.close()
```

Después aparece `/Volumes/NO NAME` y se copia el `.uf2` ahí.

**REGLA CRÍTICA: todo firmware nuevo debe llevar `pico_enable_stdio_usb`.** Sin
eso la placa queda invisible, el truco de los 1200 baudios no funciona, y hay
que sacar la Pico de la protoboard para apretar el botón. Ya pasó una vez.

El `picotool` que compila el SDK **no tiene soporte USB**, así que
`picotool reboot` no sirve.

### Leer la consola del firmware

```
cat /dev/cu.usbmodem11201
```

Conviene envolverlo en `perl -e 'alarm 15; exec @ARGV' cat ...` porque no
termina solo.

### Volver a MicroPython

Bajar el `.uf2` de https://micropython.org/download/RPI_PICO2_W/ (el ARM, **no**
el RISC-V), copiarlo en modo BOOTSEL, y después `./pico/cargar.sh`, que usa
`mpremote` (instalado en `~/Library/Python/3.9/bin/mpremote`).

### Herramienta útil

`pico/test_cruces.py` — detecta cables cruzados entre pines sin tester. Pone
cada pin en alto y lee los otros. Encontró un cruce `GP2`-`GP3` en 30 segundos
después de un rato largo de mirar barras de colores tratando de adivinarlo.

---

## 6. Bugs resueltos — no repetirlos

### Hardware

1. **Los dos cables micro-USB eran de sólo carga.** La Pico no aparecía en el bus
   USB. Costó media sesión. El LED apagado **no** es síntoma de nada: en la Pico
   2 W el LED cuelga del chip de WiFi y una placa virgen lo tiene siempre
   apagado.
2. **La placa estaba montada al revés** y la numeración salía espejada. Los
   cables terminaban en `VBUS` y `3V3_EN`, que apagaban el regulador.
3. **Los cables estaban en pines muertos** por confundir "pin físico 21" con
   `GP21`. Se detectó midiendo: GP0-4/GP16/GP17 en 0,00 %, GP18-22 activos.
4. **Faltaban las masas del sincronismo** (agujeros 5 y 10 del VGA).
5. **Cruce físico entre `GP2` y `GP3`** en la protoboard.

### Software — MicroPython

6. **El video y el WiFi se peleaban por PIO0.** Con `PIO_UNIT = 0` la placa se
   cuelga en `VGA_init()`. Se pasó a PIO1.
7. **Bug en la librería de video**: `draw_fastVline` con x=0,y=0 escribía ~47000
   palabras fuera del framebuffer y corrompía la memoria contigua (se rompía
   `self.font` y fallaba todo el texto). Corregido con wrap del índice.
8. **Los objetos DMA eran variables locales** y el recolector de basura los
   destruía, abortando los canales. El video moría en el primer `gc.collect()`:
   sincronismo sí, imagen no. Se guardan en `self.dma0` / `self.dma1`.
9. **MicroPython no tiene `ljust` ni `rjust`.** Usar anchos en el formato.
10. **Redibujar toda la pantalla en cada vuelta** hacía que parpadeara. Sólo se
    redibuja cuando cambian los datos.

### Software — C

11. **252 MHz de reloj deja la placa muerta al arrancar.** Con **100,8 MHz** los
    tres divisores dan enteros (hsync/vsync 6,3 MHz, píxeles 50,4 MHz) y no hace
    falta sobrefrecuencia.
12. **Los tres programas de PIO sumaban 33 instrucciones y sólo hay 32 lugares.**
    Se pisaban entre sí. Ahora son 9 + 15 + 7 = 31. **Queda 1 lugar libre**: si
    hace falta más, hay que compactar.
13. **El DMA debe transferir de a 32 bits** (4 píxeles por palabra), no de a
    byte. Con bytes, tres de cada cuatro píxeles salían basura.
14. **Hay que reiniciar el DMA en cada cuadro** desde una interrupción de vsync.
    Sin eso la imagen se desplaza sola y parece un estroboscopio.

---

## 7. Lo que falta, en orden

1. **Primitivas de dibujo en C**: texto con fuentes, líneas, círculos,
   rectángulos, blits. Base de todo lo demás.
2. **Área útil configurable** (`install.json`), como ya estaba en MicroPython:
   este monitor recorta ~7 filas de abajo.
3. **WiFi y portal de configuración**: portar de `pico/wifi.py`, `pico/portal.py`
   y `pico/store.py`. La lógica está probada, hay que traducirla.
4. **Generador de QR**: portar `pico/qr.py`. Es versión 3, nivel L, modo byte,
   máscara 0. Verificado contra un lector real.
5. **Datos ADS-B**: HTTP al proxy y parseo de JSON.
6. **El render con el diseño de la web** — el grueso del trabajo, y lo que
   Valentino quiere ver.

### Problema abierto: los logos

Los logos de aerolíneas de `docs/logos/` pesan **4,2 MB** y la Pico tiene 4 MB de
memoria de programa. **No entran.** Hay que resolverlo y es una decisión que
Valentino tiene que tomar:

- Convertirlos a un formato indexado chico (unos 30 KB los 800), o
- Guardar sólo las aerolíneas de la zona del equipo.

---

## 8. Cómo trabaja Valentino

Está en `~/.claude/CLAUDE.md`, pero lo esencial:

- **Español argentino, de vos, directo.** Sin preámbulos ni adulación.
- **Respuestas cortas.** No narrar cada paso ni repetir lo que dijo.
- Si da instrucciones claras, **trabajar solo y contar al final**. Si no, parar y
  preguntar todo junto.
- **Marcar lo que queda a medias.** Prefiere un "esto NO está" antes que una
  línea optimista.
- **No pushear sin avisar.** El repo es público.

Además, aprendido en esta sesión:

- **Las fotos con el celular funcionan muy bien** para diagnosticar. Pedirlas.
- **Su ojo vale más que la cámara** para juzgar brillos sutiles. La cámara sirve
  para lo estructural: rayas, corrimientos, geometría.
- **Intentar ver por la cámara del Mac no funciona**: macOS bloquea el acceso
  desde la terminal y FaceTime se queda en ahorro de energía. No insistir.
- Cuando algo no se entiende, **hacer un diagrama visual publicado como
  artifact**, no una tabla. Los pidió explícitamente dos veces.
- **Medir antes que adivinar.** Cada vez que se midió —niveles en los pines,
  contadores de DMA, posición de las máquinas de estado— apareció la causa en
  minutos. Cada vez que se adivinó, se perdió una hora.

---

## 9. Prompt para la sesión nueva

> Estoy siguiendo el proyecto Pico VGA Radar, en `~/Desktop/Proyectos/PicoVga`.
> Leé `TRASPASO.md` completo antes de hacer nada: tiene todo el contexto, el
> estado actual, el cableado, cómo compilar y cargar firmware, y los bugs ya
> resueltos que no hay que repetir.
>
> Venimos de una sesión larga donde dejamos andando el driver de video en C:
> 640×480 con 256 colores, verificado en el monitor. Lo que sigue es construir
> el radar sobre esa base, con el requisito de que **lo que se ve en la web se
> tiene que ver igual en la Pico**.
>
> Arrancá por las primitivas de dibujo en C —texto, líneas, círculos,
> rectángulos— y armá un patrón que las demuestre en pantalla. La placa está
> enchufada; para cargar firmware usá el truco de los 1200 baudios que está
> documentado, no me pidas apretar BOOTSEL.
