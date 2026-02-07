/*
 * Linker script for FreeRTOS on Intel 8085 (flat 64K RAM for simulator)
 *
 * Memory map:
 *   0x0000-0x003F  Interrupt vector table
 *   0x0040-...     Code (.text), read-only data (.rodata)
 *   ...-...        Initialized data (.data), BSS (.bss)
 *   ...            Task stacks (allocated in BSS)
 *   0xFB00-0xFBFF  ISR stack (256 bytes, grows down from 0xFC00)
 *   0xFC00-0xFDFF  Main/startup stack (512 bytes, grows down from 0xFE00)
 *   0xFE00-0xFEFF  Marker / test dump area
 */

OUTPUT_FORMAT(elf32-i8085)
ENTRY(_start)

SECTIONS
{
    /* Interrupt vector table at address 0 */
    .vectors 0x0000 : {
        KEEP(*(.vectors))
    }

    /* Code starts after vectors */
    . = 0x0040;

    .text : {
        *(.text.startup)
        *(.text .text.*)
        *(.init .fini)
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

    /* For flat RAM config, _sidata == _sdata (no copy needed) */
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

    /* Verify no overlap with ISR stack area */
    ASSERT(_end <= 0xFB00, "Sections overflow into ISR stack area")

    /* Test/marker area for simulator verification */
    . = 0xFE00;
    .testdump (NOLOAD) : {
        *(.testdump)
    }

    /* Discard unneeded sections */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.llvm_addrsig)
    }
}

/* ISR stack: 256 bytes from 0xFB00 to 0xFC00 */
PROVIDE(_isr_stack_top = 0xFC00);

/* Main stack: startup and idle task, 512 bytes from 0xFC00 to 0xFE00 */
PROVIDE(_stack = 0xFE00);
PROVIDE(_estack = 0xFE00);

/* Data section sizes (for possible future ROM->RAM copy) */
PROVIDE(_data_size = _edata - _sdata);
PROVIDE(_bss_size = _ebss - _sbss);
