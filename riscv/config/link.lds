/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

RISCV_BASE_ADDR = 0x47C00000;
RISCV_LEN = 2 * 1024 * 1024;
__STACKSIZE__ = 4096;

/*
 * The OUTPUT_ARCH command specifies the machine architecture where the
 * argument is one of the names used in the Kendryte library.
 */
OUTPUT_ARCH( "riscv" )
OUTPUT_FORMAT("elf32-littleriscv")

MEMORY
{
  /* DDR RAM */
  SRAM : ORIGIN = RISCV_BASE_ADDR, LENGTH = RISCV_LEN
}

PHDRS
{
    sbi     PT_LOAD FLAGS(5); /* PF_R|PF_X */
    boot    PT_LOAD FLAGS(5); /* PF_R|PF_X */
    text    PT_LOAD FLAGS(5); /* PF_R|PF_X */
    rodata  PT_LOAD FLAGS(4); /* PF_R */
    data    PT_LOAD FLAGS(6); /* PF_R|PF_W */
    note    PT_NOTE FLAGS(4); /* PF_R */
    debug   PT_NOTE FLAGS(0); /* PF_R */
}

PROVIDE (Stack_Size = 0x1000);

ENTRY(_start)
SECTIONS
{
    . = RISCV_BASE_ADDR;

    /* __STACKSIZE__ = 4096; */

    .start :
    {
        . = ALIGN(8);
        *(.start);
        . = ALIGN(8);
    } > SRAM :boot =0x9002

    . = ALIGN(8);

    .text :
    {
        *(.text)                        /* remaining code */
        *(.text.*)                      /* remaining code */
        *(.eh_frame_hdr)
        *(.eh_frame_entry)
        *(.rodata)                      /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t*)

        /* section information for finsh shell */
        . = ALIGN(8);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(8);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        . = ALIGN(8);

        /* section information for initial. */
        . = ALIGN(8);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        . = ALIGN(8);

        __rt_utest_tc_tab_start = .;
        KEEP(*(UtestTcTab))
        __rt_utest_tc_tab_end = .;

        . = ALIGN(8);
        _etext = .;
    } > SRAM :text =0x9002
    . = ALIGN(8);

    .data :
    {
        *(.data)
        *(.data.*)

        *(.data1)
        *(.data1.*)

        . = ALIGN(8);
        PROVIDE( __global_pointer$ = . + 0x800 );

        *(.sdata)
        *(.sdata.*)
    } > SRAM :data =0x9002

    /* stack for dual core */
    .stack :
    {
        . = ALIGN(64);
        __stack_start__ = .;

        . += __STACKSIZE__;
        __stack_cpu0 = .;

    } > SRAM :data

    .resource_table ADDR(.stack) + SIZEOF(.stack): AT(LOADADDR(.stack) + SIZEOF(.stack))
    {
        KEEP(*(.vbuffer))
    } > SRAM :data

    .sbss :
    {
        __bss_start = .;
        __bss_start__ = .;
        *(.sbss)
        *(.sbss.*)
        *(.dynsbss)
        *(.scommon)
    } > SRAM :data

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.dynbss)
        *(COMMON)
        __bss_end = .;
        __bss_end__ = .;
    } > SRAM :data

    _end = .;

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
