/* 16K RAM @ 0x0000, 48K ROM @ 0x4000. Vector table at 0x0000. */
OUTPUT_FORMAT(elf32-i8085)
ENTRY(_start)

MEMORY
{
  VECTORS (rx)  : ORIGIN = 0x0000, LENGTH = 0x0040
  RAM     (rwx) : ORIGIN = 0x0040, LENGTH = 0x3FC0
  ROM     (rx)  : ORIGIN = 0x4000, LENGTH = 0xC000
}

SECTIONS
{
  .vectors : {
    KEEP(*(.vectors))
  } > VECTORS

  .text : {
    *(.text .text.* .init .fini)
  } > ROM

  .rodata : {
    *(.rodata .rodata.*)
  } > ROM

  . = ALIGN(2);
  _etext = .;

  . = ORIGIN(RAM);

  .data : AT (_etext) {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(2);
    _edata = .;
  } > RAM

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : {
    . = ALIGN(2);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    _ebss = .;
  } > RAM

  _end = .;
}

PROVIDE(_data_size = _edata - _sdata);
PROVIDE(_bss_size = _ebss - _sbss);

PROVIDE(_estack = ORIGIN(RAM) + LENGTH(RAM));
PROVIDE(_stack = _estack);
