# Qué cargarle a la Raspberry Pi Pico 2 W

Instrucciones desde cero. No hace falta saber programar ni instalar
compiladores: se arrastran archivos y listo.

---

## Antes que nada: qué es cada cosa

La Pico viene vacía. Hay que ponerle dos cosas, en este orden:

1. **MicroPython** — el "sistema operativo". Se pone una sola vez, arrastrando
   un archivo. Después de esto la Pico entiende Python.
2. **Los archivos de esta carpeta** — el radar en sí.

---

## Paso 1 — Instalar MicroPython (una sola vez)

1. Bajá el archivo `.uf2` de MicroPython **para Pico 2 W** de acá:
   https://micropython.org/download/RPI_PICO2_W/
   (el primero de la lista, el más nuevo)

2. Con la Pico **desenchufada**, mantené apretado el botón blanco **BOOTSEL**
   y, sin soltarlo, enchufá el cable USB a la computadora.

3. Aparece un pendrive nuevo llamado **RP2350**. Soltá el botón.

4. Arrastrá el `.uf2` adentro de ese pendrive.

5. El pendrive desaparece solo. Eso significa que salió bien.

---

## Paso 2 — Instalar Thonny (una sola vez)

Thonny es el programa para pasarle archivos a la Pico.

1. Bajalo de https://thonny.org y instalalo.
2. Abrilo, con la Pico enchufada.
3. Abajo a la derecha dice el intérprete. Elegí **MicroPython (Raspberry Pi Pico)**.
4. Menú **Ver → Archivos**. Se abren dos paneles: arriba tu computadora,
   abajo la Pico.

---

## Paso 3 — Copiar los archivos

En el panel de arriba buscá esta carpeta (`pico/`). Copiá **todo el
contenido** a la Pico, respetando las subcarpetas:

```
main.py           store.py     config.json     VGA/  (carpeta entera)
ui.py             sky.py       install.json    fonts/ (carpeta entera)
qr.py             render.py
wifi.py           portal.py    test_video.py
```

Para copiar: clic derecho sobre el archivo en el panel de arriba →
**Upload to /**. Para las carpetas `VGA` y `fonts`, clic derecho sobre la
carpeta y **Upload to /** también.

---

## Paso 4 — Probar que hay imagen (¡hacé esto primero!)

Antes de nada, cableá el monitor siguiendo **[CABLEADO.md](CABLEADO.md)**.

En Thonny abrí `test_video.py` y apretá **Run (F5)**.

En el monitor tenés que ver un cartel, 8 barras de colores y un par de
círculos. Si eso aparece, el video anda y ya está lo más difícil.

**Si el monitor dice "sin señal":**
- Revisá que GND esté conectado (es el error más común).
- Fijate que HSYNC (13) y VSYNC (14) no estén cruzados.
- Si sigue sin andar, abrí `VGA/VGA_800x600.py` y cambiá arriba de todo
  `PIO_UNIT = 0` por `PIO_UNIT = 1`, guardá y probá de nuevo.

**Si ves la imagen pero con colores raros:** las barras están rotuladas. El
rótulo tiene que coincidir con el color que ves. Si no, tenés cruzados los
cables de color (GP18/19/20).

---

## Paso 5 — Usarlo

Desenchufá y volvé a enchufar la Pico. Ahora arranca solo con `main.py`.

**Primera vez (todavía no sabe ninguna red WiFi):**

1. En el monitor aparece un **código QR** y abajo el nombre de una red WiFi
   (`RADAR-XXXX`) con su clave.
2. Desde el celular, entrá a esa red WiFi.
3. Escaneá el QR (o abrí `http://192.168.4.1`).
4. Cargá tu red WiFi de casa y la contraseña. Guardar.
5. La Pico se reinicia sola y se conecta.

**Con WiFi ya configurado:**

1. Arranca, se conecta, y muestra un QR durante 25 segundos.
2. Ese QR abre la página de configuración completa en el celular.
3. Elegís aeropuerto, radio, vista, filtros, tema. Guardar.
4. El monitor cambia en el momento y empieza a mostrar el tráfico.
5. Si no tocás nada, a los 25 segundos arranca solo con lo último guardado.

---

## Cómo lo dejás configurado antes de venderlo

`install.json` es tu parte, la del instalador. Editalo en Thonny antes de
entregar el equipo:

```json
{"inches": 15, "w": 800, "h": 600}
```

Ponés las pulgadas del monitor que le vas a poner y la resolución. El cliente
no ve esto ni lo puede cambiar: a él le aparecen sólo las opciones de
**Monitor** y **Cliente**.

`config.json` es lo del cliente y se sobreescribe solo cada vez que guarda
desde el celular. Podés dejarlo con la configuración por defecto que quieras
que vea al prender por primera vez.

---

## Qué hace cada archivo

| Archivo | Para qué |
|---|---|
| `main.py` | El que arranca solo. Ordena todo lo demás. |
| `ui.py` | Las pantallas: inicio, QR, error. |
| `qr.py` | Genera el código QR que se ve en el monitor. |
| `wifi.py` | Conectarse a la red, o crear la propia si no hay. |
| `portal.py` | El servidor web chiquito que atiende al celular. |
| `store.py` | Leer y guardar `config.json` e `install.json`. |
| `sky.py` | Pide los vuelos al proxy. |
| `render.py` | Dibuja el radar y la lista. |
| `VGA/` | La librería de video (no la toques salvo el `PIO_UNIT`). |
| `fonts/` | Tipografías para el texto en pantalla. |
| `test_video.py` | La prueba del Paso 4. No se usa después. |

---

## Estado real de esto

Lo que está **escrito y andando en la parte que se puede probar sin la placa**:
el generador de QR (verificado contra un lector real), la lógica de
configuración y el servidor web.

Lo que está **escrito pero sin probar en hardware**, porque hace falta la Pico
con el monitor cableado: el video, el WiFi y el dibujado. La librería de video
es de terceros y está probada por su autor en una Pico 2, pero nadie la probó
todavía **con el WiFi prendido al mismo tiempo** — ese es el punto de riesgo,
y por eso está el interruptor `PIO_UNIT`.

Lo que **falta**: el radar dibuja puntos y la lista es texto. Las tarjetas con
logo de aerolínea, ruta, horarios y barra de progreso —lo que se ve en la web—
todavía no están pasadas al monitor. También faltan las pistas y la senda de
aproximación.
