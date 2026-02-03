/* Flat 64K map for simulator tests with a higher testdump address. */
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

  . = ALIGN(2);
  _etext = .;

  .data : {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(2);
    _edata = .;
  }

  _sidata = _etext;

  .bss (NOLOAD) : {
    . = ALIGN(2);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    _ebss = .;
  }

  _end = .;

  ASSERT(_end <= 0xFF40, "sim-flat-hi: sections overlap .testdump")

  . = 0xFF40;
  .testdump (NOLOAD) : {
    *(.testdump)
  }
}

PROVIDE(_data_size = _edata - _sdata);
PROVIDE(_bss_size = _ebss - _sbss);
PROVIDE(_estack = 0xFE00);
PROVIDE(_stack = _estack);
