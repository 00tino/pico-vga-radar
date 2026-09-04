# QR minimo: version 3 (29x29), correccion L, modo byte, mascara 0.
# Alcanza hasta 53 caracteres, suficiente para "http://192.168.4.1".
# No es una libreria general: hace una sola cosa y la hace chica.

_EXP = bytearray(512)
_LOG = bytearray(256)


def _init_gf():
    x = 1
    for i in range(255):
        _EXP[i] = x
        _LOG[x] = i
        x <<= 1
        if x & 0x100:
            x ^= 0x11D
    for i in range(255, 512):
        _EXP[i] = _EXP[i - 255]


_init_gf()


def _poly_mul(a, b):
    out = bytearray(len(a) + len(b) - 1)
    for i, av in enumerate(a):
        if not av:
            continue
        la = _LOG[av]
        for j, bv in enumerate(b):
            if bv:
                out[i + j] ^= _EXP[la + _LOG[bv]]
    return out


def _generator(n):
    g = bytearray([1])
    for i in range(n):
        g = _poly_mul(g, bytearray([1, _EXP[i]]))
    return g


def _ec_codewords(data, n):
    gen = _generator(n)
    rem = bytearray(len(data) + n)
    rem[: len(data)] = data
    for i in range(len(data)):
        c = rem[i]
        if not c:
            continue
        lc = _LOG[c]
        for j, gv in enumerate(gen):
            rem[i + j] ^= _EXP[lc + _LOG[gv]]
    return rem[len(data):]


SIZE = 29
_DATA_CW = 55          # version 3, nivel L
_EC_CW = 15
_ALIGN = (6, 22)       # centros del patron de alineacion en version 3
_FORMAT = 0b111011111000100   # nivel L + mascara 0, ya con su BCH


def _blank():
    return [bytearray(SIZE) for _ in range(SIZE)]


def _reserved():
    """Matriz de celdas ocupadas por patrones fijos (no llevan datos)."""
    res = _blank()

    def block(x, y, w, h):
        for j in range(y, y + h):
            if 0 <= j < SIZE:
                for i in range(x, x + w):
                    if 0 <= i < SIZE:
                        res[j][i] = 1

    # tres buscadores + su separador + la franja de formato
    block(0, 0, 9, 9)
    block(SIZE - 8, 0, 8, 9)
    block(0, SIZE - 8, 9, 8)
    # patron de alineacion
    block(_ALIGN[1] - 2, _ALIGN[1] - 2, 5, 5)
    # temporizadores
    for i in range(SIZE):
        res[6][i] = 1
        res[i][6] = 1
    return res


def _draw_static(m):
    def finder(ox, oy):
        for j in range(-1, 8):
            for i in range(-1, 8):
                x, y = ox + i, oy + j
                if not (0 <= x < SIZE and 0 <= y < SIZE):
                    continue
                edge = i in (0, 6) and 0 <= j <= 6
                edge = edge or (j in (0, 6) and 0 <= i <= 6)
                core = 2 <= i <= 4 and 2 <= j <= 4
                m[y][x] = 1 if (edge or core) else 0

    finder(0, 0)
    finder(SIZE - 7, 0)
    finder(0, SIZE - 7)

    for i in range(8, SIZE - 8):
        v = 1 if i % 2 == 0 else 0
        m[6][i] = v
        m[i][6] = v

    cx = cy = _ALIGN[1]
    for j in range(-2, 3):
        for i in range(-2, 3):
            edge = max(abs(i), abs(j)) != 1
            m[cy + j][cx + i] = 1 if edge else 0

    m[SIZE - 8][8] = 1   # modulo oscuro obligatorio

    # informacion de formato, duplicada en las dos posiciones que manda la norma
    bits = [(_FORMAT >> (14 - i)) & 1 for i in range(15)]
    for i in range(6):
        m[8][i] = bits[i]
    m[8][7] = bits[6]
    m[8][8] = bits[7]
    m[7][8] = bits[8]
    for i in range(9, 15):
        m[14 - i][8] = bits[i]
    for i in range(8):
        m[SIZE - 1 - i][8] = bits[i]
    for i in range(8, 15):
        m[8][SIZE - 15 + i] = bits[i]


def encode(text):
    """Devuelve una matriz SIZE x SIZE de 0/1. 1 = modulo oscuro."""
    raw = text.encode()
    if len(raw) > 53:
        raise ValueError("texto demasiado largo para QR v3")

    bits = []

    def push(value, n):
        for i in range(n - 1, -1, -1):
            bits.append((value >> i) & 1)

    push(0b0100, 4)          # modo byte
    push(len(raw), 8)        # longitud
    for b in raw:
        push(b, 8)
    push(0, min(4, _DATA_CW * 8 - len(bits)))   # terminador
    while len(bits) % 8:
        bits.append(0)

    data = bytearray()
    for i in range(0, len(bits), 8):
        byte = 0
        for b in bits[i:i + 8]:
            byte = (byte << 1) | b
        data.append(byte)
    pad = (0xEC, 0x11)
    k = 0
    while len(data) < _DATA_CW:
        data.append(pad[k & 1])
        k += 1

    full = data + _ec_codewords(data, _EC_CW)

    m = _blank()
    res = _reserved()
    _draw_static(m)

    # recorrido en zigzag de abajo a la derecha hacia arriba
    stream = []
    for byte in full:
        for i in range(7, -1, -1):
            stream.append((byte >> i) & 1)

    idx = 0
    col = SIZE - 1
    upward = True
    while col > 0:
        if col == 6:      # la columna del temporizador no cuenta
            col -= 1
        rows = range(SIZE - 1, -1, -1) if upward else range(SIZE)
        for row in rows:
            for c in (col, col - 1):
                if res[row][c]:
                    continue
                bit = stream[idx] if idx < len(stream) else 0
                idx += 1
                if (row + c) % 2 == 0:      # mascara 0
                    bit ^= 1
                m[row][c] = bit
        upward = not upward
        col -= 2
    return m
