// QR minimo: version 3 (29x29), correccion L, modo byte, mascara 0.
//
// Es el mismo de pico/qr.py, pasado a C sin cambiarle nada: alcanza hasta 53
// caracteres, que es de sobra para "http://192.168.4.1". No es una libreria
// general de QR y no pretende serlo; hace una sola cosa y ocupa poco.
//
// Que sea siempre la version 3 y la mascara 0 es lo que lo hace tan corto: no
// hay que elegir version, ni probar mascaras, ni calcular la penalizacion de
// cada una. La URL del portal nunca cambia de largo, asi que no hace falta.
#include "qr.h"
#include <string.h>

// --- aritmetica del campo de Galois, para los codigos de correccion --------
static uint8_t EXP[512], LOG[256];
static bool tablas_listas;

static void tablas(void) {
    if (tablas_listas) return;
    int x = 1;
    for (int i = 0; i < 255; i++) {
        EXP[i] = (uint8_t)x;
        LOG[x] = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11D;
    }
    for (int i = 255; i < 512; i++) EXP[i] = EXP[i - 255];
    tablas_listas = true;
}

#define DATA_CW  55            // version 3, nivel L
#define EC_CW    15
#define ALIGN    22            // centro del patron de alineacion
#define FORMATO  0x77C4        // nivel L + mascara 0, con su BCH ya hecho

// Los quince bytes de correccion de los cincuenta y cinco de datos.
static void correccion(const uint8_t *datos, uint8_t *salida) {
    uint8_t gen[EC_CW + 1];
    memset(gen, 0, sizeof gen);
    gen[0] = 1;
    int largo = 1;
    for (int i = 0; i < EC_CW; i++) {
        // gen = gen * (x + EXP[i]). El coeficiente que entra es el de al lado
        // multiplicado por EXP[i]; el primero se queda como estaba. Se
        // recorre de atras para adelante para no pisar lo que falta usar.
        for (int j = largo; j > 0; j--)
            gen[j] = (uint8_t)(gen[j] ^ (gen[j - 1] ? EXP[LOG[gen[j - 1]] + i] : 0));
        largo++;
    }

    uint8_t resto[DATA_CW + EC_CW];
    memset(resto, 0, sizeof resto);
    memcpy(resto, datos, DATA_CW);
    for (int i = 0; i < DATA_CW; i++) {
        uint8_t c = resto[i];
        if (!c) continue;
        int lc = LOG[c];
        for (int j = 0; j <= EC_CW; j++)
            if (gen[j]) resto[i + j] ^= EXP[lc + LOG[gen[j]]];
    }
    memcpy(salida, resto + DATA_CW, EC_CW);
}

// --- la grilla ------------------------------------------------------------
// Marca las celdas que ocupan los patrones fijos: esas no llevan datos.
static void reservadas(uint8_t res[QR_LADO][QR_LADO]) {
    memset(res, 0, QR_LADO * QR_LADO);
    struct { int x, y, an, al; } cajas[] = {
        {0, 0, 9, 9},                        // buscador de arriba a la izquierda
        {QR_LADO - 8, 0, 8, 9},              // el de arriba a la derecha
        {0, QR_LADO - 8, 9, 8},              // el de abajo a la izquierda
        {ALIGN - 2, ALIGN - 2, 5, 5},        // el de alineacion
    };
    for (unsigned c = 0; c < sizeof cajas / sizeof *cajas; c++)
        for (int j = cajas[c].y; j < cajas[c].y + cajas[c].al; j++)
            for (int i = cajas[c].x; i < cajas[c].x + cajas[c].an; i++)
                if (j >= 0 && j < QR_LADO && i >= 0 && i < QR_LADO) res[j][i] = 1;
    // Los temporizadores, la fila y la columna 6.
    for (int i = 0; i < QR_LADO; i++) { res[6][i] = 1; res[i][6] = 1; }
}

static void dibujar_fijos(uint8_t m[QR_LADO][QR_LADO]) {
    // Los tres cuadrados de las esquinas, con su marco vacio alrededor.
    const int esquinas[3][2] = {{0, 0}, {QR_LADO - 7, 0}, {0, QR_LADO - 7}};
    for (int e = 0; e < 3; e++) {
        const int ox = esquinas[e][0], oy = esquinas[e][1];
        for (int j = -1; j < 8; j++) {
            for (int i = -1; i < 8; i++) {
                const int x = ox + i, y = oy + j;
                if (x < 0 || x >= QR_LADO || y < 0 || y >= QR_LADO) continue;
                const bool borde = ((i == 0 || i == 6) && j >= 0 && j <= 6) ||
                                   ((j == 0 || j == 6) && i >= 0 && i <= 6);
                const bool centro = i >= 2 && i <= 4 && j >= 2 && j <= 4;
                m[y][x] = (borde || centro) ? 1 : 0;
            }
        }
    }

    for (int i = 8; i < QR_LADO - 8; i++) {
        const uint8_t v = (i % 2 == 0) ? 1 : 0;
        m[6][i] = v;
        m[i][6] = v;
    }

    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            const int ai = i < 0 ? -i : i, aj = j < 0 ? -j : j;
            m[ALIGN + j][ALIGN + i] = ((ai > aj ? ai : aj) != 1) ? 1 : 0;
        }
    }

    m[QR_LADO - 8][8] = 1;     // el modulo oscuro que la norma obliga

    // La informacion de formato va dos veces, en los dos lugares que manda la
    // norma, para que se lea aunque una esquina este tapada.
    uint8_t bits[15];
    for (int i = 0; i < 15; i++) bits[i] = (FORMATO >> (14 - i)) & 1;
    for (int i = 0; i < 6; i++) m[8][i] = bits[i];
    m[8][7] = bits[6];
    m[8][8] = bits[7];
    m[7][8] = bits[8];
    for (int i = 9; i < 15; i++) m[14 - i][8] = bits[i];
    for (int i = 0; i < 8; i++) m[QR_LADO - 1 - i][8] = bits[i];
    for (int i = 8; i < 15; i++) m[8][QR_LADO - 15 + i] = bits[i];
}

bool qr_armar(const char *texto, uint8_t m[QR_LADO][QR_LADO]) {
    tablas();
    const int largo = (int)strlen(texto);
    if (largo > QR_TEXTO_MAX) return false;

    // Los datos como cadena de bits: cuatro del modo, ocho del largo, y
    // despues el texto tal cual.
    uint8_t bits[DATA_CW * 8];
    int n = 0;
    const int cabecera = 0x4;                       // modo byte
    for (int i = 3; i >= 0; i--) bits[n++] = (cabecera >> i) & 1;
    for (int i = 7; i >= 0; i--) bits[n++] = (largo >> i) & 1;
    for (int b = 0; b < largo; b++)
        for (int i = 7; i >= 0; i--) bits[n++] = ((uint8_t)texto[b] >> i) & 1;

    // Terminador y relleno hasta completar el ultimo byte.
    int fin = DATA_CW * 8 - n;
    if (fin > 4) fin = 4;
    for (int i = 0; i < fin; i++) bits[n++] = 0;
    while (n % 8) bits[n++] = 0;

    uint8_t datos[DATA_CW];
    int cuantos = 0;
    for (int i = 0; i < n; i += 8) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; j++) byte = (uint8_t)((byte << 1) | bits[i + j]);
        datos[cuantos++] = byte;
    }
    // Los dos bytes de relleno que manda la norma, alternados.
    const uint8_t relleno[2] = {0xEC, 0x11};
    for (int k = 0; cuantos < DATA_CW; k++) datos[cuantos++] = relleno[k & 1];

    uint8_t todo[DATA_CW + EC_CW];
    memcpy(todo, datos, DATA_CW);
    correccion(datos, todo + DATA_CW);

    memset(m, 0, QR_LADO * QR_LADO);
    uint8_t res[QR_LADO][QR_LADO];
    reservadas(res);
    dibujar_fijos(m);

    // El recorrido en zigzag: de a dos columnas, desde abajo a la derecha
    // hacia arriba, saltando lo reservado.
    int idx = 0;
    bool subiendo = true;
    for (int col = QR_LADO - 1; col > 0; col -= 2) {
        if (col == 6) col--;         // la columna del temporizador no cuenta
        for (int k = 0; k < QR_LADO; k++) {
            const int fila = subiendo ? QR_LADO - 1 - k : k;
            for (int d = 0; d < 2; d++) {
                const int c = col - d;
                if (res[fila][c]) continue;
                uint8_t bit = 0;
                if (idx < (DATA_CW + EC_CW) * 8) {
                    bit = (todo[idx / 8] >> (7 - (idx % 8))) & 1;
                }
                idx++;
                if ((fila + c) % 2 == 0) bit ^= 1;      // mascara 0
                m[fila][c] = bit;
            }
        }
        subiendo = !subiendo;
    }
    return true;
}
