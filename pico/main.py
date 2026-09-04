# Arranque del radar. Se ejecuta solo al enchufar el equipo.
#
#   1. Prende el video y muestra que esta iniciando.
#   2. Si hay una red guardada, se conecta.
#        - No hay red guardada o falla  -> crea su propia red y muestra el QR.
#        - Conecta                      -> muestra el QR de configuracion un rato
#                                          y despues empieza a pintar trafico.
#   3. El servidor sigue vivo: si el cliente reconfigura, cambia en el momento.

import gc
import machine
import time

import portal
import render
import sky
import store
import ui

SETUP_QR_S = 25          # cuanto queda el QR en pantalla al conectar
CFG = None
PENDING = None


def _on_wifi(ssid, password):
    cfg = store.load_config()
    cfg["wifi"] = {"ssid": ssid, "pass": password}
    store.save_config(cfg)


def _on_config(q):
    global CFG, PENDING
    CFG = store.apply_query(CFG, q)
    store.save_config(CFG)
    PENDING = True


def run():
    global CFG, PENDING
    ui.screen_boot("INICIANDO RADAR")
    CFG = store.load_config()
    gc.collect()

    import wifi
    creds = CFG.get("wifi") or {}
    ip = wifi.connect(creds.get("ssid", ""), creds.get("pass", ""))

    if not ip:
        name, password, ip = wifi.start_ap()
        srv = portal.Portal(ip)
        ui.screen_ap(name, password, "http://" + ip)
        while True:
            if srv.poll(_on_wifi, _on_config, True) == "wifi":
                ui.screen_boot("REINICIANDO...")
                time.sleep(2)
                machine.reset()
            time.sleep_ms(50)

    srv = portal.Portal(ip)
    ui.screen_setup("http://" + ip, ip)
    until = time.ticks_add(time.ticks_ms(), SETUP_QR_S * 1000)
    while time.ticks_diff(until, time.ticks_ms()) > 0 and not PENDING:
        srv.poll(_on_wifi, _on_config, False)
        time.sleep_ms(50)

    PENDING = False
    ui.screen_boot("BUSCANDO VUELOS...")
    data = []
    page = 0
    last_poll = 0
    last_page = time.ticks_ms()

    while True:
        now = time.ticks_ms()

        if last_poll == 0 or time.ticks_diff(now, last_poll) > CFG["poll_s"] * 1000:
            last_poll = now
            got = sky.fetch(CFG["lat"], CFG["lon"], CFG["radius_km"])
            if got is not None:
                data = render.filtered(got, CFG)
            gc.collect()

        if time.ticks_diff(now, last_page) > CFG["rotate_s"] * 1000:
            last_page = now
            page += 1

        render.draw(CFG, data, page)

        # Se sigue atendiendo al celular mientras el radar corre.
        for _ in range(20):
            if srv.poll(_on_wifi, _on_config, False) == "config":
                last_poll = 0
                page = 0
            time.sleep_ms(25)


try:
    run()
except Exception as e:
    try:
        ui.screen_error("ERROR", str(e)[:60])
    except Exception:
        pass
    time.sleep(20)
    machine.reset()
