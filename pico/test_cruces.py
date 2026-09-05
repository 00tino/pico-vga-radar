from machine import Pin
import time
PINES = list(range(0, 10))          # GP0..GP9
NOMBRE = {0:"azul b0",1:"azul b1",2:"verde b0",3:"verde b1",4:"verde b2",
          5:"rojo b0",6:"rojo b1",7:"rojo b2",8:"hsync",9:"vsync"}
print("buscando pines que se toquen entre si...\n")
hallados = []
for a in PINES:
    salida = Pin(a, Pin.OUT)
    salida.value(1)
    entradas = {}
    for b in PINES:
        if b == a: continue
        entradas[b] = Pin(b, Pin.IN, Pin.PULL_DOWN)
    time.sleep_ms(30)
    for b, p in entradas.items():
        if p.value():
            hallados.append((a, b))
            print("  *** GP%d y GP%d SE ESTAN TOCANDO  (%s / %s)" % (a, b, NOMBRE[a], NOMBRE[b]))
    salida.value(0)
    Pin(a, Pin.IN, Pin.PULL_DOWN)
print("")
if hallados:
    print("RESULTADO: hay", len(hallados)//2, "cruce(s). Hay que separarlos.")
else:
    print("RESULTADO: ningun pin toca a otro. El cableado esta limpio.")

# Como usarlo:
#   mpremote run test_cruces.py
#
# Pone cada pin de video en alto de a uno y lee los otros nueve. Si dos pines
# se estan tocando en la protoboard, el que esta escuchando se enciende solo y
# el test lo reporta. No hace falta tester.
#
# Encontro el cruce GP2-GP3 de la primera unidad en 30 segundos, despues de un
# rato largo de mirar barras de colores tratando de adivinarlo a ojo.
