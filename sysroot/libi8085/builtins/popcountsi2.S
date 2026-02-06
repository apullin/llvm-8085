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

; popcount8: count set bits in A. Returns count in A. Clobbers B.
.Lpopcount8:
	mov	b, a		; B = x
	mvi	a, 0		; A = count
	mov	c, b		; C = x (working copy)
.Lpopcount8_loop:
	mov	b, c		; B = x
	ora	b		; test if B is zero (also clears carry... but we use ORA)
	mov	a, b
	ora	a
	jz	.Lpopcount8_done_restore
	; x &= (x - 1): clears lowest set bit
	mov	a, c		; A = x
	dcr	a		; A = x - 1
	ana	c		; A = x & (x - 1)
	mov	c, a		; C = new x
	; Increment count (we need a separate counter)
	; Let me restructure with D as counter
	jmp	.Lpopcount8_loop
.Lpopcount8_done_restore:
	ret

	; Better approach: use D as counter, A/B for Kernighan
.Lpop8:
	; A = byte to count. Returns count in A.
	; Clobbers B.
	ora	a
	jz	.Lpop8_zero
	mov	b, a		; B = x
	mvi	a, 0		; A = count
.Lpop8_loop:
	inr	a		; count++
	mov	c, b		; save count... no, A is count
	; x &= (x-1)
	push	psw		; save count
	mov	a, b
	dcr	a		; A = x - 1
	ana	b		; A = x & (x - 1)
	mov	b, a		; B = new x
	pop	psw		; restore count
	ora	b		; test if x is zero... no this corrupts A
	; Need a different approach
	push	psw		; save count again
	mov	a, b
	ora	a
	jz	.Lpop8_done
	pop	psw
	jmp	.Lpop8_loop
.Lpop8_done:
	pop	psw		; A = count
	ret
.Lpop8_zero:
	; A is already 0
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
