.set MULTIBOOT_MAGIC,    0x1BADB002
.set MULTIBOOT_FLAGS,    (1 << 1)
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.section .multiboot, "a"
.align 4
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long MULTIBOOT_CHECKSUM

.section .text, "ax"
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
.size _start, . - _start

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384
stack_top:
