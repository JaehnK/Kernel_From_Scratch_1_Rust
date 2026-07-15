#include <stddef.h>
#include <stdint.h>
#include "./string.h"

//vga range is 80(w:row) * 25(h:column)  * 2(char:attribute) = 4000
//attribute format is 1byte front 4bit is background color , back 4bit is char color

//logic we cahange vga text mode -> parsing vga controll mode view
uint16_t VGA_ROW = 0;
uint16_t VGA_COL = 0;
// uint16_t VGA_INDEX = 0;

uint8_t ATTRIBUTE = 0x0f;
uint8_t BACKGROUND = 0x0;
uint8_t CHARCOLOR = 0xf;

#define VGA_BUFFER ((volatile uint8_t *)0xB8000u)
// #define VGA_ATTR_WHITE_ON_BLACK 0x0Fu


/*
jinseo code
void        put_char(uint8_t c)
{
    // 10 % 80 = 10 10 / 25 = 0 
    // 100 % 80 = 20 100 /80 = 1
    //nor make col max
    uint16_t VGA_INDEX = 0;
    if (VGA_ROW == 80 || c == '\n'){
        VGA_COL += 1;
        VGA_ROW = 0;
    }
    if (VGA_COL == 25){
        VGA_COL = 0;
    }
    VGA_INDEX = VGA_ROW + (VGA_COL * 80);
    VGA_BUFFER[VGA_INDEX * 2] = c;
    VGA_BUFFER[VGA_INDEX * 2 + 1] = ATTRIBUTE;
    if (c == 0x08) //'\b' backspace ;; not clear backspace mode
    {
        if (VGA_ROW != 0 && VGA_COL != 0)
        {
            VGA_ROW -= 1;
        }
    }
    else
    {
        VGA_ROW += 1;
    }
}
*/

void put_char(uint8_t c)//AI code
{
    uint16_t VGA_INDEX = 0;

    if (c == '\n')
    {
        VGA_COL += 1;
        VGA_ROW = 0;
        if (VGA_COL == 25)
            VGA_COL = 0;
        return;
    }

    if (VGA_ROW == 80)
    {
        VGA_COL += 1;
        VGA_ROW = 0;
    }

    if (VGA_COL == 25)
        VGA_COL = 0;

    VGA_INDEX = VGA_ROW + (VGA_COL * 80);
    VGA_BUFFER[VGA_INDEX * 2] = c;
    VGA_BUFFER[VGA_INDEX * 2 + 1] = ATTRIBUTE;

    VGA_ROW += 1;
}

void        put_chars(char *str)
{
    size_t len = ft_strlen(str);
    for (size_t index = 0; index < len; index++)
    {
        put_char(str[index]);
    }
}

void        reset_screen()
{
    VGA_ROW = 0;
    VGA_COL = 0;
    for (size_t col = 0; col < 25; col++)
    {
        for (size_t row = 0; row < 80; row++) {
            put_char(' ');
        }
    }
    VGA_ROW = 0;
    VGA_COL = 0;
}

void        set_charcolor(uint8_t color)
{
    CHARCOLOR = color;
    ATTRIBUTE &= 0xf0;
    ATTRIBUTE |= CHARCOLOR & 0x0f;
}

void        set_backgroundcolor(uint8_t color)
{
    BACKGROUND = color << 4;
    ATTRIBUTE &= 0x0f;
    ATTRIBUTE |= BACKGROUND;
}

void set_mix(uint8_t color_mix)
{
    ATTRIBUTE = color_mix;
}