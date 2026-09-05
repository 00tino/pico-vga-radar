#include "gfx.h"
#include "trig.h"

int gfx_banda_y0 = 0, gfx_banda_y1 = VGA_ALTO - 1;

static int isqrt(int v);

void gfx_hlinea(int x, int y, int largo, uint8_t c) {
    if (y < gfx_banda_y0 || y > gfx_banda_y1) return;
    if (largo < 0) { x += largo + 1; largo = -largo; }
    if (x < 0) { largo += x; x = 0; }
    if (x + largo > VGA_ANCHO) largo = VGA_ANCHO - x;
    if (largo <= 0) return;
    uint8_t *p = &vga_fb[y * VGA_ANCHO + x];
    for (int i = 0; i < largo; i++) p[i] = c;
}

void gfx_vlinea(int x, int y, int largo, uint8_t c) {
    if ((unsigned)x >= VGA_ANCHO) return;
    if (largo < 0) { y += largo + 1; largo = -largo; }
    if (y < gfx_banda_y0) { largo += y - gfx_banda_y0; y = gfx_banda_y0; }
    if (y + largo > gfx_banda_y1 + 1) largo = gfx_banda_y1 + 1 - y;
    if (largo <= 0) return;
    uint8_t *p = &vga_fb[y * VGA_ANCHO + x];
    for (int i = 0; i < largo; i++, p += VGA_ANCHO) *p = c;
}

// Bresenham entero, sin divisiones ni flotantes.
void gfx_linea(int x0, int y0, int x1, int y1, uint8_t c) {
    if (y0 == y1) { gfx_hlinea(x0 < x1 ? x0 : x1, y0, (x1 > x0 ? x1 - x0 : x0 - x1) + 1, c); return; }
    if (x0 == x1) { gfx_vlinea(x0, y0 < y1 ? y0 : y1, (y1 > y0 ? y1 - y0 : y0 - y1) + 1, c); return; }
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        gfx_punto(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(int x, int y, int an, int al, uint8_t c) {
    if (an <= 0 || al <= 0) return;
    gfx_hlinea(x, y, an, c);
    gfx_hlinea(x, y + al - 1, an, c);
    gfx_vlinea(x, y, al, c);
    gfx_vlinea(x + an - 1, y, al, c);
}

void gfx_rect_lleno(int x, int y, int an, int al, uint8_t c) {
    if (al < 0) { y += al + 1; al = -al; }
    if (y < gfx_banda_y0) { al += y - gfx_banda_y0; y = gfx_banda_y0; }
    if (y + al > gfx_banda_y1 + 1) al = gfx_banda_y1 + 1 - y;
    for (int i = 0; i < al; i++) gfx_hlinea(x, y + i, an, c);
}

// Contorno de circulo por filas, no por Bresenham.
//
// El cuadro se pinta una vez por banda, y con el metodo del punto medio cada
// circulo se recorria entero diez veces aunque casi todo cayera afuera.
//
// El medio ancho de cada fila no se saca con una raiz: entre una fila y la
// siguiente cambia de a poco, asi que se arranca del valor anterior y se
// ajusta con un par de comparaciones. Una raiz por fila costaba mas que el
// Bresenham que se queria evitar.
//
// El tramo horizontal entre una fila y la siguiente se pinta entero, que es
// lo que evita los huecos arriba y abajo, donde el circulo es casi
// horizontal y salta varias columnas por fila.
void gfx_circulo(int cx, int cy, int r, uint8_t c) {
    if (r < 0) return;
    int dy0 = gfx_banda_y0 - cy, dy1 = gfx_banda_y1 - cy;
    if (dy0 < -r) dy0 = -r;
    if (dy1 >  r) dy1 =  r;
    if (dy0 > dy1) return;
    const int r2 = r * r;

    int x = isqrt(r2 - dy0 * dy0);
    for (int dy = dy0; dy <= dy1; dy++) {
        // Ajuste incremental del medio ancho para esta fila.
        while (x > 0 && x * x + dy * dy > r2) x--;
        while ((x + 1) * (x + 1) + dy * dy <= r2) x++;

        // Hasta donde llega la fila de al lado, para cerrar el tramo.
        int siguiente = x;
        int dy2 = dy + (dy < 0 ? 1 : -1);
        if (dy2 >= -r && dy2 <= r) {
            siguiente = x;
            while (siguiente > 0 && siguiente * siguiente + dy2 * dy2 > r2) siguiente--;
            while ((siguiente + 1) * (siguiente + 1) + dy2 * dy2 <= r2) siguiente++;
        }
        int desde = siguiente < x ? siguiente : x;
        int hasta = siguiente < x ? x : siguiente;
        int largo = hasta - desde + 1;
        gfx_hlinea(cx + desde, cy + dy, largo, c);
        gfx_hlinea(cx - hasta, cy + dy, largo, c);
    }
}

void gfx_circulo_lleno(int cx, int cy, int r, uint8_t c) {
    if (r < 0) return;
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        gfx_hlinea(cx - x, cy + y, 2 * x + 1, c);
        gfx_hlinea(cx - x, cy - y, 2 * x + 1, c);
        gfx_hlinea(cx - y, cy + x, 2 * y + 1, c);
        gfx_hlinea(cx - y, cy - x, 2 * y + 1, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

// La fuente guarda cada caracter como 7 columnas de 14 bits: bit 0 arriba.
static int dibujar_char(int x, int y, char ch, uint8_t c, int escala) {
    if (ch < 32 || ch > 127) ch = '?';
    const uint8_t *g = &gfx_fuente[(ch - 32) * 15];
    int avance = g[0];
    for (int col = 0; col < 7; col++) {
        uint16_t bits = (uint16_t)(g[1 + col * 2] | (g[2 + col * 2] << 8));
        if (!bits) continue;
        for (int fila = 0; fila < GFX_FUENTE_ALTO; fila++) {
            if (!(bits & (1u << fila))) continue;
            if (escala == 1) gfx_punto(x + col, y + fila, c);
            else gfx_rect_lleno(x + col * escala, y + fila * escala, escala, escala, c);
        }
    }
    return avance * escala;
}

int gfx_texto(int x, int y, const char *s, uint8_t c, int escala) {
    if (escala < 1) escala = 1;
    // Fuera de la banda no se dibuja nada, pero hay que devolver el ancho.
    if (y + GFX_FUENTE_ALTO * escala < gfx_banda_y0 || y > gfx_banda_y1)
        return gfx_ancho_texto(s, escala);
    int x0 = x;
    for (; *s; s++) x += dibujar_char(x, y, *s, c, escala);
    return x - x0;
}

int gfx_ancho_texto(const char *s, int escala) {
    if (escala < 1) escala = 1;
    int an = 0;
    for (; *s; s++) {
        char ch = (*s < 32 || *s > 127) ? '?' : *s;
        an += gfx_fuente[(ch - 32) * 15] * escala;
    }
    return an;
}

int gfx_texto_centrado(int cx, int y, const char *s, uint8_t c, int escala) {
    return gfx_texto(cx - gfx_ancho_texto(s, escala) / 2, y, s, c, escala);
}

static const uint8_t bayer4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5,
};

// Sube un escalon el canal cuando el resto supera el umbral de la celda.
static inline uint8_t canal(int v, int max, int umbral) {
    int t = v * max * 16 / 255;
    int n = t / 16 + ((t % 16) > umbral ? 1 : 0);
    return (uint8_t)(n > max ? max : n);
}

// El azul va sin dither a proposito: tiene 4 niveles, o sea escalones de 85
// sobre 255. Un solo pixel encendido en una zona oscura se ve como suciedad
// azul. En rojo y verde el escalon es 36 y el ruido no se nota.
uint8_t gfx_rgb_dither(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    int u = bayer4[(y & 3) * 4 + (x & 3)];
    return vga_color(canal(r, 7, u), canal(g, 7, u), (uint8_t)((b * 3 + 127) / 255));
}

void gfx_rect_dither(int x, int y, int an, int al, uint8_t r, uint8_t g, uint8_t b) {
    for (int j = 0; j < al; j++)
        for (int i = 0; i < an; i++)
            gfx_punto(x + i, y + j, gfx_rgb_dither(x + i, y + j, r, g, b));
}

void gfx_blit(int x, int y, int an, int al, const uint8_t *datos) {
    for (int j = 0; j < al; j++) {
        int fy = y + j;
        if (fy < gfx_banda_y0 || fy > gfx_banda_y1) continue;
        for (int i = 0; i < an; i++) {
            int fx = x + i;
            if ((unsigned)fx < VGA_ANCHO)
                vga_fb[fy * VGA_ANCHO + fx] = datos[j * an + i];
        }
    }
}

uint8_t gfx_mezcla(uint8_t r, uint8_t g, uint8_t b,
                   uint8_t fr, uint8_t fg_, uint8_t fb, int a) {
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return vga_rgb((uint8_t)((r * a + fr * (255 - a)) / 255),
                   (uint8_t)((g * a + fg_ * (255 - a)) / 255),
                   (uint8_t)((b * a + fb * (255 - a)) / 255));
}

// Rasterizado por barrido de filas: se ordenan los vertices por altura y se
// rellena entre los dos bordes activos. Alcanza para los triangulitos de los
// aviones, que es lo unico que dibuja triangulos.
void gfx_triangulo_lleno(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c) {
    int tx, ty;
    if (y0 > y1) { tx=x0;x0=x1;x1=tx; ty=y0;y0=y1;y1=ty; }
    if (y1 > y2) { tx=x1;x1=x2;x2=tx; ty=y1;y1=y2;y2=ty; }
    if (y0 > y1) { tx=x0;x0=x1;x1=tx; ty=y0;y0=y1;y1=ty; }
    if (y2 == y0) { gfx_hlinea(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), y0, 1, c); return; }
    // Recorte vertical: sin esto un triangulo con vertices lejos de la
    // pantalla recorre miles de filas para no dibujar nada.
    int ya = y0 < gfx_banda_y0 ? gfx_banda_y0 : y0;
    int yb = y2 > gfx_banda_y1 ? gfx_banda_y1 : y2;
    for (int y = ya; y <= yb; y++) {
        int xa = x0 + (x2 - x0) * (y - y0) / (y2 - y0);   // borde largo
        int xb;
        if (y < y1) xb = (y1 == y0) ? x0 : x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        else        xb = (y2 == y1) ? x1 : x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        gfx_hlinea(xa < xb ? xa : xb, y, (xa > xb ? xa - xb : xb - xa) + 1, c);
    }
}

void gfx_linea_punteada(int x0, int y0, int x1, int y1, int trazo, int hueco, uint8_t c) {
    int dx = x1 - x0, dy = y1 - y0;
    int pasos = (dx > 0 ? dx : -dx) > (dy > 0 ? dy : -dy)
              ? (dx > 0 ? dx : -dx) : (dy > 0 ? dy : -dy);
    if (pasos <= 0) return;
    int ciclo = trazo + hueco;
    for (int i = 0; i <= pasos; i++)
        if (i % ciclo < trazo)
            gfx_punto(x0 + dx * i / pasos, y0 + dy * i / pasos, c);
}

// Raiz cuadrada entera, para los circulos y el borde del sector.
static int isqrt(int v) {
    if (v <= 0) return 0;
    int x = v, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + v / x) / 2; }
    return x;
}

static inline int div_piso(int a, int b) {
    int q = a / b;
    if ((a % b) && ((a < 0) != (b < 0))) q--;
    return q;
}
static inline int div_techo(int a, int b) {
    int q = a / b;
    if ((a % b) && ((a < 0) == (b < 0))) q++;
    return q;
}

// Sector lleno, fila por fila.
//
// Antes esto se dibujaba con rayos desde el centro y quedaba rayado: una
// linea diagonal de Bresenham toca un solo pixel por columna, asi que entre
// un rayo y el siguiente quedaban huecos (medidos: 18 por ciento del area).
// Ahora cada fila se resuelve como la interseccion del disco con los dos
// semiplanos de los bordes, que da un tramo continuo y sale de una.
// Vale para sectores de menos de media vuelta, que es lo que usa el barrido.
void gfx_sector(int cx, int cy, int r, int a0, int a1, uint8_t c) {
    if (r <= 0 || a1 - a0 <= 0 || a1 - a0 >= TRIG_VUELTA / 2) return;
    const int c0 = trig_cos(a0), s0 = trig_sen(a0);
    const int c1 = trig_cos(a1), s1 = trig_sen(a1);
    // Solo las filas de la banda: calcular la raiz para las demas es tiempo
    // tirado, y esto se dibuja una vez por banda.
    int dy0 = gfx_banda_y0 - cy, dy1 = gfx_banda_y1 - cy;
    if (dy0 < -r) dy0 = -r;
    if (dy1 >  r) dy1 =  r;
    const int r2 = r * r;
    int med = isqrt(r2 - dy0 * dy0);
    for (int dy = dy0; dy <= dy1; dy++) {
        // Mismo ajuste incremental que en gfx_circulo, por lo mismo.
        while (med > 0 && med * med + dy * dy > r2) med--;
        while ((med + 1) * (med + 1) + dy * dy <= r2) med++;
        int lo = -med, hi = med;

        // Borde de arranque: el punto tiene que quedar de un lado de a0.
        if (s0 > 0)      hi = (hi < div_piso(c0 * dy, s0)) ? hi : div_piso(c0 * dy, s0);
        else if (s0 < 0) lo = (lo > div_techo(c0 * dy, s0)) ? lo : div_techo(c0 * dy, s0);
        else if (c0 * dy < 0) continue;

        // Borde de llegada: del otro lado de a1.
        if (s1 > 0)      lo = (lo > div_techo(c1 * dy, s1)) ? lo : div_techo(c1 * dy, s1);
        else if (s1 < 0) hi = (hi < div_piso(c1 * dy, s1)) ? hi : div_piso(c1 * dy, s1);
        else if (c1 * dy > 0) continue;

        if (lo <= hi) gfx_hlinea(cx + lo, cy + dy, hi - lo + 1, c);
    }
}

// Rellena un poligono: por cada fila se buscan los cruces con los lados, se
// ordenan y se pinta entre pares.
//
// Es el metodo directo, sin lista de aristas activas: ordenar las aristas por
// su fila de arranque costaba mas que esto, porque los contornos vienen en
// orden de recorrido y la insercion se vuelve cuadratica. Lo que hace que
// esto rinda es no llamarlo sesenta veces por segundo: el mapa se dibuja una
// sola vez y despues solo se mueve el avion.
void gfx_poligono_lleno(const int *xs, const int *ys, int n, uint8_t c) {
    if (n < 3) return;
    int ymin = ys[0], ymax = ys[0];
    for (int i = 1; i < n; i++) {
        if (ys[i] < ymin) ymin = ys[i];
        if (ys[i] > ymax) ymax = ys[i];
    }
    if (ymin < gfx_banda_y0) ymin = gfx_banda_y0;
    if (ymax > gfx_banda_y1) ymax = gfx_banda_y1;

    int cruces[128];
    for (int y = ymin; y <= ymax; y++) {
        int m = 0;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            int y0 = ys[j], y1 = ys[i];
            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) {
                if (m < (int)(sizeof cruces / sizeof cruces[0]))
                    cruces[m++] = xs[j] + (y - y0) * (xs[i] - xs[j]) / (y1 - y0);
            }
        }
        for (int a = 1; a < m; a++) {
            int v = cruces[a], b = a - 1;
            while (b >= 0 && cruces[b] > v) { cruces[b + 1] = cruces[b]; b--; }
            cruces[b + 1] = v;
        }
        for (int a = 0; a + 1 < m; a += 2)
            gfx_hlinea(cruces[a], y, cruces[a + 1] - cruces[a] + 1, c);
    }
}

void gfx_guardar(int x, int y, int an, int al, uint8_t *dst) {
    for (int j = 0; j < al; j++) {
        int fy = y + j;
        for (int i = 0; i < an; i++) {
            int fx = x + i;
            dst[j * an + i] = ((unsigned)fx < VGA_ANCHO && (unsigned)fy < VGA_ALTO)
                            ? vga_fb[fy * VGA_ANCHO + fx] : 0;
        }
    }
}

void gfx_reponer(int x, int y, int an, int al, const uint8_t *src) {
    for (int j = 0; j < al; j++) {
        int fy = y + j;
        if ((unsigned)fy >= VGA_ALTO) continue;
        for (int i = 0; i < an; i++) {
            int fx = x + i;
            if ((unsigned)fx < VGA_ANCHO) vga_fb[fy * VGA_ANCHO + fx] = src[j * an + i];
        }
    }
}

// Recorte de poligono contra un rectangulo, por los cuatro lados.
//
// Para cada lado se recorre el poligono y se van quedando los vertices de
// adentro, agregando el punto de cruce cuando un lado atraviesa el borde. El
// resultado es un poligono nuevo, cerrado y correcto, que ya se puede
// rellenar sin que el relleno se escape.
static int recortar_lado(const int *xs, const int *ys, int n,
                         int lado, int valor, int *sx, int *sy, int max) {
    int m = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        int xi = xs[i], yi = ys[i], xj = xs[j], yj = ys[j];
        int di, dj;
        switch (lado) {
            case 0: di = xi - valor; dj = xj - valor; break;   // x >= valor
            case 1: di = valor - xi; dj = valor - xj; break;   // x <= valor
            case 2: di = yi - valor; dj = yj - valor; break;   // y >= valor
            default:di = valor - yi; dj = valor - yj; break;   // y <= valor
        }
        int dentro_i = di >= 0, dentro_j = dj >= 0;
        if (dentro_i != dentro_j && (di - dj) != 0) {
            // Punto donde el lado cruza el borde.
            int t_num = dj, t_den = dj - di;
            int cx = xj + (int)((int64_t)(xi - xj) * t_num / t_den);
            int cy = yj + (int)((int64_t)(yi - yj) * t_num / t_den);
            if (m < max) { sx[m] = cx; sy[m] = cy; m++; }
        }
        if (dentro_i && m < max) { sx[m] = xi; sy[m] = yi; m++; }
    }
    return m;
}

int gfx_poligono_recortar(const int *xs, const int *ys, int n,
                          int rx0, int ry0, int rx1, int ry1,
                          int *sx, int *sy, int max) {
    static int ax[GFX_POLIGONO_MAX * 2], ay[GFX_POLIGONO_MAX * 2];
    const int amax = GFX_POLIGONO_MAX * 2;
    int m = recortar_lado(xs, ys, n, 0, rx0, ax, ay, amax);
    if (!m) return 0;
    m = recortar_lado(ax, ay, m, 1, rx1, sx, sy, max);
    if (!m) return 0;
    m = recortar_lado(sx, sy, m, 2, ry0, ax, ay, amax);
    if (!m) return 0;
    return recortar_lado(ax, ay, m, 3, ry1, sx, sy, max);
}
