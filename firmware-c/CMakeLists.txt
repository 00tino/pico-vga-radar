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
    # La radio con lwIP en modo poll: nada de interrupciones asincronicas que
    # le roben tiempo al video. Ver sky.c.
    pico_cyw43_arch_lwip_poll
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
pico_enable_stdio_usb(radar 1)
pico_enable_stdio_uart(radar 0)

pico_add_extra_outputs(radar)
