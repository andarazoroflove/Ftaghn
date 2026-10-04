OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradbigmips", "elf32-tradlittlemips")
OUTPUT_ARCH(mips)
ENTRY(WinMainCRTStartup)

SECTIONS
{
    /* Windows CE standard image base is 0x00010000; .text starts at RVA 0x1000 */
    . = 0x00011000;

    .text : {
        *(.text.WinMainCRTStartup)
        *(.text*)
        *(.rodata*)
        *(.rdata*)
        . = ALIGN(4);
    }

    /* Page align to 0x1000 for PE section mapping */
    . = ALIGN(0x1000);

    .data : {
        s_data_start = .;
        *(.data*)
        . = ALIGN(4);
        s_data_end = .;
    }

    . = ALIGN(0x1000);

    .bss : {
        s_bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        s_bss_end = .;
    }

    /DISCARD/ : {
        *(.reginfo)
        *(.MIPS.abiflags)
        *(.note.gnu.build-id)
        *(.comment)
    }
}
