/* Linker script for GCC torture tests on i8085 simulator.
 * Flat 64K memory map with heap support.
 * Status word at 0xFE00 for pass/fail reporting.
 */
OUTPUT_FORMAT(elf32-i8085)
ENTRY(_start)

SECTIONS
{
  . = 0x0000;
  .vectors : {
    KEEP(*(.vectors))
  }

  . = 0x0040;

  .text : {
    *(.text .text.* .init .fini)
  }

  .rodata : {
    *(.rodata .rodata.*)
  }

  /* C++ global constructors/destructors */
  .init_array : {
    . = ALIGN(2);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  }

  . = ALIGN(2);
  _etext = .;

  .data : {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(2);
    _edata = .;
  }

  /* Flat memory model: data initializers are already at _sdata.
   * Set _sidata = _sdata so the CRT0 copy loop is a no-op.
   * (_etext may differ from _sdata due to alignment padding.) */
  _sidata = _sdata;

  .bss (NOLOAD) : {
    . = ALIGN(2);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    _ebss = .;
  }

  _end = .;

  ASSERT(_end <= 0xF800, "torture: sections overflow available RAM")

  . = 0xFE00;
  .testdump (NOLOAD) : {
    *(.testdump)
  }
}

PROVIDE(_data_size = _edata - _sdata);
PROVIDE(_bss_size = _ebss - _sbss);
PROVIDE(_estack = 0xFE00);
PROVIDE(_stack = _estack);

/* Heap for tests that use malloc */
PROVIDE(__heap_start = _end);
PROVIDE(__heap_end = _estack - 0x0200);
