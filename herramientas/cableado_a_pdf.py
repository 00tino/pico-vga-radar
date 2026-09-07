#!/usr/bin/env python3
"""Genera el plano de cableado del equipo, en PDF, para tenerlo en el taller.

Se escribe el PDF a mano, sin librerias: es texto con operadores de dibujo, y
para lineas, rectangulos y texto alcanza de sobra. Sale vectorial, o sea que
se puede imprimir o agrandar todo lo que haga falta sin que se pixele.

Todo lo que dice sale del cableado ya verificado en la placa; si se cambia un
cable, hay que cambiarlo aca y en el TRASPASO.

  python3 herramientas/cableado_a_pdf.py [salida.pdf]
"""
import sys, zlib

A4_AN, A4_AL = 595.28, 841.89

# ---------------------------------------------------------------- los datos

# El pinout de la Pico, de arriba (lado del USB) hacia abajo. Izquierda son
# los pines 1 al 20 y derecha del 21 al 40.
PICO_IZQ = ["GP0", "GP1", "GND", "GP2", "GP3", "GP4", "GP5", "GND", "GP6", "GP7",
            "GP8", "GP9", "GND", "GP10", "GP11", "GP12", "GP13", "GND", "GP14", "GP15"]
PICO_DER = ["VBUS", "VSYS", "GND", "3V3_EN", "3V3", "ADC_VREF", "GP28", "AGND",
            "GP27", "GP26", "RUN", "GP22", "GND", "GP21", "GP20", "GP19", "GP18",
            "GND", "GP17", "GP16"]

# Que va en cada pin: señal, resistencia y agujero del VGA.
CONEX = {
    "GP0":  ("Azul  bit 0", "1 k",     3),
    "GP1":  ("Azul  bit 1", "500",     3),
    "GP2":  ("Verde bit 0", "1,95 k",  2),
    "GP3":  ("Verde bit 1", "1 k",     2),
    "GP4":  ("Verde bit 2", "500",     2),
    "GP5":  ("Rojo  bit 0", "1,95 k",  1),
    "GP6":  ("Rojo  bit 1", "1 k",     1),
    "GP7":  ("Rojo  bit 2", "500",     1),
    "GP8":  ("HSync",       "directo", 13),
    "GP9":  ("VSync",       "directo", 14),
    "GP10": ("DDC datos",   "1,95 k",  12),
    "GP11": ("DDC reloj",   "1,95 k",  15),
}
GRUPO = {1: "rojo", 2: "verde", 3: "azul", 13: "sync", 14: "sync",
         12: "ddc", 15: "ddc"}
COLOR = {
    "rojo":  (0.85, 0.24, 0.20),
    "verde": (0.24, 0.68, 0.33),
    "azul":  (0.25, 0.47, 0.90),
    "sync":  (0.85, 0.65, 0.20),
    "ddc":   (0.62, 0.40, 0.80),
    "masa":  (0.42, 0.42, 0.45),
}
MASAS = [5, 6, 7, 8, 10]

# Que es cada agujero del VGA, para que se entienda por que va ahi.
VGA_QUE = {
    1: "Rojo", 2: "Verde", 3: "Azul", 4: "sin uso", 5: "Masa",
    6: "Masa rojo", 7: "Masa verde", 8: "Masa azul", 9: "+5 V  NO CONECTAR",
    10: "Masa sync", 11: "sin uso", 12: "DDC datos", 13: "HSync",
    14: "VSync", 15: "DDC reloj",
}

# ------------------------------------------------------------- el PDF crudo

class Pdf:
    def __init__(self):
        self.paginas = []
        self.op = []

    # -- operadores de dibujo --
    def color(self, r, g, b):     self.op.append("%.3f %.3f %.3f RG" % (r, g, b))
    def relleno(self, r, g, b):   self.op.append("%.3f %.3f %.3f rg" % (r, g, b))
    def grosor(self, w):          self.op.append("%.2f w" % w)
    def linea(self, x1, y1, x2, y2):
        self.op.append("%.2f %.2f m %.2f %.2f l S" % (x1, y1, x2, y2))
    def poli(self, pts):
        self.op.append("%.2f %.2f m " % pts[0] + " ".join("%.2f %.2f l" % p for p in pts[1:]) + " S")
    def rect(self, x, y, an, al, lleno=False):
        self.op.append("%.2f %.2f %.2f %.2f re %s" % (x, y, an, al, "f" if lleno else "S"))
    def rect_redondo(self, x, y, an, al, r=4):
        k = r * 0.5523          # cuanto sale la curva de Bezier de la esquina
        self.op.append(
            "%.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c "
            "%.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c "
            "%.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c "
            "%.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c h S" % (
                x + r, y,            x + an - r, y,
                x + an - k, y, x + an, y + k, x + an, y + r,
                x + an, y + al - r,
                x + an, y + al - k, x + an - k, y + al, x + an - r, y + al,
                x + r, y + al,
                x + k, y + al, x, y + al - k, x, y + al - r,
                x, y + r,
                x, y + k, x + k, y, x + r, y))

    def circulo(self, cx, cy, r, lleno=False):
        k = r * 0.5523
        self.op.append(
            "%.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c "
            "%.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c "
            "%.2f %.2f %.2f %.2f %.2f %.2f c %s" % (
                cx + r, cy,  cx + r, cy + k, cx + k, cy + r, cx, cy + r,
                cx - k, cy + r, cx - r, cy + k, cx - r, cy,
                cx - r, cy - k, cx - k, cy - r, cx, cy - r,
                cx + k, cy - r, cx + r, cy - k, cx + r, cy,
                "f" if lleno else "S"))
    def texto(self, x, y, s, tam=8, fuente="F1"):
        s = (s.replace("\\", r"\\").replace("(", r"\(").replace(")", r"\)")
              .replace("Ω", "ohm").replace("→", "->").replace("·", "-"))
        self.op.append("BT /%s %.1f Tf %.2f %.2f Td (%s) Tj ET" % (fuente, tam, x, y, s))
    def texto_centrado(self, cx, y, s, tam=8, fuente="F1"):
        an = len(s) * tam * (0.5 if fuente == "F1" else 0.6)
        self.texto(cx - an / 2, y, s, tam, fuente)
    def texto_der(self, xd, y, s, tam=8, fuente="F1"):
        an = len(s) * tam * (0.5 if fuente == "F1" else 0.6)
        self.texto(xd - an, y, s, tam, fuente)

    def fin_pagina(self):
        self.paginas.append("\n".join(self.op))
        self.op = []

    def guardar(self, ruta):
        objs, out = [], bytearray(b"%PDF-1.4\n")
        n_pag = len(self.paginas)
        # 1 catalogo, 2 pages, 3 F1, 4 F2, luego por pagina: page y contents
        fuentes = ("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
                   "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>")
        kids = " ".join("%d 0 R" % (5 + i * 2) for i in range(n_pag))
        cuerpos = [
            "<< /Type /Catalog /Pages 2 0 R >>",
            "<< /Type /Pages /Count %d /Kids [%s] >>" % (n_pag, kids),
            fuentes[0], fuentes[1],
        ]
        for i, contenido in enumerate(self.paginas):
            cuerpos.append(
                "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 %.2f %.2f] "
                "/Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> /Contents %d 0 R >>"
                % (A4_AN, A4_AL, 6 + i * 2))
            cuerpos.append(("stream", contenido))
        for i, c in enumerate(cuerpos):
            objs.append(len(out))
            if isinstance(c, tuple):
                datos = zlib.compress(c[1].encode("latin-1"))
                out += ("%d 0 obj\n<< /Length %d /Filter /FlateDecode >>\nstream\n"
                        % (i + 1, len(datos))).encode()
                out += datos + b"\nendstream\nendobj\n"
            else:
                out += ("%d 0 obj\n%s\nendobj\n" % (i + 1, c)).encode()
        inicio = len(out)
        out += ("xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1)).encode()
        for o in objs:
            out += ("%010d 00000 n \n" % o).encode()
        out += ("trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n"
                % (len(objs) + 1, inicio)).encode()
        open(ruta, "wb").write(bytes(out))


# ------------------------------------------------------------- las paginas

def encabezado(p, titulo, bajada):
    p.relleno(0.10, 0.10, 0.12)
    p.texto(40, A4_AL - 52, titulo, 17, "F2")
    p.relleno(0.45, 0.45, 0.48)
    p.texto(40, A4_AL - 68, bajada, 8.5)
    p.color(0.80, 0.80, 0.82); p.grosor(0.8)
    p.linea(40, A4_AL - 78, A4_AN - 40, A4_AL - 78)


def pagina_plano(p):
    encabezado(p, "Pico VGA Radar - como va cableado",
               "Verificado en la placa. Los colores agrupan cada señal: rojo, verde, azul, sincronismo, DDC y masa.")

    # --- la Pico ---
    px, py, pan = 62, 150, 96
    alto_pin, pines = 26.5, 20
    pal = alto_pin * pines
    p.color(0.20, 0.20, 0.24); p.grosor(1.2)
    p.rect_redondo(px, py, pan, pal, 5)
    p.relleno(0.20, 0.20, 0.24)
    p.texto_centrado(px + pan / 2, py + pal - 22, "Pico 2 W", 10, "F2")
    p.texto_centrado(px + pan / 2, py + pal - 34, "USB arriba", 6.5)
    # el USB
    p.color(0.55, 0.55, 0.60)
    p.rect(px + pan / 2 - 13, py + pal - 6, 26, 9)

    y_de = {}
    for lado, etiquetas in (("izq", PICO_IZQ), ("der", PICO_DER)):
        for i, et in enumerate(etiquetas):
            n = i + 1 if lado == "izq" else 40 - i
            y = py + pal - 46 - i * alto_pin
            x = px if lado == "izq" else px + pan
            usado = et in CONEX
            masa = (et == "GND")
            if lado == "izq":
                y_de[et if usado else (et + str(n))] = y
            # el agujero
            if usado:
                c = COLOR[GRUPO[CONEX[et][2]]]
            elif masa and lado == "izq" and n in (3, 8, 13):
                c = COLOR["masa"]
            else:
                c = (0.78, 0.78, 0.80)
            p.color(*c); p.relleno(*c); p.grosor(0.8)
            p.circulo(x + (-7 if lado == "izq" else 7), y, 3.4, lleno=usado or masa)
            # la etiqueta impresa, que es lo unico que hay que mirar
            p.relleno(*(c if usado else (0.55, 0.55, 0.58)))
            if lado == "izq":
                p.texto(px + 4, y - 2.6, et, 7.6, "F2" if usado else "F1")
                p.relleno(0.62, 0.62, 0.65)
                p.texto_der(px - 12, y - 2.2, str(n), 6)
            else:
                p.texto_der(px + pan - 4, y - 2.6, et, 7.6)
                p.relleno(0.62, 0.62, 0.65)
                p.texto(x + 12, y - 2.2, str(n), 6)

    p.relleno(0.55, 0.20, 0.20)
    p.texto_centrado(px + pan / 2, py - 16, "De este lado no sale ningun cable", 7, "F2")
    p.texto_centrado(px + pan / 2, py - 26, "VBUS, VSYS, 3V3 y 3V3_EN apagan la placa", 6.5)

    # --- el VGA ---
    vx, vy = A4_AN - 168, 300
    van, val = 118, 132
    p.color(0.20, 0.20, 0.24); p.grosor(1.2)
    p.rect_redondo(vx, vy, van, val, 6)
    p.relleno(0.20, 0.20, 0.24)
    p.texto_centrado(vx + van / 2, vy + val - 16, "VGA hembra", 9.5, "F2")
    p.texto_centrado(vx + van / 2, vy + val - 27, "visto de frente, soldando", 6.5)

    # tres filas: 1-5, 6-10, 11-15
    pos_vga = {}
    for fila, (desde, cant) in enumerate(((1, 5), (6, 5), (11, 5))):
        for k in range(cant):
            n = desde + k
            # la fila del medio va corrida, como en el conector de verdad
            corr = 9 if fila == 1 else 0
            x = vx + 20 + corr + k * 19.5
            y = vy + val - 46 - fila * 24
            pos_vga[n] = (x, y)
            if n in GRUPO:      c = COLOR[GRUPO[n]]
            elif n in MASAS:    c = COLOR["masa"]
            elif n == 9:        c = (0.85, 0.30, 0.30)
            else:               c = (0.80, 0.80, 0.82)
            p.color(*c); p.relleno(*c); p.grosor(0.9)
            lleno = (n in GRUPO) or (n in MASAS)
            p.circulo(x, y, 5.2, lleno=lleno)
            p.relleno(1, 1, 1) if lleno else p.relleno(*c)
            p.texto_centrado(x, y - 2.2, str(n), 6, "F2")

    p.relleno(0.85, 0.30, 0.30)
    p.texto_centrado(vx + van / 2, vy + 16, "El 9 (+5 V) NO se conecta", 7, "F2")
    p.relleno(0.45, 0.45, 0.48)
    p.texto_centrado(vx + van / 2, vy + 6, "Si se conecta, el monitor apagado", 6)
    p.texto_centrado(vx + van / 2, vy - 3, "sigue contestando el EDID", 6)

    # --- los cables ---
    for gp, (senal, res, agu) in CONEX.items():
        if gp not in y_de:
            continue
        y = y_de[gp]
        x1 = px + pan
        xr = 300
        c = COLOR[GRUPO[agu]]
        p.color(*c); p.grosor(1.1)
        p.linea(x1 + 14, y, xr - 16, y)
        if res == "directo":
            p.linea(xr - 16, y, xr + 34, y)
        else:
            # la resistencia
            p.relleno(1, 1, 1)
            p.rect(xr - 16, y - 4.6, 34, 9.2, lleno=True)
            p.color(*c)
            p.rect(xr - 16, y - 4.6, 34, 9.2)
            p.relleno(*c)
            p.texto_centrado(xr + 1, y - 2.2, res, 5.6, "F2")
        gx, gy = pos_vga[agu]
        p.color(*c); p.grosor(1.1)
        p.poli([(xr + 18 if res != "directo" else xr + 34, y), (xr + 46, y),
                (gx - 12, gy), (gx - 6.5, gy)])
        p.relleno(*c)
        p.texto(x1 + 18, y + 3.4, senal, 6)
        p.texto_der(gx - 14, gy + 4.2, "ag. %d" % agu, 6, "F2")

    # --- las masas ---
    ymasa = py + 26
    p.color(*COLOR["masa"]); p.grosor(1.3)
    p.linea(px + pan + 14, ymasa, 330, ymasa)
    p.relleno(*COLOR["masa"])
    p.texto(px + pan + 18, ymasa + 4, "Masa: de cualquier GND de la Pico", 6.5, "F2")
    p.texto(px + pan + 18, ymasa - 11, "salen CINCO cables, uno a cada agujero", 6)
    for n in MASAS:
        gx, gy = pos_vga[n]
        p.color(*COLOR["masa"]); p.grosor(1.0)
        p.poli([(330, ymasa), (352, ymasa), (gx - 12, gy), (gx - 6.5, gy)])

    # --- referencias ---
    ry = 118
    p.relleno(0.10, 0.10, 0.12)
    p.texto(40, ry, "Con una sola masa el monitor pierde el enganche cada pocos segundos: las cinco son obligatorias.", 7.5, "F2")
    p.relleno(0.55, 0.20, 0.20)
    p.texto(40, ry - 14, "Leer siempre la etiqueta impresa en la placa (GP0, GP1...). NUNCA contar pines: por contarlos se perdieron horas.", 7.5, "F2")
    p.relleno(0.45, 0.45, 0.48)
    p.texto(40, ry - 28, "Los numeros chicos al costado de la Pico son el pin fisico, y estan solo como referencia.", 7)
    p.texto(40, ry - 40, "Las resistencias de 1,95 k se arman con dos de 3,9 k en paralelo.", 7)
    p.fin_pagina()


def pagina_tabla(p):
    encabezado(p, "La tabla, cable por cable",
               "Lo mismo de la hoja anterior, para ir tachando mientras se arma.")

    y = A4_AL - 110
    cols = (44, 116, 210, 300, 372)
    p.relleno(0.35, 0.35, 0.38)
    for x, t in zip(cols, ("Pico", "Señal", "Resistencia", "Agujero VGA", "Que es ese agujero")):
        p.texto(x, y, t.upper(), 7, "F2")
    p.color(0.75, 0.75, 0.78); p.grosor(0.7)
    p.linea(40, y - 6, A4_AN - 40, y - 6)

    y -= 20
    for gp, (senal, res, agu) in CONEX.items():
        c = COLOR[GRUPO[agu]]
        p.color(*c); p.relleno(*c); p.grosor(0.8)
        p.circulo(46, y + 2.5, 3, lleno=True)
        p.relleno(0.12, 0.12, 0.15)
        p.texto(cols[0] + 10, y, gp, 8.5, "F2")
        p.relleno(0.30, 0.30, 0.34)
        p.texto(cols[1], y, senal, 8)
        p.texto(cols[2], y, res if res == "directo" else res + "ohm", 8)
        p.relleno(0.12, 0.12, 0.15)
        p.texto(cols[3], y, str(agu), 8.5, "F2")
        p.relleno(0.45, 0.45, 0.48)
        p.texto(cols[4], y, VGA_QUE[agu], 8)
        p.color(0.90, 0.90, 0.92); p.grosor(0.4)
        p.linea(40, y - 7, A4_AN - 40, y - 7)
        y -= 19

    # masa
    c = COLOR["masa"]
    p.color(*c); p.relleno(*c); p.grosor(0.8)
    p.circulo(46, y + 2.5, 3, lleno=True)
    p.relleno(0.12, 0.12, 0.15)
    p.texto(cols[0] + 10, y, "GND", 8.5, "F2")
    p.relleno(0.30, 0.30, 0.34)
    p.texto(cols[1], y, "Masa", 8)
    p.texto(cols[2], y, "directo", 8)
    p.relleno(0.12, 0.12, 0.15)
    p.texto(cols[3], y, "5, 6, 7, 8 y 10", 8.5, "F2")
    p.relleno(0.45, 0.45, 0.48)
    p.texto(cols[4], y, "las cinco masas del VGA", 8)
    y -= 34

    p.relleno(0.85, 0.30, 0.30)
    p.texto(44, y, "Agujero 9 (+5 V): NO se conecta.", 8.5, "F2")
    p.relleno(0.45, 0.45, 0.48)
    p.texto(210, y, "Sin el, el EDID solo contesta con el monitor prendido.", 8)
    y -= 30

    # que queda libre
    p.relleno(0.10, 0.10, 0.12)
    p.texto(40, y, "Que queda libre en la Pico", 11, "F2")
    y -= 16
    p.relleno(0.35, 0.35, 0.38)
    libres = [e for e in PICO_IZQ if e.startswith("GP") and e not in CONEX]
    p.texto(44, y, "De este lado: " + ", ".join(libres), 8)
    y -= 13
    p.texto(44, y, "Del otro lado: nada se toca. Ahi estan VBUS, VSYS, 3V3 y 3V3_EN, que apagan la placa.", 8)
    y -= 34

    p.relleno(0.10, 0.10, 0.12)
    p.texto(40, y, "Lo que costo horas y no hay que repetir", 11, "F2")
    y -= 18
    avisos = [
        "Leer la etiqueta impresa (GP0, GP1...), nunca contar pines. La placa montada al reves da la numeracion espejada.",
        "Las cinco masas del VGA son obligatorias: con una sola, el monitor pierde el enganche cada pocos segundos.",
        "Los ocho pines de color tienen que ser consecutivos, GP0 a GP7: el PIO los escribe de a ocho de una sola vez.",
        "Las resistencias del DDC no son opcionales: ese bus trabaja a 5 V y los pines de la Pico no toleran 5.",
        "Un cable micro-USB de solo carga alimenta la placa pero no sirve para cargarle el firmware.",
    ]
    for t in avisos:
        p.relleno(0.75, 0.55, 0.20)
        p.texto(44, y, "!", 9, "F2")
        p.relleno(0.30, 0.30, 0.34)
        p.texto(54, y, t, 7.6)
        y -= 15

    p.relleno(0.62, 0.62, 0.65)
    p.texto(40, 44, "Generado por herramientas/cableado_a_pdf.py - si se cambia un cable, cambiarlo ahi y en TRASPASO.md", 6.5)
    p.fin_pagina()


salida = sys.argv[1] if len(sys.argv) > 1 else "cableado.pdf"
p = Pdf()
pagina_plano(p)
pagina_tabla(p)
p.guardar(salida)
print("generado", salida)
