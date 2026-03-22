/*
 * NUCLEO-H743ZI + LwIP — same layout as lwip_zero (RAM_D1 for .data/.bss/stack).
 * lwip_zero works; this matches its memory map.
 */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

_estack = 0x24000000 + 512K;
_Min_Heap_Size = 0x400;
_Min_Stack_Size = 0x800;

MEMORY
{
  ROM (rx)       : ORIGIN = 0x08000000, LENGTH = 2048K
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)   : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 288K
}

SECTIONS
{
  .isr_vector : {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > ROM

  .text : {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } > ROM

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > ROM
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > ROM

  .preinit_array : {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > ROM
  .init_array : {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > ROM
  .fini_array : {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > ROM

  _sidata = LOADADDR(.data);
  /* Bounds of programmed application in Flash (vector + code/rodata + .data init LMA) — fw_integrity.c */
  PROVIDE(__app_flash_start = ORIGIN(ROM));
  .data : {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } > RAM_D1 AT > ROM

  PROVIDE(__app_flash_end = ALIGN(LOADADDR(.data) + SIZEOF(.data), 4));

  . = ALIGN(4);
  .bss : {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
    _end = .;
    end = .;
  } > RAM_D1

  ._user_heap_stack : {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM_D1

  .lwip_sec (NOLOAD) : {
    . = ABSOLUTE(0x30000000);
    *(.RxDescripSection)
    . = ABSOLUTE(0x30000080);
    *(.TxDescripSection)
    . = ABSOLUTE(0x30000100);
    *(.Rx_PoolSection)
  } > RAM_D2

  .lwip_ram_heap 0x30004000 (NOLOAD) : {
    _lwip_ram_heap = .;
    . += 0x4000;
  } > RAM_D2

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
