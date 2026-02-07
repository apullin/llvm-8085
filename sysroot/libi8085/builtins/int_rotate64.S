; Hand-optimised 64-bit rotate routines for i8085.
;
; Calling convention (sret for i64 return):
;   [SP+2..3]   = sret pointer (where to store 8-byte result)
;   [SP+4..11]  = x (int64_t, 8 bytes, little-endian)
;   [SP+12..15] = n (int32_t, only low 6 bits used)
;
; Algorithm: byte-shuffle for multiples of 8, then bit-level rotate
;            for remaining 0-7 bits. Same structure as int_shift64.S.

	.text

; ===================================================================
; __rotldi2: i64 left rotate
; ===================================================================
	.globl	__rotldi2
	.type	__rotldi2,@function
__rotldi2:
	; Allocate 8-byte work area
	lxi	h, 0
	push	h
	push	h
	push	h
	push	h

	; Stack layout:
	;   [SP+ 0.. 7] = work area
	;   [SP+ 8.. 9] = return address
	;   [SP+10..11] = sret pointer
	;   [SP+12..19] = x (input)
	;   [SP+20..23] = n

	; Copy x to work area
	lxi	h, 12
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	lxi	h, 0
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	lxi	h, 16
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	lxi	h, 4
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	; Load rotate amount (only low 6 bits matter)
	lxi	h, 20
	dad	sp
	mov	a, m
	ani	63

.Lrotl64_byte_loop:
	cpi	8
	jc	.Lrotl64_bit_shift

	; Byte rotate left: work[i] = work[i-1], work[0] = old work[7]
	push	psw
	; Read work[7] first (will become work[0])
	lxi	h, 9
	dad	sp		; work[7] at SP+2+7=9
	mov	d, m		; save old work[7]

	; Read work[0..6], store shifted
	lxi	h, 2
	dad	sp		; -> work[0]
	mov	c, m		; old work[0]
	mov	m, d		; work[0] = old work[7]... wait
	; Left rotate: each byte moves UP, top wraps to bottom.
	; work_new[1] = work_old[0], work_new[2] = work_old[1], ...
	; work_new[0] = work_old[7]
	; So: save work[7], shift work[6]->work[7], ..., work[0]->work[1],
	;     put saved work[7] into work[0].

	; Read all bytes, shift
	lxi	h, 8
	dad	sp		; -> work[6]
	mov	e, m		; work[6]
	dcx	h
	mov	c, m		; work[5]
	dcx	h
	mov	b, m		; work[4]
	; Store upper bytes
	lxi	h, 9
	dad	sp		; -> work[7]
	mov	m, e		; work[7] = old work[6]
	dcx	h
	mov	m, c		; work[6] = old work[5]
	dcx	h
	mov	m, b		; work[5] = old work[4]

	; Read work[1..3]
	lxi	h, 3
	dad	sp		; -> work[1]
	mov	e, m		; work[1]
	inx	h
	mov	c, m		; work[2]
	inx	h
	mov	b, m		; work[3]
	; Store: work[4]=work[3], work[3]=work[2], work[2]=work[1]
	lxi	h, 6
	dad	sp		; -> work[4]
	mov	m, b		; work[4] = old work[3]
	dcx	h
	mov	m, c		; work[3] = old work[2]
	dcx	h
	mov	m, e		; work[2] = old work[1]

	; work[0] -> work[1], old work[7] -> work[0]
	lxi	h, 2
	dad	sp		; -> work[0]
	mov	c, m		; old work[0]
	mov	m, d		; work[0] = old work[7] (saved in D)
	inx	h
	mov	m, c		; work[1] = old work[0]

	pop	psw
	sui	8
	jmp	.Lrotl64_byte_loop

.Lrotl64_bit_shift:
	ora	a
	jz	.Lrotl64_done

	mov	e, a		; E = remaining count

.Lrotl64_bit_loop:
	; Rotate all 8 bytes left by 1 bit.
	; Wrap bit = MSB of work[7], enters as LSB of work[0].
	lxi	h, 7
	dad	sp		; -> work[7]
	mov	a, m
	ral			; carry = MSB of work[7]

	; Navigate from work[7] to work[0] without clobbering carry
	; (DAD SP would destroy carry — use 7×DCX instead)
	dcx	h
	dcx	h
	dcx	h
	dcx	h
	dcx	h
	dcx	h
	dcx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a

	dcr	e
	jnz	.Lrotl64_bit_loop

.Lrotl64_done:
	; Write result to sret pointer
	lxi	h, 10
	dad	sp
	mov	e, m
	inx	h
	mov	d, m		; DE = sret pointer

	lxi	h, 0
	dad	sp
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d

	inx	d
	inx	h
	push	d
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	pop	h
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	; Deallocate work area
	lxi	h, 8
	dad	sp
	sphl

	ret
	.size	__rotldi2, .-__rotldi2

; ===================================================================
; __rotrdi2: i64 right rotate
; ===================================================================
	.globl	__rotrdi2
	.type	__rotrdi2,@function
__rotrdi2:
	; Allocate 8-byte work area
	lxi	h, 0
	push	h
	push	h
	push	h
	push	h

	; Copy x to work area (same as rotl)
	lxi	h, 12
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	lxi	h, 0
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	lxi	h, 16
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	lxi	h, 4
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	; Load rotate amount
	lxi	h, 20
	dad	sp
	mov	a, m
	ani	63

.Lrotr64_byte_loop:
	cpi	8
	jc	.Lrotr64_bit_shift

	; Byte rotate right: work[i] = work[i+1], work[7] = old work[0]
	push	psw

	; Save work[0] (will become work[7])
	lxi	h, 2
	dad	sp
	mov	d, m		; save old work[0]

	; Shift work[1..3] -> work[0..2]
	lxi	h, 3
	dad	sp		; -> work[1]
	mov	c, m		; work[1]
	inx	h
	mov	b, m		; work[2]
	inx	h
	mov	e, m		; work[3]
	lxi	h, 2
	dad	sp
	mov	m, c		; work[0] = old work[1]
	inx	h
	mov	m, b		; work[1] = old work[2]
	inx	h
	mov	m, e		; work[2] = old work[3]

	; Shift work[4..7] -> work[3..6]
	lxi	h, 6
	dad	sp		; -> work[4]
	mov	c, m		; work[4]
	inx	h
	mov	b, m		; work[5]
	inx	h
	mov	e, m		; work[6]
	inx	h
	mov	a, m		; work[7]
	lxi	h, 5
	dad	sp		; -> work[3]
	mov	m, c		; work[3] = old work[4]
	inx	h
	mov	m, b		; work[4] = old work[5]
	inx	h
	mov	m, e		; work[5] = old work[6]
	inx	h
	mov	m, a		; work[6] = old work[7]

	; work[7] = old work[0] (saved in D)
	inx	h
	mov	m, d		; work[7] = old work[0]

	pop	psw
	sui	8
	jmp	.Lrotr64_byte_loop

.Lrotr64_bit_shift:
	ora	a
	jz	.Lrotr64_done

	mov	e, a		; E = remaining count

.Lrotr64_bit_loop:
	; Rotate all 8 bytes right by 1 bit.
	; Wrap bit = LSB of work[0], enters as MSB of work[7].
	lxi	h, 0
	dad	sp
	mov	a, m		; work[0]
	rar			; carry = LSB of work[0]

	; Navigate from work[0] to work[7] without clobbering carry
	; (DAD SP would destroy carry — use 7×INX instead)
	inx	h
	inx	h
	inx	h
	inx	h
	inx	h
	inx	h
	inx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a
	dcx	h
	mov	a, m
	rar
	mov	m, a

	dcr	e
	jnz	.Lrotr64_bit_loop

.Lrotr64_done:
	; Write result to sret pointer (same epilogue as rotl)
	lxi	h, 10
	dad	sp
	mov	e, m
	inx	h
	mov	d, m

	lxi	h, 0
	dad	sp
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d
	inx	h
	inx	d
	mov	a, m
	stax	d

	inx	d
	inx	h
	push	d
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	pop	h
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	lxi	h, 8
	dad	sp
	sphl

	ret
	.size	__rotrdi2, .-__rotrdi2
