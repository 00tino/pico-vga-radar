#include "gfx.h"
#include "trig.h"

int gfx_banda_y0 = 0, gfx_banda_y1 = VGA_ALTO - 1;

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

// Midpoint: se calcula un octante y se refleja en los otros siete.
void gfx_circulo(int cx, int cy, int r, uint8_t c) {
    if (r < 0) return;
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        gfx_punto(cx + x, cy + y, c); gfx_punto(cx - x, cy + y, c);
        gfx_punto(cx + x, cy - y, c); gfx_punto(cx - x, cy - y, c);
        gfx_punto(cx + y, cy + x, c); gfx_punto(cx - y, cy + x, c);
        gfx_punto(cx + y, cy - x, c); gfx_punto(cx - y, cy - x, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
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

// Raiz cuadrada entera, para el borde circular del sector.
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
    for (int dy = dy0; dy <= dy1; dy++) {
        int med = isqrt(r * r - dy * dy);
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
