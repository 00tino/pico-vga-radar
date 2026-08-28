# Pico VGA Radar

Radar de trafico aereo en un monitor VGA, impulsado por una **Raspberry Pi Pico 2 W**.
Setup como The FlightWall: al prender, el monitor muestra un QR y se configura todo desde una web. Sin app nativa y sin suscripcion.

## Web del proyecto

https://00tino.github.io/pico-vga-radar/

Wizard de configuracion (la misma UI que servira el Pico):

https://00tino.github.io/pico-vga-radar/setup.html

Repo: https://github.com/00tino/pico-vga-radar

## Idea

- Pico 2 W genera VGA (PicoVGA) y se conecta a Wi-Fi 2.4 GHz.
- Datos ADS-B gratis: [adsb.lol](https://api.adsb.lol) y OpenSky de respaldo.
- Tres vistas: radar, lista tipo FlightWall, o hibrido.
- Primera vez (o si falla el Wi-Fi): SoftAP `RADAR-XXXX` + pagina en `http://192.168.4.1`.
- Sin boton extra de setup. Si no hay red, entra al AP.

## Hardware

Ver [hardware/PINOUT.md](hardware/PINOUT.md).

- Raspberry Pi Pico 2 W
- Monitor VGA + conector DE-15
- 8 resistencias de color + 1 o 2 de sync

## Estado

Fase 0: arquitectura, pinout y web de configuracion.
Siguiente: test pattern VGA + SoftAP sirviendo esta misma pagina.
