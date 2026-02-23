; Hand-optimised popcount routines for i8085.
;
; __popcountsi2: count set bits in a 32-bit value.
;   [SP+2..5] = x (i32, little-endian)
;   Returns i32 in BC:DE
;
; __popcountdi2: count set bits in a 64-bit value.
;   [SP+2..9] = x (i64, little-endian)
;   Returns i32 in BC:DE
;
; Uses Kernighan's trick: x &= (x-1) clears the lowest set bit.
; Count iterations until zero.

	.text

; pop8: count set bits in byte A. Returns count in A.
; Clobbers B.
; Uses Kernighan's trick: x &= (x-1) clears lowest set bit.
.Lpop8:
	ora	a
	rz			; zero byte -> return 0
	mov	b, a		; B = x
	mvi	a, 0		; A = count
.Lpop8_loop:
	inr	a		; count++ (we know at least 1 bit is set)
	; x &= (x-1): clear lowest set bit
	push	psw		; save count on stack
	mov	a, b		; A = x
	dcr	a		; A = x - 1
	ana	b		; A = x & (x - 1)
	mov	b, a		; B = new x
	ora	a		; test if x is now zero
	jz	.Lpop8_done	; done -- count is still on stack
	pop	psw		; restore count
	jmp	.Lpop8_loop
.Lpop8_done:
	pop	psw		; A = count
	ret

	.globl	__popcountsi2
	.type	__popcountsi2,@function
__popcountsi2:
	; Sum popcount of each of 4 bytes.
	; Use E as running total.
	mvi	e, 0		; total = 0

	lxi	h, 2
	dad	sp

	; Byte 0
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a		; save byte count
	pop	h
	mov	a, e
	add	d
	mov	e, a		; total += count

	; Byte 1
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 2
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 3
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d

	; Result in A = total popcount
	mov	c, a
	mvi	b, 0
	mvi	e, 0
	mvi	d, 0
	ret
	.size	__popcountsi2, .-__popcountsi2

	.globl	__popcountdi2
	.type	__popcountdi2,@function
__popcountdi2:
	; Sum popcount of each of 8 bytes.
	; Use E as running total.
	mvi	e, 0		; total = 0

	lxi	h, 2
	dad	sp

	; Byte 0
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 1
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 2
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 3
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 4
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 5
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 6
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d
	mov	e, a

	; Byte 7
	inx	h
	mov	a, m
	push	h
	call	.Lpop8
	mov	d, a
	pop	h
	mov	a, e
	add	d

	; Result in A = total popcount
	mov	c, a
	mvi	b, 0
	mvi	e, 0
	mvi	d, 0
	ret
	.size	__popcountdi2, .-__popcountdi2
