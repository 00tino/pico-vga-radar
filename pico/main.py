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
import sys
import time


def log(msg):
    """Deja rastro de cada paso del arranque en boot.log, para poder leerlo
    despues aunque no haya nadie mirando el puerto serie."""
    try:
        with open("boot.log", "a") as f:
            f.write(str(msg) + "\n")
    except Exception:
        pass
    print(msg)


try:
    with open("boot.log", "w") as f:
        f.write("--- arranque ---\n")
except Exception:
    pass

log("0. arrancando, importando modulos")
try:
    import portal
    log("   portal ok")
    import store
    log("   store ok")
    import ui
    log("   ui ok")
    import render
    log("   render ok")
    import sky
    log("   sky ok")
except Exception as _e:
    try:
        with open("boot.log", "a") as f:
            f.write("FALLO IMPORTANDO:\n")
            sys.print_exception(_e, f)
    except Exception:
        pass
    sys.print_exception(_e)
    raise

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
    log("1. dibujando pantalla de inicio")
    ui.screen_boot("INICIANDO RADAR")
    log("2. leyendo config")
    CFG = store.load_config()
    gc.collect()

    log("3. importando wifi")
    import wifi
    creds = CFG.get("wifi") or {}
    log("4. intentando conectar a: " + repr(creds.get("ssid", "")))
    ip = wifi.connect(creds.get("ssid", ""), creds.get("pass", ""))
    log("   resultado: " + repr(ip))

    if not ip:
        log("5. levantando punto de acceso")
        name, password, ip = wifi.start_ap()
        log("   AP: " + name + " en " + ip)
        gc.collect()
        log("6. abriendo servidor web. RAM: " + str(gc.mem_free()))
        srv = portal.Portal(ip)
        log("7. dibujando pantalla del QR")
        ui.screen_ap(name, password, "http://" + ip)
        try:
            from machine import mem32
            sc = ui.screen()
            buf = sc.H_buffer_line
            nz = 0
            tot = 0
            for i in range(0, len(buf), 53):
                tot += 1
                if buf[i]:
                    nz += 1
            log("   framebuffer: %d de %d palabras con contenido" % (nz, tot))
            log("   direccion del buffer: %x" % sc.H_buffer_line_address[0])
            log("   PIO1 CTRL: %08x  (los 3 bits bajos = SM 0,1,2 andando)" % mem32[0x50300000])
            log("   DMA0 CTRL: %08x" % mem32[0x50000010])
            log("   DMA1 CTRL: %08x" % mem32[0x5000004c])
            log("   DMA1 transferencias restantes: %d" % mem32[0x50000048])
        except Exception as _d:
            log("   fallo el diagnostico: " + str(_d))
        log("8. LISTO, esperando al celular")
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
        with open("boot.log", "a") as f:
            f.write("EXCEPCION:\n")
            sys.print_exception(e, f)
    except Exception:
        pass
    sys.print_exception(e)
    try:
        ui.screen_error("ERROR", str(e)[:60])
    except Exception:
        pass
    time.sleep(30)
