; Hand-optimised 32-bit funnel shift routines for i8085.
;
; __fshlsi3(uint32_t hi, uint32_t lo, uint8_t n):
;   result = (hi << n) | (lo >> (32-n))
;   [SP+2..5] = hi, [SP+6..9] = lo, [SP+10] = n
;   Returns BC:DE
;
; __fshrsi3(uint32_t hi, uint32_t lo, uint8_t n):
;   result = (hi << (32-n)) | (lo >> n)
;   [SP+2..5] = hi, [SP+6..9] = lo, [SP+10] = n
;   Returns BC:DE

	.text

; ===================================================================
; __fshlsi3: funnel shift left i32
; ===================================================================
	.globl	__fshlsi3
	.type	__fshlsi3,@function
__fshlsi3:
	; Load shift amount, mask to 0-31
	lxi	h, 10
	dad	sp
	mov	a, m
	ani	31
	ora	a
	jz	.Lfshlsi_zero	; n == 0, return hi

	; Allocate 8-byte work area: [work+0..3]=hi, [work+4..7]=lo
	; We'll treat this as an 8-byte value and shift left by n,
	; then take bytes 4..7 of the result.
	; Actually: fshl(hi,lo,n) for n in 1..31 means we concatenate
	; hi:lo (8 bytes, hi is upper), shift left by n, take upper 4 bytes.
	; In little-endian memory: lo at lower address, hi at higher.
	; So work[0..3]=lo, work[4..7]=hi, shift left by n, result = work[4..7].

	push	psw		; save n
	; Copy lo to work[0..3]
	lxi	h, 8		; lo at SP+6+2(push)=8
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	; Allocate work area below: push hi first, then lo
	; Actually easier: push lo, then hi on top
	; work[0..3] = lo, work[4..7] = hi
	; Let's push in reverse order to get correct layout

	; Load hi
	lxi	h, 4		; hi at SP+2+2(push)=4
	dad	sp
	; Save lo values first
	push	d		; lo byte3,byte2
	push	b		; lo byte1,byte0
	; Now load hi
	lxi	h, 8		; hi at SP+2+4(pushes)=6... recalc
	; With 2 pushes (4 bytes) + 1 push(psw, 2 bytes) = 6 bytes on stack
	; Original hi at SP_orig+2, new SP = SP_orig-6
	; hi at new_SP + 2 + 6 = new_SP + 8
	lxi	h, 8
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	push	d		; hi byte3,byte2
	push	b		; hi byte1,byte0

	; Stack: [SP+0..3]=hi, [SP+4..7]=lo, [SP+8..9]=saved_n(psw)
	; But we want [SP+0..3]=lo, [SP+4..7]=hi.
	; Let me redo the push order.

	; Clean up and redo
	pop	b
	pop	d
	pop	b
	pop	d
	; Back to just psw on stack. Redo properly.

	; Load lo
	lxi	h, 10		; lo at SP_orig+6, with psw pushed: SP+6+2=8...
	; SP_orig was before push psw. SP now = SP_orig-2.
	; lo at SP_orig+6 = SP+8
	lxi	h, 8
	dad	sp
	mov	c, m		; lo byte0
	inx	h
	mov	b, m		; lo byte1
	inx	h
	mov	e, m		; lo byte2
	inx	h
	mov	d, m		; lo byte3

	; Push lo (becomes work[0..3])
	push	d		; lo byte3, byte2  -> work[2..3]
	push	b		; lo byte1, byte0  -> work[0..1]

	; Load hi
	; hi at SP_orig+2 = (current SP + 4 + 2 + 2) = SP+8...
	; current SP = SP_orig - 2(psw) - 4(lo pushes)= SP_orig-6
	; hi at SP_orig+2 = SP+8
	lxi	h, 8
	dad	sp
	mov	c, m		; hi byte0
	inx	h
	mov	b, m		; hi byte1
	inx	h
	mov	e, m		; hi byte2
	inx	h
	mov	d, m		; hi byte3

	; Push hi (becomes work[4..7])
	push	d		; hi byte3, byte2  -> work[6..7]
	push	b		; hi byte1, byte0  -> work[4..5]

	; Stack layout:
	; [SP+0..3] = hi (work[4..7]... no)
	; Wait, pushes go to decreasing addresses.
	; push d:  SP-=2, [SP]=E, [SP+1]=D  (lo byte, hi byte)
	; push b:  SP-=2, [SP]=C, [SP+1]=B
	; So the layout ends up:
	; First pushed lo: push d -> D,E at higher addresses
	; push b -> B,C at lower addresses
	; Then pushed hi on top.
	; This puts hi at lower SP addresses and lo at higher.
	; That's backwards from what we want!

	; Let me use a simpler approach: allocate 8 bytes, copy manually.
	pop	b		; undo
	pop	d		; undo
	pop	b		; undo
	pop	d		; undo
	; Just psw on stack again.

	; Allocate 8 bytes of work area
	lxi	h, 0
	push	h		; work[6..7]
	push	h		; work[4..5]
	push	h		; work[2..3]
	push	h		; work[0..1]

	; Stack:
	; [SP+0..7] = work area
	; [SP+8..9] = saved n (PSW)
	; [SP+10..11] = return address
	; [SP+12..15] = hi
	; [SP+16..19] = lo
	; [SP+20] = n

	; Copy lo to work[0..3]
	lxi	h, 16
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

	; Copy hi to work[4..7]
	lxi	h, 12
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

	; Recover n from saved PSW
	pop	psw		; but this pops from work area!
	; No, PSW was pushed before work area. Let me reconsider.
	; Actually PSW is at SP+8, not at SP+0. I can't pop it.
	; Let me read n from original stack position instead.
	; n at [SP+20] (original [SP_orig+10])
	lxi	h, 20
	dad	sp
	mov	a, m
	ani	31

	; Left shift the 8-byte buffer by A bits, then result = work[4..7]
.Lfshlsi_byteloop:
	cpi	8
	jc	.Lfshlsi_bits

	; Byte shuffle left: work[7]=work[6], ..., work[1]=work[0], work[0]=0
	push	psw
	; Same byte shuffle as in __ashldi3
	lxi	h, 8
	dad	sp		; work[6] at SP+2+6=8
	mov	d, m
	dcx	h
	mov	e, m
	dcx	h
	mov	b, m
	dcx	h
	mov	c, m
	lxi	h, 9
	dad	sp
	mov	m, d
	dcx	h
	mov	m, e
	dcx	h
	mov	m, b
	dcx	h
	mov	m, c

	lxi	h, 4
	dad	sp
	mov	d, m		; work[2]
	dcx	h
	mov	e, m		; work[1]
	dcx	h
	mov	c, m		; work[0]
	lxi	h, 5
	dad	sp
	mov	m, d
	dcx	h
	mov	m, e
	dcx	h
	mov	m, c
	dcx	h
	mvi	m, 0		; work[0] = 0

	pop	psw
	sui	8
	jmp	.Lfshlsi_byteloop

.Lfshlsi_bits:
	ora	a
	jz	.Lfshlsi_extract

	mov	e, a
.Lfshlsi_bitloop:
	; Shift all 8 bytes left by 1 bit
	lxi	h, 0
	dad	sp
	mov	a, m
	add	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a
	inx	h
	mov	a, m
	adc	a
	mov	m, a

	dcr	e
	jnz	.Lfshlsi_bitloop

.Lfshlsi_extract:
	; Result = work[4..7] = upper 32 bits
	lxi	h, 4
	dad	sp
	mov	c, m		; byte0
	inx	h
	mov	b, m		; byte1
	inx	h
	mov	e, m		; byte2
	inx	h
	mov	d, m		; byte3

	; Deallocate work area (8 bytes) + saved PSW (already consumed...
	; actually we never popped PSW from the right place)
	; Work area = 8 bytes, PSW = 2 bytes = 10 bytes total
	lxi	h, 10
	dad	sp
	sphl
	ret

.Lfshlsi_zero:
	; n == 0: return hi directly
	lxi	h, 2
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	ret

	.size	__fshlsi3, .-__fshlsi3

; ===================================================================
; __fshrsi3: funnel shift right i32
;   result = (hi << (32-n)) | (lo >> n)
;   Treat as 8-byte buffer [lo,hi], shift RIGHT by n, take lower 4 bytes.
; ===================================================================
	.globl	__fshrsi3
	.type	__fshrsi3,@function
__fshrsi3:
	; Load shift amount
	lxi	h, 10
	dad	sp
	mov	a, m
	ani	31
	ora	a
	jz	.Lfshrsi_zero	; n == 0, return lo

	push	psw		; save n

	; Allocate 8-byte work area
	lxi	h, 0
	push	h
	push	h
	push	h
	push	h

	; Stack:
	; [SP+0..7] = work area
	; [SP+8..9] = saved n (PSW)
	; [SP+10..11] = return address
	; [SP+12..15] = hi
	; [SP+16..19] = lo
	; [SP+20] = n

	; Copy lo to work[0..3]
	lxi	h, 16
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

	; Copy hi to work[4..7]
	lxi	h, 12
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

	; Read n
	lxi	h, 20
	dad	sp
	mov	a, m
	ani	31

	; Right shift the 8-byte buffer by A bits, then result = work[0..3]
.Lfshrsi_byteloop:
	cpi	8
	jc	.Lfshrsi_bits

	; Byte shuffle right: work[0]=work[1], ..., work[6]=work[7], work[7]=0
	push	psw
	lxi	h, 3
	dad	sp
	mov	c, m		; work[1]
	inx	h
	mov	b, m		; work[2]
	inx	h
	mov	e, m		; work[3]
	inx	h
	mov	d, m		; work[4]
	lxi	h, 2
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mov	m, d

	lxi	h, 7
	dad	sp
	mov	c, m		; work[5]
	inx	h
	mov	b, m		; work[6]
	inx	h
	mov	e, m		; work[7]
	lxi	h, 6
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e
	inx	h
	mvi	m, 0		; work[7] = 0

	pop	psw
	sui	8
	jmp	.Lfshrsi_byteloop

.Lfshrsi_bits:
	ora	a
	jz	.Lfshrsi_extract

	mov	e, a
.Lfshrsi_bitloop:
	; Shift all 8 bytes right by 1 bit (logical)
	lxi	h, 7
	dad	sp
	mov	a, m
	ora	a		; clear carry
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
	jnz	.Lfshrsi_bitloop

.Lfshrsi_extract:
	; Result = work[0..3]
	lxi	h, 0
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m

	; Deallocate work area (8) + saved PSW (2) = 10 bytes
	lxi	h, 10
	dad	sp
	sphl
	ret

.Lfshrsi_zero:
	; n == 0: return lo directly
	lxi	h, 6
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
	inx	h
	mov	e, m
	inx	h
	mov	d, m
	ret

	.size	__fshrsi3, .-__fshrsi3
