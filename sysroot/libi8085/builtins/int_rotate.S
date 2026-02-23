; Hand-optimised rotate routines for i8085.
;
; __rotlhi2: left rotate i16  — [SP+2..3]=x, [SP+4]=n, returns BC
; __rotrhi2: right rotate i16 — [SP+2..3]=x, [SP+4]=n, returns BC
; __rotlsi2: left rotate i32  — [SP+2..5]=x, [SP+6]=n, returns BC:DE
; __rotrsi2: right rotate i32 — [SP+2..5]=x, [SP+6]=n, returns BC:DE

	.text

; ===================================================================
; __rotlhi2: uint16_t left rotate
; ===================================================================
	.globl	__rotlhi2
	.type	__rotlhi2,@function
__rotlhi2:
	; Load x into BC
	lxi	h, 2
	dad	sp
	mov	c, m		; x low
	inx	h
	mov	b, m		; x high

	; Load n, mask to 0-15
	inx	h
	mov	a, m
	ani	15
	ora	a
	jz	.Lrotlhi_done

	; If n >= 8, swap bytes and subtract 8
	cpi	8
	jc	.Lrotlhi_bits
	; Swap B and C
	mov	d, b
	mov	b, c
	mov	c, d
	sui	8
	ora	a
	jz	.Lrotlhi_done

.Lrotlhi_bits:
	; Bit rotate left by A bits (1..7)
	mov	e, a		; E = count
.Lrotlhi_loop:
	; Rotate BC left by 1 bit.
	; Wrap bit = MSB of B (bit 15), enters LSB of C (bit 0).
	; Step 1: set carry = bit 7 of B (the wrap-around bit)
	mov	a, b
	ral			; carry = bit7 of B
	; Step 2: shift C left through carry
	mov	a, c
	ral			; A = C<<1 | wrap_bit; carry = old bit7 of C
	mov	c, a
	; Step 3: shift B left through carry from C
	mov	a, b
	ral			; A = B<<1 | carry_from_C
	mov	b, a

	dcr	e
	jnz	.Lrotlhi_loop

.Lrotlhi_done:
	ret
	.size	__rotlhi2, .-__rotlhi2

; ===================================================================
; __rotrhi2: uint16_t right rotate
; ===================================================================
	.globl	__rotrhi2
	.type	__rotrhi2,@function
__rotrhi2:
	; Load x into BC
	lxi	h, 2
	dad	sp
	mov	c, m		; x low
	inx	h
	mov	b, m		; x high

	; Load n, mask to 0-15
	inx	h
	mov	a, m
	ani	15
	ora	a
	jz	.Lrotrhi_done

	; If n >= 8, swap bytes and subtract 8
	cpi	8
	jc	.Lrotrhi_bits
	mov	d, b
	mov	b, c
	mov	c, d
	sui	8
	ora	a
	jz	.Lrotrhi_done

.Lrotrhi_bits:
	; Bit rotate right by A bits (1..7)
	mov	e, a		; E = count
.Lrotrhi_loop:
	; Rotate BC right by 1: LSB of C wraps to MSB of B.
	; Set carry = bit 0 of C (the wrap-around bit)
	mov	a, c
	rar			; carry = bit0 of C
	; Shift B right through carry
	mov	a, b
	rar			; A = carry|B>>1; carry = bit0 of B
	mov	b, a
	; Shift C right through carry
	mov	a, c
	rar			; A = carry|C>>1
	mov	c, a

	dcr	e
	jnz	.Lrotrhi_loop

.Lrotrhi_done:
	ret
	.size	__rotrhi2, .-__rotrhi2

; ===================================================================
; __rotlsi2: uint32_t left rotate
; [SP+2..5] = x (little-endian, byte0=C, byte1=B, byte2=E, byte3=D)
; [SP+6] = n (shift amount, only low 5 bits matter)
; Returns BC:DE
; ===================================================================
	.globl	__rotlsi2
	.type	__rotlsi2,@function
__rotlsi2:
	; Load x into registers: C=byte0, B=byte1, E=byte2, D=byte3
	lxi	h, 2
	dad	sp
	mov	c, m		; byte 0
	inx	h
	mov	b, m		; byte 1
	inx	h
	mov	e, m		; byte 2
	inx	h
	mov	d, m		; byte 3

	; Load n, mask to 0-31
	inx	h
	mov	a, m
	ani	31
	ora	a
	jz	.Lrotlsi_done

	; Byte shuffle for n >= 8
.Lrotlsi_byteloop:
	cpi	8
	jc	.Lrotlsi_bits
	; Rotate bytes left: D=E, E=B, B=C, C=old_D... wait
	; Left rotate: byte3=byte2, byte2=byte1, byte1=byte0, byte0=byte3
	; i.e. D(byte3) gets old E(byte2), E gets old B(byte1),
	;      B gets old C(byte0), C gets old D(byte3)
	push	psw		; save shift amount
	mov	a, d		; save old byte3
	mov	d, e		; byte3 = old byte2
	mov	e, b		; byte2 = old byte1
	mov	b, c		; byte1 = old byte0
	mov	c, a		; byte0 = old byte3
	pop	psw
	sui	8
	ora	a
	jz	.Lrotlsi_done
	jmp	.Lrotlsi_byteloop

.Lrotlsi_bits:
	; Bit rotate left by A bits (1..7)
	; Use stack working area since we need all registers
	push	d		; save byte3:byte2 on stack
	push	b		; save byte1:byte0 on stack
	mov	d, a		; D = bit count

	; Stack: [SP+0..1]=byte0,byte1 [SP+2..3]=byte2,byte3
.Lrotlsi_bitloop:
	; Rotate 4 bytes left by 1 bit.
	; Carry chain: wrap bit = MSB of byte3
	; byte0 = byte0<<1 | wrap_bit
	; byte1 = byte1<<1 | carry_from_byte0
	; byte2 = byte2<<1 | carry_from_byte1
	; byte3 = byte3<<1 | carry_from_byte2

	; Point HL to byte0 on stack (do this BEFORE setting carry,
	; because dad sp clobbers the carry flag)
	lxi	h, 0
	dad	sp

	; Get wrap bit (MSB of byte3) into carry
	; byte3 is at offset 3 from HL (which points to byte0)
	push	h		; save pointer to byte0
	inx	h
	inx	h
	inx	h		; HL now points to byte3
	mov	a, m		; byte3
	ral			; carry = bit7 of byte3
	pop	h		; restore pointer to byte0 (pop doesn't affect carry)

	; Now chain through all 4 bytes using RAL
	mov	a, m		; byte0
	ral
	mov	m, a		; byte0 shifted
	inx	h
	mov	a, m		; byte1
	ral
	mov	m, a
	inx	h
	mov	a, m		; byte2
	ral
	mov	m, a
	inx	h
	mov	a, m		; byte3
	ral
	mov	m, a

	dcr	d
	jnz	.Lrotlsi_bitloop

	; Restore result into BC:DE
	pop	b		; C=byte0, B=byte1
	pop	d		; E=byte2, D=byte3

.Lrotlsi_done:
	ret
	.size	__rotlsi2, .-__rotlsi2

; ===================================================================
; __rotrsi2: uint32_t right rotate
; [SP+2..5] = x, [SP+6] = n
; Returns BC:DE
; ===================================================================
	.globl	__rotrsi2
	.type	__rotrsi2,@function
__rotrsi2:
	; Load x
	lxi	h, 2
	dad	sp
	mov	c, m		; byte 0
	inx	h
	mov	b, m		; byte 1
	inx	h
	mov	e, m		; byte 2
	inx	h
	mov	d, m		; byte 3

	; Load n, mask to 0-31
	inx	h
	mov	a, m
	ani	31
	ora	a
	jz	.Lrotrsi_done

	; Byte shuffle for n >= 8
.Lrotrsi_byteloop:
	cpi	8
	jc	.Lrotrsi_bits
	; Rotate bytes right: byte0=byte1, byte1=byte2, byte2=byte3, byte3=byte0
	; i.e. C gets old B, B gets old E, E gets old D, D gets old C
	push	psw
	mov	a, c		; save old byte0
	mov	c, b		; byte0 = old byte1
	mov	b, e		; byte1 = old byte2
	mov	e, d		; byte2 = old byte3
	mov	d, a		; byte3 = old byte0
	pop	psw
	sui	8
	ora	a
	jz	.Lrotrsi_done
	jmp	.Lrotrsi_byteloop

.Lrotrsi_bits:
	; Bit rotate right by A bits (1..7)
	push	d
	push	b
	mov	d, a		; D = bit count

.Lrotrsi_bitloop:
	; Rotate 4 bytes right by 1 bit.
	; Wrap bit = LSB of byte0.
	; byte3 = byte3>>1 | wrap_bit<<7
	; byte2 = byte2>>1 | carry_from_byte3
	; byte1 = byte1>>1 | carry_from_byte2
	; byte0 = byte0>>1 | carry_from_byte1

	; Point HL to byte3 on stack (do this BEFORE setting carry,
	; because dad sp clobbers the carry flag)
	lxi	h, 3
	dad	sp

	; Get wrap bit (LSB of byte0) into carry
	; byte0 is at offset -3 from HL (which points to byte3)
	push	h		; save pointer to byte3
	dcx	h
	dcx	h
	dcx	h		; HL now points to byte0
	mov	a, m		; byte0
	rar			; carry = bit0 of byte0
	pop	h		; restore pointer to byte3 (pop doesn't affect carry)

	; Chain from MSB to LSB using RAR
	mov	a, m		; byte3
	rar
	mov	m, a
	dcx	h
	mov	a, m		; byte2
	rar
	mov	m, a
	dcx	h
	mov	a, m		; byte1
	rar
	mov	m, a
	dcx	h
	mov	a, m		; byte0
	rar
	mov	m, a

	dcr	d
	jnz	.Lrotrsi_bitloop

	pop	b
	pop	d

.Lrotrsi_done:
	ret
	.size	__rotrsi2, .-__rotrsi2
