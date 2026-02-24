; Simple memory operations for the i8085 target.
;
; The LLVM compiler emits calls to memcpy, memset, and memmove for
; aggregate initialisation, structure copies, etc.
;
; Calling convention (stack-based, cdecl-ish):
;   Arguments pushed right-to-left.  Return value in BC.
;   Stack on entry to memcpy(dst, src, n):
;     [SP+0] = return address (2 bytes)
;     [SP+2] = dst low
;     [SP+3] = dst high
;     [SP+4] = src low
;     [SP+5] = src high
;     [SP+6] = n   low
;     [SP+7] = n   high
;
;   memset(dst, c, n):
;     [SP+2] = dst low
;     [SP+3] = dst high
;     [SP+4] = c   (only low byte used)
;     [SP+5] = (c high, ignored)
;     [SP+6] = n   low
;     [SP+7] = n   high
;
;   Returns dst in BC.

	.text
	.globl	memcpy
	.type	memcpy,@function
memcpy:
	; Load n into DE
#ifdef UNDOC
	ldsi	6
	lhlx
	mov	e, l		; n low
	mov	d, h		; n high
#else
	lxi	h, 6
	dad	sp
	mov	e, m		; n low
	inx	h
	mov	d, m		; n high
#endif

	; Load src into HL (will be used with MOV A,M / INX H)
	lxi	h, 4
	dad	sp
	mov	a, m		; src low
	inx	h
	mov	h, m		; src high
	mov	l, a		; HL = src

	; Save HL (src) on stack, load dst into BC
	push	h		; push src
	lxi	h, 4		; offset 2+2 (return addr + pushed src)
	dad	sp
	mov	c, m		; dst low
	inx	h
	mov	b, m		; dst high
	pop	h		; HL = src again

	; BC = dst, HL = src, DE = n
	; Loop: copy n bytes from [HL] to [BC]
.Lmemcpy_loop:
	; Check if DE == 0
	mov	a, d
	ora	e
	jz	.Lmemcpy_done

	; Copy one byte: A = [HL], [BC] = A
	mov	a, m		; A = *src
	stax	b		; *dst = A
	inx	h		; src++
	inx	b		; dst++
	dcx	d		; n--
	jmp	.Lmemcpy_loop

.Lmemcpy_done:
	; Return dst (original) in BC
	; BC has been incremented, so reload original dst
	lxi	h, 2
	dad	sp
	mov	c, m		; dst low
	inx	h
	mov	b, m		; dst high
	ret
	.size	memcpy, .-memcpy

	.globl	memset
	.type	memset,@function
memset:
	; Load n into DE
#ifdef UNDOC
	ldsi	6
	lhlx
	mov	e, l		; n low
	mov	d, h		; n high
#else
	lxi	h, 6
	dad	sp
	mov	e, m		; n low
	inx	h
	mov	d, m		; n high
#endif

	; Load c (byte value) - only low byte
	lxi	h, 4
	dad	sp
	mov	a, m		; c (value to fill)
	push	psw		; save fill byte

	; Load dst into HL
	lxi	h, 4		; 2 (ret addr) + 2 (pushed PSW)
	dad	sp
	mov	a, m		; dst low
	inx	h
	mov	h, m		; dst high
	mov	l, a		; HL = dst

	pop	psw		; A = fill byte

	; HL = dst, A = fill byte, DE = n
.Lmemset_loop:
	; Check if DE == 0
	push	psw		; save fill byte
	mov	a, d
	ora	e
	jz	.Lmemset_done

	pop	psw		; restore fill byte
	mov	m, a		; *dst = c
	inx	h		; dst++
	dcx	d		; n--
	jmp	.Lmemset_loop

.Lmemset_done:
	pop	psw		; balance the push
	; Return original dst in BC
	lxi	h, 2
	dad	sp
	mov	c, m		; dst low
	inx	h
	mov	b, m		; dst high
	ret
	.size	memset, .-memset

	.globl	memmove
	.type	memmove,@function
memmove:
	; Load n into DE
#ifdef UNDOC
	ldsi	6
	lhlx
	mov	e, l		; n low
	mov	d, h		; n high
#else
	lxi	h, 6
	dad	sp
	mov	e, m		; n low
	inx	h
	mov	d, m		; n high
#endif

	; Check if n == 0
	mov	a, d
	ora	e
	jz	.Lmemmove_done_nop

	; Load dst into BC
	lxi	h, 2
	dad	sp
	mov	c, m		; dst low
	inx	h
	mov	b, m		; dst high

	; Load src into HL
	lxi	h, 4
	dad	sp
	mov	a, m		; src low
	inx	h
	mov	h, m		; src high
	mov	l, a		; HL = src

	; Compare dst vs src: if dst < src, copy forward
	; dst (BC) < src (HL)?  Subtract: dst - src
	mov	a, c
	sub	l
	mov	a, b
	sbb	h
	jc	.Lmemmove_fwd	; dst < src => forward copy

	; Backward copy: start from end
	; HL = src + n - 1, BC = dst + n - 1
	; Add n-1 to both
	push	d		; save n
	dcx	d		; n - 1
	mov	a, l
	add	e
	mov	l, a
	mov	a, h
	adc	d
	mov	h, a		; HL = src + n - 1

	mov	a, c
	add	e
	mov	c, a
	mov	a, b
	adc	d
	mov	b, a		; BC = dst + n - 1
	pop	d		; DE = n

.Lmemmove_bwd:
	mov	a, d
	ora	e
	jz	.Lmemmove_done

	mov	a, m		; A = *src
	stax	b		; *dst = A
	dcx	h		; src--
	dcx	b		; dst--
	dcx	d		; n--
	jmp	.Lmemmove_bwd

.Lmemmove_fwd:
	; Forward copy (same as memcpy loop)
	; HL = src, BC = dst, DE = n
.Lmemmove_fwd_loop:
	mov	a, d
	ora	e
	jz	.Lmemmove_done

	mov	a, m		; A = *src
	stax	b		; *dst = A
	inx	h		; src++
	inx	b		; dst++
	dcx	d		; n--
	jmp	.Lmemmove_fwd_loop

.Lmemmove_done:
	; Return original dst in BC
	lxi	h, 2
	dad	sp
	mov	c, m		; dst low
	inx	h
	mov	b, m		; dst high
	ret

.Lmemmove_done_nop:
	; n == 0, just return dst
#ifdef UNDOC
	ldsi	2
	lhlx
	mov	c, l
	mov	b, h
#else
	lxi	h, 2
	dad	sp
	mov	c, m
	inx	h
	mov	b, m
#endif
	ret
	.size	memmove, .-memmove

	.globl	memcmp
	.type	memcmp,@function
memcmp:
	; int memcmp(const void *s1, const void *s2, size_t n)
	; [SP+2..3] = s1, [SP+4..5] = s2, [SP+6..7] = n
	; Returns i16 in BC: <0 if s1<s2, 0 if equal, >0 if s1>s2
	;
	; Strategy: HL = s1, DE = s2, counter on stack.
	; Compare using MOV A,M (s1) and LDAX D (s2).

	; Load n into BC (temporary)
#ifdef UNDOC
	ldsi	6
	lhlx
	mov	c, l		; n low
	mov	b, h		; n high
#else
	lxi	h, 6
	dad	sp
	mov	c, m		; n low
	inx	h
	mov	b, m		; n high
#endif

	; Check if n == 0
	mov	a, b
	ora	c
	jz	.Lmemcmp_equal

	; Push n as our counter
	push	b		; [SP] = n

	; Load s2 into DE  (stack offsets shifted by 2 from push)
#ifdef UNDOC
	ldsi	6
	lhlx
	mov	e, l		; s2 low
	mov	d, h		; s2 high
#else
	lxi	h, 6
	dad	sp
	mov	e, m		; s2 low
	inx	h
	mov	d, m		; s2 high
#endif

	; Load s1 into HL
	lxi	h, 4
	dad	sp
	mov	a, m		; s1 low
	inx	h
	mov	h, m		; s1 high
	mov	l, a		; HL = s1

	; HL = s1, DE = s2, counter at [SP+0..1]
.Lmemcmp_loop:
	mov	a, m		; A = *s1
	push	psw		; save *s1 and flags
	ldax	d		; A = *s2
	mov	b, a		; B = *s2
	pop	psw		; A = *s1
	sub	b		; A = *s1 - *s2
	jnz	.Lmemcmp_diff

	; Bytes are equal, advance pointers
	inx	h		; s1++
	inx	d		; s2++

	; Decrement counter on stack
	pop	b		; BC = counter
	dcx	b
	mov	a, b
	ora	c
	jz	.Lmemcmp_eq_pop	; counter == 0, all equal
	push	b		; push counter back
	jmp	.Lmemcmp_loop

.Lmemcmp_diff:
	; A = *s1 - *s2 (non-zero). Clean up stack (counter).
	pop	b		; discard counter
	jc	.Lmemcmp_less
	; *s1 > *s2: return 1 in BC
	lxi	b, 1
	ret
.Lmemcmp_less:
	; *s1 < *s2: return -1 in BC
	lxi	b, 0xFFFF
	ret

.Lmemcmp_eq_pop:
	; All bytes equal (counter already popped)
.Lmemcmp_equal:
	lxi	b, 0
	ret
	.size	memcmp, .-memcmp
