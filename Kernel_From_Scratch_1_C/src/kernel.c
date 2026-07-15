#include <stdint.h>
#include "./vga.h"
#include "./debug.h"

// #define VGA_BUFFER ((volatile uint8_t *)0xB8000u)
// #define VGA_ATTR_WHITE_ON_BLACK 0x0Fu

// __attribute__((noreturn))
// void kernel_main(void)
// {
//     VGA_BUFFER[0] = '4';
//     VGA_BUFFER[1] = VGA_ATTR_WHITE_ON_BLACK;
//     VGA_BUFFER[2] = '2';
//     VGA_BUFFER[3] = VGA_ATTR_WHITE_ON_BLACK;

//     __asm__ volatile ("cli");
//     for (;;) {
//         __asm__ volatile ("hlt");
//     }
// }

#define VGA_ATTR_WHITE_ON_BLACK 0x0Fu

__attribute__((noreturn))
void kernel_main(void)
{
    set_mix(VGA_ATTR_WHITE_ON_BLACK);
    put_chars("42 and Hello World");
    key_debug_loop();

    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
