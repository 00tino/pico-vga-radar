cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(radar C CXX ASM)
set(PICO_BOARD pico2_w)
pico_sdk_init()

add_executable(radar main.c volcado.c fuente_grande.c monitor.c arranques.c pantallas.c vga.c gfx.c fuente.c area.c logos.c trig.c geo.c pistas.c aeropuertos.c radar.c viaje.c logos_pantalla.c costas.c tarjetas.c demo.c sky.c vivo.c aerolineas.c config.c sky_parse.c qr.c portal.c portal_parse.c ap.c)
pico_generate_pio_header(radar ${CMAKE_CURRENT_LIST_DIR}/vga.pio)
# lwipopts.h y mbedtls_config.h estan aca al lado.
target_include_directories(radar PRIVATE ${CMAKE_CURRENT_LIST_DIR})
# El SDK ya apunta MBEDTLS_CONFIG_FILE a "mbedtls_config.h", que es el que
# esta aca al lado: volver a definirlo solo llena la compilacion de avisos.

target_link_libraries(radar
    pico_stdlib
    pico_multicore
    # La radio con lwIP atendido por interrupcion. Se probo primero en modo
    # poll, para no meterle interrupciones al video, y no sirve: en ese modo
    # el chip solo se atiende cuando uno lo pide, y los comandos internos del
    # driver (los que hacen falta para asociarse a una red) se quedan sin
    # respuesta y dan timeout. Es seguro para el video porque la interrupcion
    # del VGA corre en prioridad maxima: puede interrumpir a la de la radio,
    # nunca al reves. Ver sky.c.
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    hardware_pio
    hardware_dma
    hardware_clocks
    hardware_i2c
    hardware_flash
    # Para poder escribir la flash con el nucleo 1 andando. Ver arranques.c.
    pico_flash
)
# Consola por USB, para poder ver que hace el firmware mientras se depura.
# La pila del nucleo 1 vive en SCRATCH_X y son cuatro kilobytes justos: no se
# puede agrandar. Por eso lo que ocupa lugar ahi (el armado de cada lote) se
# saca de la pila y se guarda aparte. Ver digerir() en sky.c.
pico_enable_stdio_usb(radar 1)
pico_enable_stdio_uart(radar 0)

# Firmware de prueba: solo la radio, sin video. Sirve para separar un problema
# de la radio de uno de convivencia con el video. Ver prueba_wifi.c.
add_executable(prueba_wifi prueba_wifi.c config.c)
target_include_directories(prueba_wifi PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(prueba_wifi
    pico_stdlib
    pico_flash
    hardware_flash
    pico_cyw43_arch_lwip_threadsafe_background
    # lwipopts.h es el mismo para los dos y pide TLS, asi que tambien aca.
    pico_lwip_mbedtls
    pico_mbedtls
)
pico_enable_stdio_usb(prueba_wifi 1)
pico_enable_stdio_uart(prueba_wifi 0)
pico_add_extra_outputs(prueba_wifi)

# La misma prueba pero con el reloj bajo, como lo deja el video.
add_executable(prueba_wifi_lento prueba_wifi.c config.c)
target_compile_definitions(prueba_wifi_lento PRIVATE PRUEBA_RELOJ_LENTO=1)
target_include_directories(prueba_wifi_lento PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(prueba_wifi_lento
    pico_stdlib
    pico_flash
    hardware_flash
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
)
pico_enable_stdio_usb(prueba_wifi_lento 1)
pico_enable_stdio_uart(prueba_wifi_lento 0)
pico_add_extra_outputs(prueba_wifi_lento)

pico_add_extra_outputs(radar)
