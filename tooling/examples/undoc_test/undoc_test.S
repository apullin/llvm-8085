; Test all 10 undocumented 8085 instructions
; Output at 0x0200: one byte per test (expected value), then HALT
; Expected output: 01 01 01 01 01 01 01 01 01 01

    .text
    .globl _start
_start:
    lxi sp, 0xFFFE

    ; ---- Test 1: DSUB (HL = HL - BC) ----
    lxi h, 0x1234
    lxi b, 0x0234
    .byte 0x08          ; DSUB: HL = 0x1234 - 0x0234 = 0x1000
    mov a, h
    cpi 0x10
    jnz fail
    mov a, l
    cpi 0x00
    jnz fail
    mvi a, 0x01
    sta 0x0200          ; Test 1 pass

    ; ---- Test 2: ARHL (arithmetic right shift HL) ----
    lxi h, 0x8004       ; -32764, shift right = -16382 = 0xC002
    .byte 0x10          ; ARHL
    mov a, h
    cpi 0xC0
    jnz fail
    mov a, l
    cpi 0x02
    jnz fail
    mvi a, 0x01
    sta 0x0201          ; Test 2 pass

    ; ---- Test 3: RDEL (rotate DE left through carry) ----
    stc                 ; Set carry
    lxi d, 0x4000       ; DE = 0x4000, CY=1
    .byte 0x18          ; RDEL: DE = 0x8001 (D7→CY=0, CY→E0=1)
    mov a, d
    cpi 0x80
    jnz fail
    mov a, e
    cpi 0x01
    jnz fail
    mvi a, 0x01
    sta 0x0202          ; Test 3 pass

    ; ---- Test 4: LDHI (DE = HL + imm8) ----
    lxi h, 0x1000
    .byte 0x28          ; LDHI
    .byte 0x05          ; imm8 = 5, DE = 0x1005
    mov a, d
    cpi 0x10
    jnz fail
    mov a, e
    cpi 0x05
    jnz fail
    mvi a, 0x01
    sta 0x0203          ; Test 4 pass

    ; ---- Test 5: LDSI (DE = SP + imm8) ----
    lxi sp, 0xFF00
    .byte 0x38          ; LDSI
    .byte 0x04          ; imm8 = 4, DE = 0xFF04
    mov a, d
    cpi 0xFF
    jnz fail
    mov a, e
    cpi 0x04
    jnz fail
    lxi sp, 0xFFFE      ; Restore SP
    mvi a, 0x01
    sta 0x0204          ; Test 5 pass

    ; ---- Test 6: SHLX ([DE] = HL) ----
    lxi h, 0xBEEF
    lxi d, 0x0300       ; Store at 0x0300
    .byte 0xD9          ; SHLX: mem[0x0300]=0xEF, mem[0x0301]=0xBE
    lda 0x0300
    cpi 0xEF
    jnz fail
    lda 0x0301
    cpi 0xBE
    jnz fail
    mvi a, 0x01
    sta 0x0205          ; Test 6 pass

    ; ---- Test 7: LHLX (HL = [DE]) ----
    ; 0x0300-0x0301 still has 0xBEEF from test 6
    lxi d, 0x0300
    .byte 0xED          ; LHLX: HL = mem[0x0300..0x0301] = 0xBEEF
    mov a, h
    cpi 0xBE
    jnz fail
    mov a, l
    cpi 0xEF
    jnz fail
    mvi a, 0x01
    sta 0x0206          ; Test 7 pass

    ; ---- Test 8: LDSI + LHLX combo (stack access pattern) ----
    ; Push a known value onto stack, then read it back with LDSI+LHLX
    lxi h, 0xCAFE
    push h              ; SP -= 2, stack has 0xCAFE
    .byte 0x38          ; LDSI
    .byte 0x00          ; imm8 = 0, DE = SP+0 (points to pushed value)
    .byte 0xED          ; LHLX: HL = [DE] = 0xCAFE
    mov a, h
    cpi 0xCA
    jnz fail
    mov a, l
    cpi 0xFE
    jnz fail
    pop h               ; Clean up stack
    mvi a, 0x01
    sta 0x0207          ; Test 8 pass

    ; ---- Test 9: LDSI + SHLX combo (stack store pattern) ----
    ; Store a value at a stack location using LDSI+SHLX
    push h              ; Make room on stack (SP -= 2)
    lxi h, 0xDEAD
    .byte 0x38          ; LDSI
    .byte 0x00          ; imm8 = 0, DE = SP+0
    .byte 0xD9          ; SHLX: [DE] = HL = 0xDEAD
    ; Now read it back with POP to verify
    pop h               ; HL = 0xDEAD
    mov a, h
    cpi 0xDE
    jnz fail
    mov a, l
    cpi 0xAD
    jnz fail
    mvi a, 0x01
    sta 0x0208          ; Test 9 pass

    ; ---- Test 10: DSUB flags (zero result) ----
    lxi h, 0x5678
    lxi b, 0x5678
    .byte 0x08          ; DSUB: HL = 0x5678 - 0x5678 = 0x0000
    jnz fail            ; Zero flag should be set
    mvi a, 0x01
    sta 0x0209          ; Test 10 pass

    hlt                 ; Success!

fail:
    mvi a, 0xFF
    hlt
