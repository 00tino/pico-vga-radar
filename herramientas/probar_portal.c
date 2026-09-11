// Prueba que lo que manda docs/setup.html es lo que entiende el firmware.
//
// Es el punto mas facil de romper de todo el portal: son dos archivos que no
// se ven entre si, uno en JavaScript y otro en C, y el unico que se entera de
// que dejaron de hablar el mismo idioma es el cliente, mirando el monitor.
//
// Se corre con herramientas/probar_portal.sh, que primero le pide la direccion
// a la pagina de verdad (no a una copia) y despues se la da a esto.
#include "../firmware-c/portal_parse.h"
#include <stdio.h>
#include <string.h>

// Lo que en la placa pone pantallas.c.
pantalla_t pantallas[PANTALLAS_MAX];
int        pantallas_n;

static int fallos;
static void mal(const char *que) { printf("  MAL: %s\n", que); fallos++; }

static const char *NOMBRE_VISTA[] = {
    "solo radar", "radar + lista", "solo lista",
    "seguir (mapa)", "seguir (mapa + ficha)", "logos",
};

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "uso: probar_portal <la consulta que manda la web>\n");
        return 2;
    }

    printf("== lo que manda la web ==\n%s\n\n", argv[1]);

    pantalla_t p[PANTALLAS_MAX];
    const int n = portal_leer_pantallas(argv[1], p, PANTALLAS_MAX);
    printf("== lo que entiende la placa ==\n%d pantallas\n\n", n);
    if (n <= 0) { mal("no entro ninguna pantalla"); printf("\nHAY FALLAS\n"); return 1; }

    for (int i = 0; i < n; i++) {
        printf("%d. \"%s\"\n", i + 1, p[i].nombre);
        printf("   vista %-22s lista %s\n",
               p[i].vista <= VISTA_LOGOS ? NOMBRE_VISTA[p[i].vista] : "???",
               p[i].lista == LISTA_FIDS ? "pantalla de aeropuerto" : "tarjetas");
        printf("   %s a %d km | %d vuelos | tema %s | %d s%s%s\n\n",
               p[i].apt, p[i].radio_km, p[i].tarjetas, p[i].tema, p[i].segundos,
               p[i].seguir[0] ? " | sigue " : "", p[i].seguir);

        if (!p[i].apt[0])    mal("una pantalla sin aeropuerto");
        if (!p[i].nombre[0]) mal("una pantalla sin nombre");
        if (!p[i].tema[0])   mal("una pantalla sin tema");
        if (p[i].vista > VISTA_LOGOS) mal("una vista que no existe");
        if (p[i].lista > LISTA_FIDS)  mal("una forma de lista que no existe");
        if (p[i].radio_km < 5 || p[i].radio_km > 400) mal("un radio fuera de rango");
        if (p[i].tarjetas < 1 || p[i].tarjetas > 6)   mal("una cantidad de vuelos rara");
        if (p[i].segundos < PANTALLA_SEGUNDOS_MIN ||
            p[i].segundos > PANTALLA_SEGUNDOS_MAX)    mal("una duracion fuera de rango");
    }

    // Lo que tiene que aguantar sin romperse: basura, campos de menos, y una
    // direccion con cosas que no son pantallas.
    printf("== cosas mal formadas ==\n");
    struct { const char *que, *consulta; } feas[] = {
        {"vacia",                 ""},
        {"sin pantallas",         "n=3"},
        {"campos de menos",       "n=1&p0=Hola~1~0"},
        {"numeros imposibles",    "n=1&p0=X~99~99~EZE~99999~99~t~~99999"},
        {"aeropuerto vacio",      "n=1&p0=X~1~0~~150~3~crt_amber~~30"},
        {"mas de las que entran", "n=99&p0=X~1~0~EZE~150~3~crt_amber~~30"},
        {"nombre larguisimo",     "n=1&p0=" "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                  "~1~0~EZE~150~3~crt_amber~~30"},
        // Los negativos pasaban el control de la vista, porque -5 no es mayor
        // que la ultima vista, y terminaban usandose como indice.
        {"vista negativa",        "n=1&p0=X~-5~0~EZE~150~3~crt_amber~~30"},
        {"lista negativa",        "n=1&p0=X~1~-9~EZE~150~3~crt_amber~~30"},
        {"todo negativo",         "n=-1&p0=X~-1~-1~EZE~-150~-3~crt_amber~~-30"},
    };
    for (unsigned i = 0; i < sizeof feas / sizeof *feas; i++) {
        pantalla_t q[PANTALLAS_MAX];
        const int cuantas = portal_leer_pantallas(feas[i].consulta, q, PANTALLAS_MAX);
        printf("  %-22s -> %d pantallas", feas[i].que, cuantas);
        if (cuantas > 0) {
            printf(" (%s, %d km, %d s, vista %d, lista %d)", q[0].apt, q[0].radio_km,
                   q[0].segundos, (int)q[0].vista, (int)q[0].lista);
            if (q[0].radio_km < 5 || q[0].radio_km > 400) mal("dejo pasar un radio imposible");
            if (q[0].segundos > PANTALLA_SEGUNDOS_MAX) mal("dejo pasar una duracion imposible");
            if ((int)q[0].vista < 0 || q[0].vista > VISTA_LOGOS)
                mal("dejo pasar una vista que no existe");
            if ((int)q[0].lista < 0 || q[0].lista > LISTA_FIDS)
                mal("dejo pasar una forma de lista que no existe");
            if (q[0].tarjetas < 1 || q[0].tarjetas > 6)
                mal("dejo pasar una cantidad de vuelos imposible");
        }
        if (cuantas > PANTALLAS_MAX) mal("devolvio mas pantallas de las que entran");
        printf("\n");
    }

    // Los nombres de red los pone cualquiera que este cerca: tienen que poder
    // salir en la pagina y en el JSON sin romper ninguno de los dos.
    printf("\n== nombres de red con mala intencion ==\n");
    const char *feos[] = {
        "<script>alert(1)</script>",
        "Red\"; DROP",
        "comillas \" y \\ barra",
        "<img src=x onerror=alert(1)>",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",     // los 32 del maximo
    };
    for (unsigned i = 0; i < sizeof feos / sizeof *feos; i++) {
        char html[512], json[512];
        portal_escapar(feos[i], html, sizeof html);
        portal_escapar_json(feos[i], json, sizeof json);
        printf("  %-30s\n    html: %s\n    json: \"%s\"\n", feos[i], html, json);
        if (strchr(html, '<') || strchr(html, '>') || strchr(html, '"'))
            mal("quedo html sin escapar");
        // En el JSON, toda comilla que quede tiene que venir precedida de su
        // barra: si no, cierra la cadena antes de tiempo.
        for (const char *c = json; *c; c++) {
            if (*c == '\\') { if (c[1]) c++; continue; }
            if (*c == '"') { mal("quedo una comilla suelta en el json"); break; }
        }
    }

    printf("\n%s\n", fallos ? "HAY FALLAS" : "todo bien");
    return fallos ? 1 : 0;
}
