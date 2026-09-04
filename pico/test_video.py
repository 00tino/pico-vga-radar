# PASO 1: probar que el monitor recibe imagen.
# Corre esto ANTES que nada. Si no ves esta pantalla, el problema es el
# cableado o el monitor, y no tiene sentido seguir con el resto.
#
# En Thonny: abri este archivo y aprieta Run (F5).

import ui

ui.clear()
ui.center(40, "PICO VGA RADAR", ui.YELLOW)
ui.center(80, "SI LEES ESTO, EL VIDEO ANDA", ui.WHITE)

s = ui.screen()

# barras de color: sirven para ver si los tres cables de color estan bien
names = ("NEGRO", "AZUL", "VERDE", "CYAN", "ROJO", "MAGENTA", "AMARILLO", "BLANCO")
bw = ui.W // 8
for i in range(8):
    s.fill_rect(i * bw, 140, (i + 1) * bw - 2, 260, i)
    s.settextcolor(ui.WHITE)
    s.settextcursor(i * bw + 4, 285)
    s.printh(names[i][:7])

# marco: si no ves los cuatro bordes, el monitor esta recortando la imagen
s.draw_rect(0, 0, ui.W - 1, ui.H - 1, ui.WHITE)

# grilla para ver que la imagen este derecha
for x in range(0, ui.W, 100):
    s.draw_fastVline(x, 320, 560, ui.BLUE)
for y in range(320, 561, 40):
    s.draw_fastHline(0, ui.W - 1, y, ui.BLUE)

s.draw_circle(400, 440, 100, ui.GREEN)
s.draw_circle(400, 440, 60, ui.GREEN)

ui.center(575, "800x600 60Hz - 8 colores", ui.YELLOW)
