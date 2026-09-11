// Lo que define Valentino al instalar cada equipo. El cliente no ve nada de
// esto: va grabado en la placa.
//
// ES EL UNICO LUGAR donde se toca para armar un equipo con otro monitor. Si
// hay que cambiar algo del monitor y no esta aca, esta mal puesto.
//
// Como se saca cada numero, para un monitor nuevo:
//   1. Cargar el firmware y mandar 'c' por la consola: sale la pantalla de
//      calibracion, con reglas numeradas contra los cuatro bordes.
//   2. Mirar el monitor (o sacarle una foto con la GoPro) y leer, en cada
//      borde, desde que numero se empieza a ver la regla.
//   3. Ese numero es el margen de ese borde. Cargarlo aca abajo.
//   4. Recompilar y cargar.
//
// El framebuffer no sirve para esto: siempre esta entero. Lo que recorta es
// el monitor, asi que hay que verlo en la pantalla de verdad.
#ifndef INSTALACION_H
#define INSTALACION_H

// --- Monitor ---
// El nombre es solo para saber de que equipo se trata al leer el codigo.
#define INSTALACION_MONITOR "ViewSonic VA1703wb 17 pulgadas"

// Cuanto recorta el monitor en cada borde, en pixeles. Salen de la pantalla
// de calibracion. En el ViewSonic recorta entre 1 y 3 px; con 4 sobra y no se
// pierde nada.
#define INSTALACION_MARGEN_ARRIBA    4
#define INSTALACION_MARGEN_ABAJO     4
#define INSTALACION_MARGEN_IZQUIERDA 4
#define INSTALACION_MARGEN_DERECHA   4

// Este monitor estira el 4:3 a pantalla ancha: los circulos salen ovalados.
// Se arregla en el menu del monitor. Si algun equipo viene con un monitor que
// no se puede corregir, hay que compensarlo por software y este es el lugar
// donde va a ir la marca.
#define INSTALACION_ESTIRA_A_PANORAMICA 1

// --- Ubicacion propia ("mi casa") ---
// Hasta que este el portal, la casa se define aca. Despues la va a fijar el
// cliente desde la web y esto queda como valor de fabrica.
#define INSTALACION_CASA_ON     1
#define INSTALACION_CASA_NOMBRE "CASA"
#define INSTALACION_CASA_KM     3

// --- WiFi ---
// Hasta que este el portal, la red se define aca, igual que la casa. Cuando
// el cliente pueda cargarla desde el celular, esto queda como valor de
// fabrica: si en la flash hay una red guardada, gana la de la flash.
//
// Si INSTALACION_WIFI_SSID queda vacio, el equipo ni prende la radio y
// muestra el trafico simulado de siempre.
#define INSTALACION_WIFI_SSID  ""
#define INSTALACION_WIFI_PASS  ""

// Pais de la radio, para que los canales permitidos sean los que
// corresponden. Argentina no esta en la lista del driver; WORLDWIDE anda en
// todos lados y es lo que usa el propio SDK por defecto.
#define INSTALACION_WIFI_PAIS  CYW43_COUNTRY_WORLDWIDE

// Minutos de diferencia con UTC, para la hora local de las tarjetas.
// Argentina es UTC-3, o sea -180.
#define INSTALACION_TZ_MINUTOS (-180)

#endif
