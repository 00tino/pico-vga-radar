# PICO2-VGA-Micropython
A simple 800x600 60Hz VGA display driver to use with Pico2 and Micropython, 3 bits per color -> 8 colors

Hello All,

This is an upgrade of my [previous](https://github.com/HughMaingauche/PICO-VGA-Micropython) VGA micropython display library, using the pico2 allows to use a VGA screen with a 800x600 resolution and 3 bits per pixel (8 colors).
This library uses 3 PIO statemachines and 2 DMA channels
A full framebuffer and a font, once loaded in memory, will leave approx 270k of available memory to do stuff and use a VGA monitor directly connected to the PICO

Other improvements are:
- Use of the DMA included library available in the standard micropython instead of direct registry modification used in my previous code
- Changed the font system to use xglcd fonts (from [rdagger library](https://github.com/rdagger/micropython-ili9341/tree/master))
- Added a plotting library to make real time plotting more easy

I tried to document the code as much as I could. I have no doubt that this can be optimized quite a lot.
If any of you is interested in using or improving it, i'll be happy to provide further explanations.

Pinout for the Pico2 <-> VGA:

Red is on GPIO18    -> using 220 ohm resistor

Green is on GPIO19  -> using 220 ohm resistor

Blue is on GPIO20   -> using 220 ohm resistor

Hsync signal is on GPIO22 -> using 47 ohm resistor

Vsync signal is on GPIO21 -> using 47 ohm resistor


![PICO2VGA00](https://github.com/user-attachments/assets/4900904c-67bb-44c4-8c3d-bf6f33d14ef3)
![PICO2VGA01](https://github.com/user-attachments/assets/9d0ca490-b1ac-4007-b455-6be7ada3bcc3)
