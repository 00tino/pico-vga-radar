// Cuenta los arranques cortos seguidos, para que el cliente pueda pedir la
// pantalla de configuracion sin ningun boton.
//
// El gesto es cortarle la corriente al equipo tres veces seguidas. La placa no
// puede ver los botones del monitor (se probo y esta descartado, ver
// monitor.c), pero de su propio arranque se entera siempre.
//
// La cuenta va en el ultimo sector de la flash. Dos detalles de por que esta
// hecho asi y no de la forma obvia:
//
//   - Borrar un sector tarda decenas de milisegundos, y con el video andando
//     eso rompe la imagen. Por eso el borrado se hace SOLO al arrancar, antes
//     de encender el video, y durante el funcionamiento lo unico que se
//     escribe es una pagina, que tarda medio milisegundo.
//   - En flash un bit puede pasar de 1 a 0 sin borrar, pero no al reves. Por
//     eso los arranques se cuentan como marcas: cada uno pone un byte en cero
//     y contar los ceros da la cuenta, sin tener que borrar nada.
#include "arranques.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/flash.h"
#include <string.h>
#include <stdio.h>

#define SECTOR_OFF   (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define MARCAS_MAX   32
#define BYTE_LARGO   32          // se marca cuando el equipo lleva un rato andando
#define USADO        0x00
#define LIBRE        0xFF

static const uint8_t *sector = (const uint8_t *)(XIP_BASE + SECTOR_OFF);
static uint8_t pagina[FLASH_PAGE_SIZE];
static int marcas_puestas;

// Escribir en la flash deja el chip sin poder leer programa mientras dura, y
// el nucleo 1 corre desde flash: si se lo agarra en el medio, se cuelga la
// placa. flash_safe_execute lo frena antes y lo suelta despues. Con el nucleo
// 1 todavia sin arrancar tambien sirve: ahi no tiene a quien frenar.
static void hacer_borrado(void *nada) {
    (void)nada;
    flash_range_erase(SECTOR_OFF, FLASH_SECTOR_SIZE);
}

static void hacer_grabado(void *nada) {
    (void)nada;
    flash_range_program(SECTOR_OFF, pagina, FLASH_PAGE_SIZE);
}

static void borrar(void) {
    if (flash_safe_execute(hacer_borrado, NULL, 2000) != PICO_OK)
        printf("arranques: no se pudo borrar el sector\n");
}

static void grabar_pagina(void) {
    if (flash_safe_execute(hacer_grabado, NULL, 2000) != PICO_OK)
        printf("arranques: no se pudo grabar la cuenta\n");
}

// Se llama al arrancar, ANTES de encender el video: aca es donde puede haber
// un borrado y no importa cuanto tarde.
bool arranques_contar(void) {
    if (!ARRANQUES_ACTIVO) return false;
    memcpy(pagina, sector, FLASH_PAGE_SIZE);

    // Si la vez pasada el equipo anduvo un rato, esto no es una seguidilla:
    // se limpia y se empieza de nuevo.
    if (pagina[BYTE_LARGO] == USADO) {
        borrar();
        memset(pagina, LIBRE, sizeof pagina);
        marcas_puestas = 0;
    }

    int n = 0;
    for (int i = 0; i < MARCAS_MAX; i++) if (pagina[i] == USADO) n++;

    // Este arranque es uno mas. Con tres seguidos se pide la configuracion.
    if (n + 1 >= ARRANQUES_PARA_CONFIG) {
        borrar();
        printf("arranques cortos seguidos: %d, se pide la configuracion\n", n + 1);
        return true;
    }
    if (n < MARCAS_MAX) {
        pagina[n] = USADO;
        grabar_pagina();
    }
    marcas_puestas = n + 1;
    printf("arranques cortos seguidos: %d de %d\n", n + 1, ARRANQUES_PARA_CONFIG);
    return false;
}

// Se llama cuando el equipo lleva un rato andando: deja la marca de que este
// arranque fue largo, asi el que viene arranca la cuenta de cero. Es una sola
// pagina, medio milisegundo: el video se pierde un cuadro y nada mas.
void arranques_fue_largo(void) {
    if (!ARRANQUES_ACTIVO) return;
    if (pagina[BYTE_LARGO] == USADO) return;
    pagina[BYTE_LARGO] = USADO;
    grabar_pagina();
    printf("el equipo lleva %d s andando: la cuenta de arranques vuelve a cero\n",
           ARRANQUES_SEGUNDOS);
}

int arranques_cuenta(void) { return marcas_puestas; }
