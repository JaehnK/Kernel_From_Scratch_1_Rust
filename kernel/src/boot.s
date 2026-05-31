.set MAGIC,    0x1BADB002
.set FLAGS,    (1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .text
.align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
