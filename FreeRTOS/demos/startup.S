; ---------------------------------------------------------------------------
; Intel 8085 FreeRTOS startup code
;
; Memory map (flat 64K for simulator):
;   0x0000-0x003F  Interrupt vector table
;   0x0040+        Code (.text)
;   ...            Data, BSS, task stacks
;   0xFCFF         ISR stack top (64 bytes: 0xFCC0-0xFCFF)
;   0xFE00         Main stack top (grows down from here)
;
; 8085 interrupt vectors:
;   0x0000  RST 0 (reset)
;   0x0008  RST 1
;   0x0010  RST 2
;   0x0018  RST 3
;   0x0020  RST 4
;   0x0024  TRAP (non-maskable)
;   0x0028  RST 5
;   0x002C  RST 5.5
;   0x0030  RST 6
;   0x0034  RST 6.5 -- used for tick timer interrupt
;   0x0038  RST 7
;   0x003C  RST 7.5
; ---------------------------------------------------------------------------

    .section .vectors, "ax"
    .globl __vector_table
__vector_table:
    .org 0x0000
    JMP _start              ; RST 0 (reset)

    .org 0x0008
    JMP _unhandled_irq      ; RST 1

    .org 0x0010
    JMP _unhandled_irq      ; RST 2

    .org 0x0018
    JMP _unhandled_irq      ; RST 3

    .org 0x0020
    JMP _unhandled_irq      ; RST 4

    .org 0x0024
    JMP _unhandled_irq      ; TRAP

    .org 0x0028
    JMP _unhandled_irq      ; RST 5

    .org 0x002C
    JMP _unhandled_irq      ; RST 5.5

    .org 0x0030
    JMP _unhandled_irq      ; RST 6

    .org 0x0034
    JMP vPortTickISR         ; RST 6.5 -- tick timer interrupt

    .org 0x0038
    JMP _unhandled_irq      ; RST 7

    .org 0x003C
    JMP _unhandled_irq      ; RST 7.5

; ---------------------------------------------------------------------------
    .section .text.startup, "ax"

    .globl _start
_start:
    ; Set up the main stack pointer (used during startup and by idle task)
    LXI SP, _stack

    ; Disable interrupts during initialization
    DI

    ; Unmask RST 6.5 via SIM instruction
    ; SIM byte: bit 7=SOD (0), bit 6=SDE (0), bit 5=xxx, bit 4=R7.5 (0),
    ;           bit 3=MSE (1=set mask), bit 2=M7.5 (1=mask), bit 1=M6.5 (0=unmask),
    ;           bit 0=M5.5 (1=mask)
    MVI A, 0x0D             ; MSE=1, M7.5=1, M6.5=0 (unmask), M5.5=1
    SIM

    ; Copy initialized data from ROM to RAM (for ROM/RAM split configs)
    ; For flat sim config, _sidata == _sdata so this is a no-op
    LXI H, _sidata          ; Source (in ROM, after .text)
    LXI D, _sdata           ; Destination (in RAM)
    LXI B, _edata           ; End of .data in RAM

    ; Compare DE (dest) with BC (end)
    MOV A, D
    CMP B
    JNZ .Ldata_loop
    MOV A, E
    CMP C
    JZ .Ldata_done

.Ldata_loop:
    MOV A, M                ; Read from source
    STAX D                  ; Write to destination
    INX H                   ; source++
    INX D                   ; dest++
    MOV A, D
    CMP B
    JNZ .Ldata_loop
    MOV A, E
    CMP C
    JNZ .Ldata_loop

.Ldata_done:
    ; Clear BSS section
    LXI H, _sbss            ; Start of BSS
    LXI D, _ebss            ; End of BSS
    MVI B, 0                ; Zero value

    MOV A, H
    CMP D
    JNZ .Lbss_loop
    MOV A, L
    CMP E
    JZ .Lbss_done

.Lbss_loop:
    MOV M, B                ; *HL = 0
    INX H                   ; HL++
    MOV A, H
    CMP D
    JNZ .Lbss_loop
    MOV A, L
    CMP E
    JNZ .Lbss_loop

.Lbss_done:
    ; Call main()
    CALL main

    ; If main returns, disable interrupts and halt
    DI
.Lhalt:
    HLT
    JMP .Lhalt

; ---------------------------------------------------------------------------
; Task exit trap - if a task function returns, it ends up here
; ---------------------------------------------------------------------------
    .globl _task_exit_trap
_task_exit_trap:
    DI
    HLT
    JMP _task_exit_trap

; ---------------------------------------------------------------------------
; Unhandled interrupt - disable and halt
; ---------------------------------------------------------------------------
    .globl _unhandled_irq
_unhandled_irq:
    EI
    RET
