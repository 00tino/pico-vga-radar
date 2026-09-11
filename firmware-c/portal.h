// El servidor del portal de configuracion. Ver portal.c.
#ifndef PORTAL_H
#define PORTAL_H
#include <stdbool.h>

// Que acaba de guardar el cliente, para que el equipo reaccione.
typedef enum {
    PORTAL_NADA,
    PORTAL_WIFI,        // cargo la red: hay que reiniciar y conectarse
    PORTAL_PANTALLAS,   // mando la configuracion: guardarla y volver al radar
} portal_pasa_t;

// Arranca el servidor. es_ap dice si el equipo esta haciendo de router (y
// entonces sirve el formulario de la red) o si ya esta en la red de casa (y
// entonces manda a la web completa).
bool portal_arrancar(bool es_ap, const char *ip);
void portal_parar(void);

// Pide un barrido de redes y lo va atendiendo. Las dos desde el nucleo 1.
void portal_barrer(void);
void portal_atender(void);

portal_pasa_t portal_paso(void);
void          portal_paso_limpiar(void);

#endif
