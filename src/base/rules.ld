ENTRY(ResetISR)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.after_vectors*)
        *(.text*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
    {
        _data = .;
        *(vtable)
        *(.data*)
        . = ALIGN(4) ;
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > RAM

    PROVIDE(_pvHeapStart = .);
    PROVIDE(_vStackTop = ORIGIN(RAM) + LENGTH(RAM));
}
