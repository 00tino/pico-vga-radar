import network
import time
import ubinascii


def _chip_id():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    mac = ubinascii.hexlify(wlan.config("mac")).decode().upper()
    wlan.active(False)
    return mac[-4:]


def ap_name():
    return "RADAR-" + _chip_id()


AP_PASS = "radar1234"


def connect(ssid, password, timeout=20):
    """Intenta entrar a la red del cliente. Devuelve la IP o None."""
    if not ssid:
        return None
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if wlan.isconnected():
        return wlan.ifconfig()[0]
    wlan.connect(ssid, password or "")
    limit = time.ticks_add(time.ticks_ms(), timeout * 1000)
    while time.ticks_diff(limit, time.ticks_ms()) > 0:
        if wlan.isconnected():
            return wlan.ifconfig()[0]
        if wlan.status() < 0:
            break
        time.sleep_ms(250)
    wlan.active(False)
    return None


def start_ap():
    """Levanta la red propia del equipo. Devuelve (nombre, clave, ip)."""
    name = ap_name()
    ap = network.WLAN(network.AP_IF)
    ap.active(False)
    time.sleep_ms(200)
    ap.config(essid=name, password=AP_PASS)
    ap.active(True)
    for _ in range(40):
        if ap.active():
            break
        time.sleep_ms(100)
    return name, AP_PASS, ap.ifconfig()[0]
