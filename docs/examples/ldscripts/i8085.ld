/* Default linker script for i8085 bare-metal targets. */
OUTPUT_FORMAT(elf32-i8085)
ENTRY(_start)

SECTIONS
{
  .text : {
    *(.text .text.* .init .fini)
  }

  .rodata : {
    *(.rodata .rodata.*)
  }

  . = ALIGN(2);
  _etext = .;

  .data : AT (_etext) {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(2);
    _edata = .;
  }

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : {
    . = ALIGN(2);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(2);
    _ebss = .;
  }

  _end = .;
}

PROVIDE(_estack = 0xFFFF);
PROVIDE(_stack = _estack);
