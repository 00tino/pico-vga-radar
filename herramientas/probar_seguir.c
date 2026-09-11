// Prueba a quien sigue el radar cuando el cliente pide seguir un vuelo.
//
// Lo que manda ADS-B es el codigo OACI de tres letras (ARG1885), pero el
// numero que conoce el pasajero, el que sale en el pasaje y el que ofrece la
// pagina, es el IATA de dos (AR1885). Comparar de una sola forma hacia que
// seguir un vuelo no encontrara nunca nada.
//
// Se corre con herramientas/probar_seguir.sh.
#include "../firmware-c/seguir.h"
#include "../firmware-c/aerolineas.h"
#include <stdio.h>
#include <string.h>

char radar_seguir[9] = "";

static int fallos;

// Arma un avion como lo deja vivo.c: con la aerolinea IATA ya resuelta desde
// el indicativo, que es de donde sale la mitad de las coincidencias.
static avion_t avion(const char *indicativo) {
    avion_t a;
    memset(&a, 0, sizeof a);
    snprintf(a.vuelo, sizeof a.vuelo, "%s", indicativo);
    const char *iata = aerolinea_de_indicativo(indicativo);
    if (iata) memcpy(a.aerolinea, iata, 2);
    return a;
}

static void probar(const char *busca, const char *indicativo, bool espera) {
    snprintf(radar_seguir, sizeof radar_seguir, "%s", busca);
    const avion_t a = avion(indicativo);
    const bool dio = radar_sigue_a(&a);
    const bool bien = dio == espera;
    if (!bien) fallos++;
    printf("  %-5s buscando %-9s contra %-9s (%s) -> %s\n",
           bien ? "ok" : "MAL", busca, indicativo,
           a.aerolinea[0] ? a.aerolinea : "--", dio ? "lo sigue" : "no");
}

int main(void) {
    printf("== el indicativo entero ==\n");
    probar("ARG1885", "ARG1885", true);    // tal cual viene por el aire
    probar("AR1885",  "ARG1885", true);    // como lo conoce el pasajero
    probar("AR 1885", "ARG1885", true);    // con un espacio en el medio
    probar("AR-1885", "ARG1885", true);    // con un guion
    probar("ar1885",  "ARG1885", true);    // en minuscula
    probar("AR1886",  "ARG1885", false);   // otro numero de la misma linea
    probar("LA1885",  "ARG1885", false);   // mismo numero, otra linea

    printf("\n== solo la aerolinea: sigue al primero que aparezca ==\n");
    probar("AR",  "ARG1885", true);
    probar("ARG", "ARG1885", true);
    probar("LA",  "ARG1885", false);
    probar("JJ",  "TAM8192", true);        // LATAM Brasil: TAM por aire, JJ en el pasaje

    printf("\n== lo que no tiene que seguir a nadie ==\n");
    probar("",        "ARG1885", false);   // sin nada pedido
    probar("AR1885",  "",        false);   // un avion sin indicativo
    probar("1885",    "ARG1885", false);   // solo numeros no alcanza
    probar("AR1885",  "LVS100",  false);   // una avioneta con su matricula

    printf("\n== una aerolinea que no esta en la tabla ==\n");
    // Andes no figura en docs/airlines.json, asi que de su indicativo no se
    // saca ningun codigo de dos letras. Por OACI tiene que andar igual: eso
    // es lo que hace que seguir un vuelo no dependa de que la tabla este
    // completa.
    probar("ANS5500", "ANS5500", true);
    probar("ANS",     "ANS5500", true);
    probar("OY5500",  "ANS5500", false);   // por IATA no puede: no esta

    printf("\n%s\n", fallos ? "HAY FALLAS" : "todo bien");
    return fallos ? 1 : 0;
}
