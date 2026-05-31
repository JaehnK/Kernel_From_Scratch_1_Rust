ENTRY(_start)

SECTIONS {
    . = 1M;

    .text :
    {
        KEEP(src/boot.o(.text))
        *(.text)
        *(.text.*)
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.debug_*)
        *(.note.*)
        *(.comment)
    }
}
