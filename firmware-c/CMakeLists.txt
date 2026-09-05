cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(radar C CXX ASM)
set(PICO_BOARD pico2_w)
pico_sdk_init()

add_executable(radar main.c volcado.c fuente_grande.c monitor.c arranques.c pantallas.c vga.c gfx.c fuente.c area.c logos.c trig.c geo.c pistas.c aeropuertos.c radar.c viaje.c logos_pantalla.c costas.c tarjetas.c demo.c)
pico_generate_pio_header(radar ${CMAKE_CURRENT_LIST_DIR}/vga.pio)
target_link_libraries(radar
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_clocks
    hardware_i2c
    hardware_flash
)
# Consola por USB, para poder ver que hace el firmware mientras se depura.
pico_enable_stdio_usb(radar 1)
pico_enable_stdio_uart(radar 0)

pico_add_extra_outputs(radar)
