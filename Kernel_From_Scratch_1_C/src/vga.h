#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void        put_char(uint8_t c);
void        put_chars(char *str);
void        set_charcolor(uint8_t color);
void        set_backgroundcolor(uint8_t color);
void        set_mix(uint8_t color_mix);
void        reset_screen();

#endif