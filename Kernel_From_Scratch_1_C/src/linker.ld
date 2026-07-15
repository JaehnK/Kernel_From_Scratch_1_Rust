OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS {
    . = 1M;
    _kernel_start = .;

    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot))
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    _kernel_end = .;

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note.*)
        *(.note.GNU-stack)
        *(.comment)
    }
}
