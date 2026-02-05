/* 32K RAM @ 0x0000, 32K ROM @ 0x8000. Vector table at 0x0000.
   Reserve input window at 0x0100-0x012F (48 bytes) for benchmark data. */
OUTPUT_FORMAT(elf32-i8085)
ENTRY(_start)

MEMORY
{
  VECTORS (rx)  : ORIGIN = 0x0000, LENGTH = 0x0040
  RAM0    (rwx) : ORIGIN = 0x0040, LENGTH = 0x00C0
  INPUT   (rw)  : ORIGIN = 0x0100, LENGTH = 0x0030
  RAM1    (rwx) : ORIGIN = 0x0130, LENGTH = 0x7ED0
  ROM     (rx)  : ORIGIN = 0x8000, LENGTH = 0x8000
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

  .input : {
    KEEP(*(.input))
  } > INPUT

  . = ORIGIN(RAM1);

  .data : AT (_etext) {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(2);
    _edata = .;
  } > RAM1

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : {
    . = ALIGN(2);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    _ebss = .;
  } > RAM1

  _end = .;
}

PROVIDE(_data_size = _edata - _sdata);
PROVIDE(_bss_size = _ebss - _sbss);

PROVIDE(_estack = ORIGIN(RAM1) + LENGTH(RAM1));
PROVIDE(_stack = _estack);
