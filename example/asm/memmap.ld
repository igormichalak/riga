OUTPUT_FORMAT(elf64-littleriscv)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1M
    RAM   (rwx): ORIGIN = 0x00100000, LENGTH = 1M
}

PHDRS
{
    text PT_LOAD FLAGS(5); /* PF_R | PF_X */
    bss  PT_LOAD FLAGS(6); /* PF_R | PF_W */
}

ENTRY(_entry_point)

SECTIONS
{
    .text : {
        . = ALIGN(4);
        *(.text)
    } > FLASH AT> FLASH :text

    .rodata : {
        . = ALIGN(4);
        *(.rodata)
    } > FLASH AT> FLASH :text

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM AT> RAM :bss
}
