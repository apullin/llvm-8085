; Hand-written IEEE 754 single-precision soft-float for i8085.
;
; IEEE 754 binary32 layout (little-endian in memory):
;   byte0 (LSB): mantissa[7:0]
;   byte1:       mantissa[15:8]
;   byte2:       exp[0] | mantissa[22:16]
;   byte3 (MSB): sign | exp[7:1]
;
; Calling convention:
;   [SP+0..1] = return address
;   [SP+2..5] = arg1 (4 bytes, little-endian)
;   [SP+6..9] = arg2 (4 bytes)
;   Return: 16-bit in BC, 32-bit in BC:DE (C=byte0, B=byte1, E=byte2, D=byte3)

	.text

; ============================================================
; HELPERS
; ============================================================

; Load 32-bit value from [HL] into C:B:E:D (byte0..byte3).
; Advances HL by 3.
.Lload32:
	mov	c, m		; byte0
	inx	h
	mov	b, m		; byte1
	inx	h
	mov	e, m		; byte2
	inx	h
	mov	d, m		; byte3
	ret

; Extract 8-bit exponent from float in C:B:E:D.
; Returns exponent in A (0..255).
; exp = ((D & 0x7F) << 1) | (E >> 7)
.Lget_exp_regs:
	mov	a, d
	ani	0x7F		; exp[7:1]
	rlc			; A <<= 1 (bit0 is old bit7 = junk)
	ani	0xFE		; clear bit0
	mov	h, a		; save
	mov	a, e		; byte2
	rlc			; carry = byte2 bit7 = exp[0]
	mov	a, h
	aci	0		; A += carry = full exponent
	ret

; Check if float in C:B:E:D is NaN.
; Returns NZ if NaN, Z if not NaN.
; Clobbers A, H.
.Lcheck_nan_regs:
	call	.Lget_exp_regs
	cpi	0xFF
	jnz	.Lcnr_not_nan
	; exp == 0xFF. NaN if mantissa != 0.
	mov	a, e
	ani	0x7F		; mantissa[22:16]
	ora	b		; | mantissa[15:8]
	ora	c		; | mantissa[7:0]
	ret			; NZ if any mantissa bit set (NaN), Z if 0 (Inf)
.Lcnr_not_nan:
	xra	a		; force Z
	ret

; ============================================================
; float __negsf2(float a)
; Flip sign bit.
; ============================================================
	.globl	__negsf2
	.type	__negsf2,@function
__negsf2:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32
#else
	lxi	h, 2
	dad	sp
	call	.Lload32
#endif
	mov	a, d
	xri	0x80
	mov	d, a
	ret
	.size	__negsf2, .-__negsf2

; ============================================================
; float __subsf3(float a, float b)
; Flip b's sign, then add.
; ============================================================
	.globl	__subsf3
	.type	__subsf3,@function
__subsf3:
	lxi	h, 9
	dad	sp
	mov	a, m		; b.byte3
	xri	0x80
	mov	m, a
	jmp	__addsf3
	.size	__subsf3, .-__subsf3

; ============================================================
; int __unordsf2(float a, float b)
; Returns nonzero if either arg is NaN.
; ============================================================
	.globl	__unordsf2
	.type	__unordsf2,@function
__unordsf2:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32		; load a
#else
	lxi	h, 2
	dad	sp
	call	.Lload32	; load a
#endif
	call	.Lcheck_nan_regs
	jnz	.Lunord_yes
	lxi	h, 6
	dad	sp
	call	.Lload32	; load b
	call	.Lcheck_nan_regs
	jnz	.Lunord_yes
	lxi	b, 0
	lxi	d, 0
	ret
.Lunord_yes:
	lxi	b, 1
	lxi	d, 0
	ret
	.size	__unordsf2, .-__unordsf2

; ============================================================
; int __lesf2(float a, float b)
; Also: __eqsf2, __ltsf2, __nesf2, __cmpsf2
; Returns -1 if a<b, 0 if a==b, +1 if a>b or unordered(NaN).
; ============================================================
	.globl	__lesf2
	.globl	__eqsf2
	.globl	__ltsf2
	.globl	__nesf2
	.globl	__cmpsf2
	.type	__lesf2,@function
	.type	__eqsf2,@function
	.type	__ltsf2,@function
	.type	__nesf2,@function
	.type	__cmpsf2,@function
__lesf2:
__eqsf2:
__ltsf2:
__nesf2:
__cmpsf2:
	; NaN check -> return +1
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32
#else
	lxi	h, 2
	dad	sp
	call	.Lload32
#endif
	call	.Lcheck_nan_regs
	jnz	.Lcmp_plus1
	lxi	h, 6
	dad	sp
	call	.Lload32
	call	.Lcheck_nan_regs
	jnz	.Lcmp_plus1
	jmp	.Lcmp_body
.Lcmp_plus1:
	lxi	b, 1
	lxi	d, 0
	ret
	.size	__lesf2, .-__lesf2

; ============================================================
; int __gesf2(float a, float b)
; Also: __gtsf2
; Returns -1 if a<b or unordered(NaN), 0 if a==b, +1 if a>b.
; ============================================================
	.globl	__gesf2
	.globl	__gtsf2
	.type	__gesf2,@function
	.type	__gtsf2,@function
__gesf2:
__gtsf2:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32
#else
	lxi	h, 2
	dad	sp
	call	.Lload32
#endif
	call	.Lcheck_nan_regs
	jnz	.Lcmp_minus1
	lxi	h, 6
	dad	sp
	call	.Lload32
	call	.Lcheck_nan_regs
	jnz	.Lcmp_minus1
	jmp	.Lcmp_body
.Lcmp_minus1:
	lxi	b, 0xFFFF
	lxi	d, 0xFFFF
	ret
	.size	__gesf2, .-__gesf2

; Shared comparison body.
; Both a and b are not NaN.
.Lcmp_body:
	; Check if both are +/-0
	; |a| = a with sign cleared, |b| = b with sign cleared
	; Both zero when |a| | |b| == 0
	lxi	h, 2
	dad	sp
	mov	a, m		; a.byte0
	inx	h
	ora	m		; | a.byte1
	inx	h
	ora	m		; | a.byte2
	inx	h
	mov	d, m		; a.byte3
	mov	b, a		; B = a.byte0|1|2
	mov	a, d
	ani	0x7F		; mask sign
	ora	b		; A = |a| nonzero bits (bytes 0-3 with sign masked)
	mov	b, a		; B = |a| summary

	lxi	h, 6
	dad	sp
	mov	a, m		; b.byte0
	inx	h
	ora	m
	inx	h
	ora	m
	inx	h
	mov	d, m		; b.byte3
	mov	c, a		; C = b.byte0|1|2
	mov	a, d
	ani	0x7F
	ora	c
	ora	b		; A = |a| | |b|
	jz	.Lcmp_eq	; both +/-0

	; Get signs
	lxi	h, 5
	dad	sp
	mov	a, m		; a.byte3
	ani	0x80
	mov	b, a		; B = a_sign (0x00 or 0x80)

	lxi	h, 9
	dad	sp
	mov	a, m		; b.byte3
	ani	0x80
	xra	b		; A = a_sign XOR b_sign
	jnz	.Lcmp_diffsign

	; Same sign: compare byte-by-byte from MSB (unsigned)
	lxi	h, 5
	dad	sp
	mov	d, m		; a.byte3
	lxi	h, 9
	dad	sp
	mov	a, d
	cmp	m		; a.byte3 - b.byte3
	jnz	.Lcmp_ne

	lxi	h, 4
	dad	sp
	mov	d, m		; a.byte2
	lxi	h, 8
	dad	sp
	mov	a, d
	cmp	m
	jnz	.Lcmp_ne

	lxi	h, 3
	dad	sp
	mov	d, m		; a.byte1
	lxi	h, 7
	dad	sp
	mov	a, d
	cmp	m
	jnz	.Lcmp_ne

	lxi	h, 2
	dad	sp
	mov	d, m		; a.byte0
	lxi	h, 6
	dad	sp
	mov	a, d
	cmp	m
	jnz	.Lcmp_ne

.Lcmp_eq:
	lxi	b, 0
	lxi	d, 0
	ret

.Lcmp_ne:
	; CMP result: carry set means a < b (unsigned)
	; For positive floats: unsigned order == float order
	; For negative floats: unsigned order is REVERSED
	jc	.Lcmp_a_lt_unsigned
	; a > b unsigned
	mov	a, b		; sign
	ora	a
	jz	.Lcmp_ret_p1	; positive: a > b -> +1
	jmp	.Lcmp_ret_m1	; negative: reversed -> -1

.Lcmp_a_lt_unsigned:
	mov	a, b
	ora	a
	jz	.Lcmp_ret_m1	; positive: a < b -> -1
	jmp	.Lcmp_ret_p1	; negative: reversed -> +1

.Lcmp_diffsign:
	; Signs differ. B = a_sign. If a is negative, a < b.
	mov	a, b
	ora	a
	jnz	.Lcmp_ret_m1	; a negative -> a < b
.Lcmp_ret_p1:
	lxi	b, 1
	lxi	d, 0
	ret
.Lcmp_ret_m1:
	lxi	b, 0xFFFF
	lxi	d, 0xFFFF
	ret

; ============================================================
; uint32_t __fixunssfsi(float a)
; float -> unsigned int32.  Negative -> 0, overflow -> 0xFFFFFFFF.
; ============================================================
	.globl	__fixunssfsi
	.type	__fixunssfsi,@function
__fixunssfsi:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 2
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Check sign -> if negative, return 0
	mov	a, d
	ani	0x80
	jnz	.Lfix_ret_zero

	; Get exponent
	call	.Lget_exp_regs	; A = biased exponent
	sui	127
	jc	.Lfix_ret_zero	; exp < 0 -> 0
	cpi	32
	jnc	.Lfix_ret_max	; exp >= 32 -> saturate

	; A = unbiased exponent (0..31)
	; Set implicit bit in mantissa
	mov	h, a		; H = exponent (save)
	mov	a, e
	ori	0x80		; set implicit bit
	mov	e, a
	; significand = 0:E:B:C (24-bit with implicit bit at position 23)

	; Push significand onto stack as scratch
	push	d		; D(=byte3, junk for mantissa):E(=byte2+implicit)
	push	b		; B(=byte1):C(=byte0)
	; Stack: [SP+0]=C, [SP+1]=B, [SP+2]=E, [SP+3]=D(junk)
	; Clear byte3 (D was sign+exp, not part of mantissa)
	lxi	h, 3
	dad	sp
	mvi	m, 0		; byte3 = 0

	; Now scratch[0..3] = significand (little-endian), implicit bit at position 23
	; We need: result = significand >> (23 - exponent) if exp <= 23
	;          result = significand << (exponent - 23) if exp > 23

	; Recover exponent from before push. It was in H before the pushes.
	; H was clobbered by dad sp. Recalculate.
	; Re-read from original arg on stack (offset shifted by 4 due to push d + push b)
	lxi	h, 6
	dad	sp		; -> arg byte0
	call	.Lload32	; reload a into C:B:E:D
	call	.Lget_exp_regs	; A = biased exponent
	sui	127		; A = unbiased exponent

	cpi	23
	jz	.Lfix_exact
	jc	.Lfix_shr	; exp < 23: shift right
	; exp > 23: shift left
	sui	23		; A = shift amount
	mov	e, a		; E = shift count
.Lfix_shl_loop:
	mov	a, e
	ora	a
	jz	.Lfix_done
	; Left-shift [SP+0..3] by 1
	lxi	h, 0
	dad	sp
	mov	a, m		; byte0
	add	a
	mov	m, a
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
	dcr	e
	jmp	.Lfix_shl_loop

.Lfix_shr:
	; Shift right by (23 - A) bits
	mov	e, a		; E = exponent
	mvi	a, 23
	sub	e		; A = shift count
	mov	e, a		; E = shift count
.Lfix_shr_loop:
	mov	a, e
	ora	a
	jz	.Lfix_done
	; Right-shift [SP+0..3] by 1
	lxi	h, 3
	dad	sp
	mov	a, m		; byte3
	ora	a		; clear carry
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
	dcr	e
	jmp	.Lfix_shr_loop

.Lfix_exact:
	; No shift needed
.Lfix_done:
	; Load result from stack scratch
	lxi	h, 0
	dad	sp
	call	.Lload32	; C:B:E:D = result
	; Clean up stack (4 bytes)
	pop	h
	pop	h
	ret

.Lfix_ret_zero:
	lxi	b, 0
	lxi	d, 0
	ret
.Lfix_ret_max:
	lxi	b, 0xFFFF
	lxi	d, 0xFFFF
	ret
	.size	__fixunssfsi, .-__fixunssfsi

; ============================================================
; int32_t __fixsfsi(float a)
; float -> signed int32.
; ============================================================
	.globl	__fixsfsi
	.type	__fixsfsi,@function
__fixsfsi:
	; Load a
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 2
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Save sign
	mov	a, d
	ani	0x80
	push	psw		; save sign on stack

	; Clear sign bit in arg so fixunssfsi sees positive
	; arg is at [SP+4..7] (2 pushed bytes + 2 ret addr bytes)
	lxi	h, 7
	dad	sp		; -> arg byte3
	mov	a, m
	ani	0x7F		; clear sign
	mov	m, a

	; Get exponent to check range
	lxi	h, 4
	dad	sp
	call	.Lload32	; reload |a|
	call	.Lget_exp_regs	; A = biased exponent
	sui	127
	jc	.Lfixs_zero	; exp < 0
	cpi	31
	jnc	.Lfixs_saturate	; exp >= 31

	; Push |a| as args for __fixunssfsi
	lxi	h, 7
	dad	sp
	mov	d, m
	dcx	h
	mov	e, m
	dcx	h
	mov	b, m
	dcx	h
	mov	c, m
	push	d		; byte3:byte2
	push	b		; byte1:byte0
	call	__fixunssfsi
	pop	h		; clean args
	pop	h

	; Result in BC:DE = |integer value|
	; Pop sign
	pop	psw		; A[7] = sign
	ani	0x80
	jz	.Lfixs_ret	; positive, return as-is

	; Negate BC:DE (two's complement)
	mov	a, c
	cma
	adi	1
	mov	c, a
	mov	a, b
	cma
	aci	0
	mov	b, a
	mov	a, e
	cma
	aci	0
	mov	e, a
	mov	a, d
	cma
	aci	0
	mov	d, a
.Lfixs_ret:
	ret

.Lfixs_zero:
	pop	psw		; clean sign
	lxi	b, 0
	lxi	d, 0
	ret
.Lfixs_saturate:
	pop	psw		; A[7] = sign
	ani	0x80
	jnz	.Lfixs_sat_neg
	; Positive: 0x7FFFFFFF
	lxi	b, 0xFFFF
	lxi	d, 0x7FFF
	ret
.Lfixs_sat_neg:
	; Negative: 0x80000000
	lxi	b, 0x0000
	lxi	d, 0x8000
	ret
	.size	__fixsfsi, .-__fixsfsi

; ============================================================
; float __floatunsisf(uint32_t a)
; unsigned int32 -> float
; ============================================================
	.globl	__floatunsisf
	.type	__floatunsisf,@function
__floatunsisf:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 2
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Check if a == 0
	mov	a, c
	ora	b
	ora	e
	ora	d
	jnz	.Lfunsisf_nz
	lxi	b, 0
	lxi	d, 0
	ret

.Lfunsisf_nz:
	; Push a onto stack as scratch
	push	d		; byte3:byte2
	push	b		; byte1:byte0
	; scratch at [SP+0..3]

	; Find highest set bit (CLZ) to determine exponent.
	; Check bytes from MSB.
	lxi	h, 3
	dad	sp
	mov	a, m		; byte3
	ora	a
	jnz	.Lfunsisf_clz_b3
	dcx	h
	mov	a, m		; byte2
	ora	a
	jnz	.Lfunsisf_clz_b2
	dcx	h
	mov	a, m		; byte1
	ora	a
	jnz	.Lfunsisf_clz_b1
	dcx	h
	mov	a, m		; byte0 (must be nonzero)
	mvi	d, 24		; CLZ offset
	jmp	.Lfunsisf_clz8

.Lfunsisf_clz_b3:
	mvi	d, 0
	jmp	.Lfunsisf_clz8
.Lfunsisf_clz_b2:
	mvi	d, 8
	jmp	.Lfunsisf_clz8
.Lfunsisf_clz_b1:
	mvi	d, 16

.Lfunsisf_clz8:
	; A = nonzero byte, D = byte offset (0/8/16/24)
	; Count leading zeros of A (shift left until bit7 set)
	mvi	e, 0
.Lfunsisf_clz8_lp:
	ora	a		; test if bit7 is set (sign flag... no, 8085 has no sign flag test)
	rlc			; bit7 -> carry, shift left
	jc	.Lfunsisf_clz_found
	inr	e
	jmp	.Lfunsisf_clz8_lp

.Lfunsisf_clz_found:
	; Total CLZ = D + E
	mov	a, d
	add	e		; A = total CLZ
	; exponent = 31 - CLZ (unbiased)
	mov	d, a		; save CLZ
	mvi	a, 31
	sub	d		; A = exponent (unbiased, 0..31)
	mov	d, a		; D = exponent

	; Now shift significand so implicit bit is at position 23.
	; Currently highest bit is at position (31 - CLZ) = exponent.
	; Need to move it to position 23.
	; If exponent < 23: left-shift by (23 - exponent)
	; If exponent == 23: no shift
	; If exponent > 23: right-shift by (exponent - 23)
	mov	a, d		; exponent
	cpi	24
	jnc	.Lfunsisf_shr
	cpi	23
	jz	.Lfunsisf_pack

	; Left-shift by (23 - exponent)
	mvi	a, 23
	sub	d		; A = shift count
	mov	e, a		; E = shift count

.Lfunsisf_shl_lp:
	mov	a, e
	ora	a
	jz	.Lfunsisf_pack
	; Left-shift [SP+0..3] by 1
	lxi	h, 0
	dad	sp
	mov	a, m		; byte0
	add	a		; << 1
	mov	m, a
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
	dcr	e
	jmp	.Lfunsisf_shl_lp

.Lfunsisf_shr:
	; Right-shift by (exponent - 23), with IEEE 754 rounding.
	; For 32-bit unsigned, shift count is 1..8.
	; Track guard and sticky bits for round-to-nearest-even.
	mov	a, d
	sui	23		; shift count (1..8)
	mov	e, a		; E = shift count

	; Save original byte0 before shifting (contains bits to be shifted out).
	; Use C register (not needed for significand shifting).
	lxi	h, 0
	dad	sp
	mov	c, m		; C = original byte0

	; Shift loop: right-shift [SP+0..3] by E bits
.Lfunsisf_shr_lp:
	mov	a, e
	ora	a
	jz	.Lfunsisf_shr_round
	; Right-shift [SP+0..3] by 1
	lxi	h, 3
	dad	sp
	mov	a, m		; byte3
	ora	a		; clear carry
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
	dcr	e
	jmp	.Lfunsisf_shr_lp

.Lfunsisf_shr_round:
	; C = original byte0 (contains the bits that were shifted out).
	; D = exponent, shift_count = D - 23.
	; The shifted-out bits are the low (shift_count) bits of C.
	; Guard bit = bit (shift_count - 1) of C.
	; Sticky = OR of bits (shift_count - 2) down to 0 of C.
	;
	; Build half = 1 << (shift_count - 1) = guard bit position mask
	mov	a, d
	sui	23		; A = shift count
	mov	e, a		; E = shift count
	mvi	a, 1
	dcr	e
	jz	.Lfunsisf_half_ok
.Lfunsisf_half_mk:
	add	a		; shift left
	dcr	e
	jnz	.Lfunsisf_half_mk
.Lfunsisf_half_ok:
	; A = half = 1 << (shift_count - 1)
	mov	b, a		; B = half (guard bit mask)

	; Build mask = (1 << shift_count) - 1
	add	a		; A = 1 << shift_count
	dcr	a		; A = mask
	mov	e, a		; E = mask

	; Extract shifted-out bits from original byte0
	mov	a, c		; A = original byte0
	ana	e		; A = shifted_out = orig_byte0 & mask
	jz	.Lfunsisf_pack	; shifted_out == 0, no rounding needed

	; A = shifted_out, B = half
	; Compare shifted_out with half
	cmp	b
	jc	.Lfunsisf_pack	; shifted_out < half, truncate (no round)
	jnz	.Lfunsisf_roundup ; shifted_out > half, round up

	; shifted_out == half (exact tie): round to even
	; Round up only if LSB of result is 1 (making it even)
	lxi	h, 0
	dad	sp
	mov	a, m		; result byte0
	ani	1		; test LSB
	jz	.Lfunsisf_pack	; LSB=0, already even, truncate

.Lfunsisf_roundup:
	; Add 1 to [SP+0..3] (round up the significand)
	lxi	h, 0
	dad	sp
	mov	a, m		; byte0
	adi	1
	mov	m, a
	jnc	.Lfunsisf_pack	; no carry, done
	inx	h
	mov	a, m		; byte1
	adi	0
	aci	0
	mov	m, a
	jnc	.Lfunsisf_pack
	inx	h
	mov	a, m		; byte2
	aci	0
	mov	m, a
	jnc	.Lfunsisf_pack
	inx	h
	mov	a, m		; byte3
	aci	0
	mov	m, a
	; If carry out of byte3, the significand overflowed (became 0x01000000).
	; This means implicit bit moved up, exponent increases by 1.
	; For uint32 this can only happen for 0xFFFFFF80..0xFFFFFFFF (rare).
	jnc	.Lfunsisf_pack
	; Significand overflowed: [SP+0..3] = 0x01000000
	; Right-shift by 1 to get 0x00800000, increment exponent
	ora	a		; clear carry
	rar
	mov	m, a		; byte3
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
	inr	d		; exponent += 1

.Lfunsisf_pack:
	; Significand in [SP+0..3], implicit bit at position 23.
	; Clear implicit bit (byte2 bit 7).
	lxi	h, 2
	dad	sp
	mov	a, m
	ani	0x7F		; clear implicit bit
	mov	m, a

	; Biased exponent = D + 127
	mov	a, d
	adi	127		; A = biased exponent (0..255)
	; exp field: byte2[7] = biased_exp[0], byte3[6:0] = biased_exp[7:1]
	mov	e, a		; save biased exp

	; Set byte2 bit7 = exp[0]
	ani	1		; get bit0
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc			; rotate to bit7
	mov	d, a		; D = exp[0] << 7
	lxi	h, 2
	dad	sp
	mov	a, m
	ora	d		; OR in exp[0] at bit7
	mov	m, a

	; Set byte3 = exp[7:1] (sign=0 for unsigned)
	mov	a, e		; biased exp
	ora	a		; clear carry
	rar			; A = exp >> 1
	ani	0x7F		; clear bit7 (sign = 0)
	lxi	h, 3
	dad	sp
	mov	m, a

	; Load result into BC:DE
	lxi	h, 0
	dad	sp
	call	.Lload32

	; Clean up stack (4 bytes)
	pop	h
	pop	h
	ret
	.size	__floatunsisf, .-__floatunsisf

; ============================================================
; float __floatsisf(int32_t a)
; signed int32 -> float
; ============================================================
	.globl	__floatsisf
	.type	__floatsisf,@function
__floatsisf:
#ifdef UNDOC
	ldsi	2
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 2
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Check if a == 0
	mov	a, c
	ora	b
	ora	e
	ora	d
	jnz	.Lfsisf_nz
	lxi	b, 0
	lxi	d, 0
	ret

.Lfsisf_nz:
	; Save sign and compute absolute value
	mov	a, d
	ani	0x80
	push	psw		; save sign (0x00 or 0x80) on stack

	; If negative, negate C:B:E:D
	ora	a
	jz	.Lfsisf_pos
	mov	a, c
	cma
	adi	1
	mov	c, a
	mov	a, b
	cma
	aci	0
	mov	b, a
	mov	a, e
	cma
	aci	0
	mov	e, a
	mov	a, d
	cma
	aci	0
	mov	d, a

.Lfsisf_pos:
	; Push abs(a) as args for __floatunsisf
	; __floatunsisf expects [SP+2..5] = arg
	; After CALL, its [SP+2..5] = our current [SP+0..3]
	; So we push abs(a) onto stack, then call.
	push	d		; byte3:byte2
	push	b		; byte1:byte0
	call	__floatunsisf
	; Result in BC:DE
	pop	h		; clean pushed args
	pop	h

	; Pop sign
	pop	psw		; A[7] = sign
	; OR sign into result byte3 (D)
	ora	d
	mov	d, a
	ret
	.size	__floatsisf, .-__floatsisf

; ============================================================
; FP environment stubs
; ============================================================
	.globl	__fe_getround
	.type	__fe_getround,@function
__fe_getround:
	lxi	b, 0
	ret
	.size	__fe_getround, .-__fe_getround

	.globl	__fe_raise_inexact
	.type	__fe_raise_inexact,@function
__fe_raise_inexact:
	ret
	.size	__fe_raise_inexact, .-__fe_raise_inexact

; ============================================================
; float __addsf3(float a, float b)
;
; IEEE 754 addition.
; Stack scratch layout (12 bytes allocated):
;   [SP+0..2]  = mantissa_a (3 bytes, 24-bit with implicit)
;   [SP+3]     = exponent_a (biased, 8-bit)
;   [SP+4..6]  = mantissa_b (3 bytes)
;   [SP+7]     = exponent_b (biased, 8-bit)
;   [SP+8]     = sign_a (0x00 or 0x80)
;   [SP+9]     = sign_b (0x00 or 0x80)
;   [SP+10..11] = (pad to keep even)
; Original args at [SP+12..19] after allocating scratch.
; ============================================================
	.globl	__addsf3
	.type	__addsf3,@function
__addsf3:
	; Allocate 12 bytes of scratch on stack
	lxi	h, -12
	dad	sp
	sphl			; SP -= 12

	; Load a from [SP+14..17] (12 scratch + 2 ret addr)
#ifdef UNDOC
	ldsi	14
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 14
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Extract sign_a
	mov	a, d
	ani	0x80
	lxi	h, 8
	dad	sp
	mov	m, a		; [SP+8] = sign_a

	; Extract exponent_a
	call	.Lget_exp_regs	; A = biased exp
	lxi	h, 3
	dad	sp
	mov	m, a		; [SP+3] = exp_a

	; Extract mantissa_a with implicit bit (if exp != 0)
	ora	a
	jz	.Ladd_a_zero_or_denorm
	; Normal: set implicit bit
	mov	a, e
	ori	0x80
	mov	e, a
.Ladd_a_store_mant:
	lxi	h, 0
	dad	sp
	mov	m, c		; mant_a[0]
	inx	h
	mov	m, b		; mant_a[1]
	inx	h
	mov	m, e		; mant_a[2] (with implicit bit)

	; Load b from [SP+18..21]
	lxi	h, 18
	dad	sp
	call	.Lload32	; C:B:E:D = b

	; Extract sign_b
	mov	a, d
	ani	0x80
	lxi	h, 9
	dad	sp
	mov	m, a		; [SP+9] = sign_b

	; Extract exponent_b
	call	.Lget_exp_regs
	lxi	h, 7
	dad	sp
	mov	m, a		; [SP+7] = exp_b

	; Extract mantissa_b with implicit bit
	ora	a
	jz	.Ladd_b_zero_or_denorm
	mov	a, e
	ori	0x80
	mov	e, a
.Ladd_b_store_mant:
	lxi	h, 4
	dad	sp
	mov	m, c		; mant_b[0]
	inx	h
	mov	m, b		; mant_b[1]
	inx	h
	mov	m, e		; mant_b[2]

	; --- Check special cases ---
	; If exp_a == 0xFF: a is Inf or NaN
	lxi	h, 3
	dad	sp
	mov	a, m		; exp_a
	cpi	0xFF
	jz	.Ladd_a_special
	; If exp_b == 0xFF: b is Inf or NaN
	lxi	h, 7
	dad	sp
	mov	a, m
	cpi	0xFF
	jz	.Ladd_b_special

	; If exp_a == 0: a is zero (denormals treated as zero)
	lxi	h, 3
	dad	sp
	mov	a, m
	ora	a
	jz	.Ladd_a_is_zero
	; If exp_b == 0: b is zero
	lxi	h, 7
	dad	sp
	mov	a, m
	ora	a
	jz	.Ladd_b_is_zero

	; --- Normal addition ---
	; Ensure exp_a >= exp_b (swap if needed)
	lxi	h, 3
	dad	sp
	mov	a, m		; exp_a
	lxi	h, 7
	dad	sp
	cmp	m		; exp_a - exp_b
	jnc	.Ladd_no_swap	; exp_a >= exp_b

	; Swap a and b: swap mantissa, exponent, sign
	; Swap exponents
	lxi	h, 3
	dad	sp
	mov	a, m		; exp_a
	mov	b, a
	lxi	h, 7
	dad	sp
	mov	c, m		; exp_b
	mov	m, b		; [SP+7] = old exp_a
	lxi	h, 3
	dad	sp
	mov	m, c		; [SP+3] = old exp_b

	; Swap mantissas (3 bytes each at [SP+0..2] and [SP+4..6])
	lxi	h, 0
	dad	sp
	mov	a, m		; mant_a[0]
	mov	b, a
	lxi	h, 4
	dad	sp
	mov	c, m		; mant_b[0]
	mov	m, b
	lxi	h, 0
	dad	sp
	mov	m, c

	lxi	h, 1
	dad	sp
	mov	a, m
	mov	b, a
	lxi	h, 5
	dad	sp
	mov	c, m
	mov	m, b
	lxi	h, 1
	dad	sp
	mov	m, c

	lxi	h, 2
	dad	sp
	mov	a, m
	mov	b, a
	lxi	h, 6
	dad	sp
	mov	c, m
	mov	m, b
	lxi	h, 2
	dad	sp
	mov	m, c

	; Swap signs
	lxi	h, 8
	dad	sp
	mov	a, m
	mov	b, a
	lxi	h, 9
	dad	sp
	mov	c, m
	mov	m, b
	lxi	h, 8
	dad	sp
	mov	m, c

.Ladd_no_swap:
	; Now exp_a >= exp_b.
	; Shift mant_b right by (exp_a - exp_b) to align.
	lxi	h, 3
	dad	sp
	mov	a, m		; exp_a
	lxi	h, 7
	dad	sp
	sub	m		; A = exp_a - exp_b = shift count
	jz	.Ladd_aligned	; same exponent, no shift needed
	; Cap shift at 24 (beyond that, mant_b is zero)
	cpi	25
	jnc	.Ladd_b_shifted_out
	mov	e, a		; E = shift count

.Ladd_align_loop:
	mov	a, e
	ora	a
	jz	.Ladd_aligned
	; Right-shift mant_b [SP+4..6] by 1
	lxi	h, 6
	dad	sp
	mov	a, m		; mant_b[2]
	ora	a		; clear carry
	rar
	mov	m, a
	dcx	h
	mov	a, m		; mant_b[1]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; mant_b[0]
	rar
	mov	m, a
	dcr	e
	jmp	.Ladd_align_loop

.Ladd_b_shifted_out:
	; mant_b is completely shifted out, result is just a
	; Load sign_a into result and pack
	jmp	.Ladd_return_a

.Ladd_aligned:
	; Check if signs match
	lxi	h, 8
	dad	sp
	mov	a, m		; sign_a
	lxi	h, 9
	dad	sp
	xra	m		; sign_a XOR sign_b
	jnz	.Ladd_subtract

	; --- SAME SIGN: add mantissas ---
	; result_mant = mant_a + mant_b (3 bytes)
	; Load mant_b into registers first, then add in-place using inx h
	; (avoids dad sp between adc and jnc, since dad clobbers carry)
	lxi	h, 4
	dad	sp
	mov	c, m		; mant_b[0]
	inx	h
	mov	b, m		; mant_b[1]
	inx	h
	mov	e, m		; mant_b[2]

	lxi	h, 0
	dad	sp		; HL -> mant_a[0]
	mov	a, m		; mant_a[0]
	add	c		; + mant_b[0]
	mov	m, a		; store result[0]
	inx	h		; HL -> mant_a[1] (no flag change)
	mov	a, m		; mant_a[1]
	adc	b		; + mant_b[1] + carry
	mov	m, a		; store result[1]
	inx	h		; HL -> mant_a[2] (no flag change)
	mov	a, m		; mant_a[2]
	adc	e		; + mant_b[2] + carry
	mov	m, a		; store result[2]

	; If carry out, shift right by 1 and bump exponent
	; (carry flag still valid — inx/mov don't affect it)
	jnc	.Ladd_normalize
	; Carry: shift result right by 1, set bit23
	lxi	h, 2
	dad	sp
	mov	a, m
	stc			; set carry (the carry-out bit)
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
	; Increment exponent
	lxi	h, 3
	dad	sp
	mov	a, m
	inr	a
	mov	m, a
	cpi	0xFF
	jz	.Ladd_overflow_inf
	jmp	.Ladd_pack

.Ladd_subtract:
	; --- DIFFERENT SIGNS: subtract mantissas ---
	; result_mant = mant_a - mant_b (a has larger exponent)
	lxi	h, 4
	dad	sp
	mov	c, m		; mant_b[0]
	inx	h
	mov	b, m		; mant_b[1]
	inx	h
	mov	e, m		; mant_b[2]

	lxi	h, 0
	dad	sp
	mov	a, m		; mant_a[0]
	sub	c
	mov	m, a		; result[0]
	inx	h
	mov	a, m		; mant_a[1]
	sbb	b
	mov	m, a		; result[1]
	inx	h
	mov	a, m		; mant_a[2]
	sbb	e
	mov	m, a		; result[2]

	; If borrow (result negative), negate and flip sign
	jnc	.Ladd_sub_positive
	; Negate result (two's complement of 3 bytes)
	lxi	h, 0
	dad	sp
	mov	a, m
	cma
	adi	1
	mov	m, a
	inx	h
	mov	a, m
	cma
	aci	0
	mov	m, a
	inx	h
	mov	a, m
	cma
	aci	0
	mov	m, a
	; Flip result sign: use sign_b instead of sign_a
	lxi	h, 9
	dad	sp
	mov	a, m		; sign_b
	lxi	h, 8
	dad	sp
	mov	m, a		; sign_a = sign_b

.Ladd_sub_positive:
	; Check if result is zero
	lxi	h, 0
	dad	sp
	mov	a, m
	inx	h
	ora	m
	inx	h
	ora	m
	jz	.Ladd_return_zero

	; Fall through to normalize
.Ladd_normalize:
	; Normalize: shift left until bit 23 (byte2 bit7) is set
	lxi	h, 2
	dad	sp
	mov	a, m		; result[2]
	ani	0x80
	jnz	.Ladd_pack	; already normalized

.Ladd_norm_loop:
	; Left-shift result [SP+0..2] by 1
	lxi	h, 0
	dad	sp
	mov	a, m		; byte0
	add	a
	mov	m, a
	inx	h
	mov	a, m		; byte1
	ral
	mov	m, a
	inx	h
	mov	a, m		; byte2
	ral
	mov	m, a
	; Decrement exponent
	lxi	h, 3
	dad	sp
	mov	a, m
	dcr	a
	mov	m, a
	; If exponent went to 0, result is denorm/zero
	ora	a
	jz	.Ladd_pack	; underflow, return denorm
	; Check if normalized
	lxi	h, 2
	dad	sp
	mov	a, m
	ani	0x80
	jz	.Ladd_norm_loop

.Ladd_pack:
	; Pack result from scratch into IEEE 754 format
	; mant in [SP+0..2] (implicit bit at position 23)
	; exp in [SP+3]
	; sign in [SP+8]

	; Clear implicit bit
	lxi	h, 2
	dad	sp
	mov	a, m
	ani	0x7F		; clear bit7
	mov	m, a

	; Build byte2: mantissa[22:16] | (exp[0] << 7)
	lxi	h, 3
	dad	sp
	mov	a, m		; exp
	ani	1		; exp[0]
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc			; bit0 -> bit7
	mov	d, a
	lxi	h, 2
	dad	sp
	mov	a, m
	ora	d
	mov	e, a		; E = byte2

	; Build byte3: sign | exp[7:1]
	lxi	h, 3
	dad	sp
	mov	a, m		; exp
	ora	a		; clear carry
	rar			; exp >> 1
	ani	0x7F		; clear sign bit
	mov	d, a
	lxi	h, 8
	dad	sp
	mov	a, m		; sign (0x00 or 0x80)
	ora	d
	mov	d, a		; D = byte3

	; Load byte0, byte1
	lxi	h, 0
	dad	sp
	mov	c, m		; byte0
	inx	h
	mov	b, m		; byte1

	; Result in C:B:E:D
	; Clean up stack (12 bytes)
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_return_a:
	; Return a (the larger operand) from scratch
	; Reload a from original arg
#ifdef UNDOC
	ldsi	14
	xchg
	call	.Lload32
#else
	lxi	h, 14
	dad	sp
	call	.Lload32
#endif
	; But we may have swapped, so we should pack from scratch instead
	jmp	.Ladd_pack

.Ladd_return_zero:
	lxi	b, 0
	lxi	d, 0
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_overflow_inf:
	; Return +/-Inf with sign_a
#ifdef UNDOC
	ldsi	8
	ldax	d		; sign
#else
	lxi	h, 8
	dad	sp
	mov	a, m		; sign
#endif
	ori	0x7F		; byte3 of Inf = sign | 0x7F
	mov	d, a
	mvi	e, 0x80		; byte2 of Inf = 0x80
	lxi	b, 0		; byte0:byte1 = 0
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_a_zero_or_denorm:
	; a exponent is 0, treat mantissa as 0 (no implicit bit for denorms)
	mvi	c, 0
	mvi	b, 0
	mvi	e, 0
	jmp	.Ladd_a_store_mant

.Ladd_b_zero_or_denorm:
	mvi	c, 0
	mvi	b, 0
	mvi	e, 0
	jmp	.Ladd_b_store_mant

.Ladd_a_is_zero:
	; a is zero, return b
	lxi	h, 18
	dad	sp
	call	.Lload32
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_b_is_zero:
	; b is zero, return a
#ifdef UNDOC
	ldsi	14
	xchg
	call	.Lload32
#else
	lxi	h, 14
	dad	sp
	call	.Lload32
#endif
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_a_special:
	; exp_a == 0xFF: a is Inf or NaN
	; If NaN, return NaN
	lxi	h, 0
	dad	sp
	mov	a, m
	inx	h
	ora	m
	inx	h
	mov	d, m
	mov	a, d
	ani	0x7F
	lxi	h, 0
	dad	sp
	ora	m
	inx	h
	ora	m
	jnz	.Ladd_ret_nan	; a is NaN
	; a is Inf. Check if b is also Inf with opposite sign
	lxi	h, 7
	dad	sp
	mov	a, m		; exp_b
	cpi	0xFF
	jnz	.Ladd_ret_a_special	; b is finite, return a (Inf)
	; b is also Inf or NaN
	lxi	h, 4
	dad	sp
	mov	a, m
	inx	h
	ora	m
	inx	h
	mov	d, m
	mov	a, d
	ani	0x7F
	lxi	h, 4
	dad	sp
	ora	m
	inx	h
	ora	m
	jnz	.Ladd_ret_nan	; b is NaN
	; Both Inf: if same sign return Inf, if different sign return NaN
	lxi	h, 8
	dad	sp
	mov	a, m		; sign_a
	lxi	h, 9
	dad	sp
	xra	m		; XOR sign_b
	jnz	.Ladd_ret_nan	; Inf - Inf = NaN
.Ladd_ret_a_special:
	; Return a from args
	lxi	h, 14
	dad	sp
	call	.Lload32
	lxi	h, 12
	dad	sp
	sphl
	ret
.Ladd_ret_nan:
	; Return canonical NaN: 0x7FC00000
	lxi	b, 0x0000
	lxi	d, 0x7FC0
	lxi	h, 12
	dad	sp
	sphl
	ret

.Ladd_b_special:
	; exp_b == 0xFF, exp_a is normal: return b (Inf or NaN)
#ifdef UNDOC
	ldsi	18
	xchg
	call	.Lload32
#else
	lxi	h, 18
	dad	sp
	call	.Lload32
#endif
	lxi	h, 12
	dad	sp
	sphl
	ret

	.size	__addsf3, .-__addsf3

; ============================================================
; float __mulsf3(float a, float b)
;
; IEEE 754 multiplication.
; result_sign = sign_a XOR sign_b
; result_exp = exp_a + exp_b - 127
; result_mant = mant_a * mant_b (24x24 -> 48, keep top 24)
;
; Stack scratch layout (16 bytes):
;   [SP+0..2]   = mantissa_a (3 bytes with implicit bit)
;   [SP+3]      = exponent_a
;   [SP+4..6]   = mantissa_b (3 bytes with implicit bit)
;   [SP+7]      = exponent_b
;   [SP+8]      = result_sign
;   [SP+9..14]  = product (6 bytes for 48-bit result)
;   [SP+15]     = pad
; Args at [SP+18..25] after 16 bytes scratch + 2 ret addr.
; ============================================================
	.globl	__mulsf3
	.type	__mulsf3,@function
__mulsf3:
	; Allocate 16 bytes scratch
	lxi	h, -16
	dad	sp
	sphl

	; Load a from [SP+18..21]
#ifdef UNDOC
	ldsi	18
	xchg
	call	.Lload32		; C:B:E:D = a
#else
	lxi	h, 18
	dad	sp
	call	.Lload32	; C:B:E:D = a
#endif

	; Compute result sign = sign_a XOR sign_b
	mov	a, d
	ani	0x80
	push	psw		; save sign_a temporarily
	; Load b byte3 for sign_b
	lxi	h, 27
	dad	sp		; b.byte3 at SP+25 before push, +2 for push = SP+27
	mov	a, m		; b.byte3
	ani	0x80
	mov	h, a		; H = sign_b
	pop	psw		; A = sign_a
	xra	h		; result_sign = sign_a XOR sign_b
	lxi	h, 8
	dad	sp
	mov	m, a		; [SP+8] = result_sign

	; Reload a
	lxi	h, 18
	dad	sp
	call	.Lload32

	; Extract exp_a
	call	.Lget_exp_regs
	lxi	h, 3
	dad	sp
	mov	m, a		; [SP+3] = exp_a

	; Check for zero/special
	ora	a
	jz	.Lmul_ret_zero	; a is zero -> result is zero

	; Set implicit bit and store mantissa_a
	mov	a, e
	ori	0x80
	mov	e, a
	lxi	h, 0
	dad	sp
	mov	m, c		; mant_a[0]
	inx	h
	mov	m, b		; mant_a[1]
	inx	h
	mov	m, e		; mant_a[2]

	; Load b
	lxi	h, 22
	dad	sp
	call	.Lload32
	call	.Lget_exp_regs
	lxi	h, 7
	dad	sp
	mov	m, a		; [SP+7] = exp_b
	ora	a
	jz	.Lmul_ret_zero	; b is zero

	; Check for Inf/NaN
	lxi	h, 3
	dad	sp
	mov	a, m
	cpi	0xFF
	jz	.Lmul_a_special
	lxi	h, 7
	dad	sp
	mov	a, m
	cpi	0xFF
	jz	.Lmul_b_special

	; Set implicit bit for b
	mov	a, e
	ori	0x80
	mov	e, a
	lxi	h, 4
	dad	sp
	mov	m, c		; mant_b[0]
	inx	h
	mov	m, b		; mant_b[1]
	inx	h
	mov	m, e		; mant_b[2]

	; Compute result exponent = exp_a + exp_b - 127
	lxi	h, 3
	dad	sp
	mov	a, m		; exp_a
	lxi	h, 7
	dad	sp
	add	m		; A = exp_a + exp_b (may overflow 8 bits)
	jc	.Lmul_exp_high	; if carry, exp > 255, check for overflow
	sui	127		; A = exp_a + exp_b - 127
	jc	.Lmul_ret_zero	; underflow
	lxi	h, 3
	dad	sp
	mov	m, a		; store result exp
	jmp	.Lmul_do_multiply

.Lmul_exp_high:
	; exp_a + exp_b > 255. True sum = A + 256.
	; Result exp = A + 256 - 127 = A + 129. Overflow if > 255.
	adi	129
	jc	.Lmul_overflow	; A + 129 > 255 means true exponent overflows
	lxi	h, 3
	dad	sp
	mov	m, a
	jmp	.Lmul_do_multiply

.Lmul_do_multiply:
	; Multiply mant_a (3 bytes at [SP+0..2]) by mant_b (3 bytes at [SP+4..6])
	; Result is 6 bytes (48 bits) stored at [SP+9..14]
	; Clear product area
	lxi	h, 9
	dad	sp
	mvi	m, 0
	inx	h
	mvi	m, 0
	inx	h
	mvi	m, 0
	inx	h
	mvi	m, 0
	inx	h
	mvi	m, 0
	inx	h
	mvi	m, 0

	; Long multiplication: for each byte of mant_b, multiply by each byte of mant_a
	; and accumulate into product.
	;
	; product += mant_a[i] * mant_b[j] << (8*(i+j))
	;
	; We do this byte-by-byte: 9 single-byte multiplications.
	; But 8085 has no MUL instruction. We'll use a simple shift-and-add multiply.
	;
	; Simpler approach: treat the 24-bit multiply as repeated addition.
	; mant_a * mant_b = sum over bits of mant_b: if bit[i] set, add mant_a << i.
	;
	; Use a 48-bit accumulator (product at [SP+9..14]).
	; Iterate 24 bits of mant_b.

	; Load mant_b into D:E:C (3 bytes) for bit testing
	; Actually, we'll keep mant_b in memory and test bits in a loop.

	mvi	d, 24		; bit counter
	; We'll shift mant_a left through the 48-bit space and test mant_b bits.
	; Copy mant_a to lower 3 bytes of a 6-byte shift register in product area.
	; Actually, use a different approach:
	; Start with mant_a in low 3 bytes of a 6-byte "addend",
	; iterate: test LSB of mant_b, if set add addend to product, shift addend left.
	; Then shift mant_b right.

	; Copy mant_a to temp 6-byte area. Reuse [SP+9..14] for accumulator (already zeroed).
	; We'll keep the shifting addend in registers, but 6 bytes won't fit.
	; Keep addend in memory too? This gets slow but correct.

	; Simplest correct approach: test each bit of mant_b, if set, add shifted mant_a.
	; For bit i (0..23): if mant_b[bit i] set, product += mant_a << i
	; Implement as: for each bit, right-shift mant_b and test carry, then add and left-shift mant_a.

	; Store mant_b into a 3-byte work area. Reuse [SP+4..6] which already has it.
	; Store a 6-byte "addend" that starts as mant_a and shifts left each iteration.
	; Need 6 more bytes... we don't have room. Let me use a different layout.

	; Alternative approach: shift-and-add using the product area.
	; Initialize a 6-byte addend at stack positions we can use.
	; We already have 16 bytes of scratch. Let me reorganize:
	;   [SP+0..2] = mant_a (input)
	;   [SP+3]    = result exponent
	;   [SP+4..6] = mant_b (will be shifted right)
	;   [SP+7]    = bit counter
	;   [SP+8]    = result_sign
	;   [SP+9..14] = product accumulator (6 bytes)
	;   [SP+15]   = pad

	mvi	a, 24
	lxi	h, 7
	dad	sp
	mov	m, a		; [SP+7] = 24 (bit counter, reusing exp_b slot)

.Lmul_bit_loop:
	; Test LSB of mant_b
	lxi	h, 4
	dad	sp
	mov	a, m		; mant_b[0]
	ani	1
	jz	.Lmul_no_add

	; Add mant_a to product at current position.
	; We need to add mant_a[0..2] to product, aligned to the current bit position.
	; But since we shift mant_b right each iteration, we need to shift the product
	; left instead. Alternative: accumulate from MSB.

	; Actually, let's use a cleaner approach:
	; product += mant_a (at fixed position), then shift product right.
	; After 24 iterations, the product will have accumulated correctly.
	; Wait, that's not quite right either.

	; Correct approach for shift-and-add:
	; For each bit of mant_b (from LSB):
	;   if bit set, add mant_a to product (at high end)
	;   right-shift product by 1
	; After 24 iterations, product holds the result.
	; But product needs to be 6 bytes to hold 48-bit result.

	; Add mant_a[0..2] to product[3..5] (upper 3 bytes of 6-byte product)
	lxi	h, 0
	dad	sp
	mov	c, m		; mant_a[0]
	inx	h
	mov	b, m		; mant_a[1]
	inx	h
	mov	e, m		; mant_a[2]

	lxi	h, 12
	dad	sp
	mov	a, m		; product[3]
	add	c
	mov	m, a
	inx	h
	mov	a, m		; product[4]
	adc	b
	mov	m, a
	inx	h
	mov	a, m		; product[5]
	adc	e
	mov	m, a
	; HL = SP+14 = &product[5], A = product[5], carry = overflow from add
	; Jump to shift MSB — carry is preserved (jmp doesn't affect flags)
	jmp	.Lmul_shift_msb

.Lmul_no_add:
	; Right-shift entire 6-byte product by 1 (no add overflow)
	lxi	h, 14
	dad	sp
	mov	a, m		; product[5]
	ora	a		; clear carry (no overflow to shift in)

.Lmul_shift_msb:
	; A = product[5], HL = &product[5], carry = correct
	rar
	mov	m, a
	dcx	h
	mov	a, m		; product[4]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; product[3]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; product[2]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; product[1]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; product[0]
	rar
	mov	m, a

	; Right-shift mant_b by 1
	lxi	h, 6
	dad	sp
	mov	a, m		; mant_b[2]
	ora	a
	rar
	mov	m, a
	dcx	h
	mov	a, m		; mant_b[1]
	rar
	mov	m, a
	dcx	h
	mov	a, m		; mant_b[0]
	rar
	mov	m, a

	; Decrement counter
	lxi	h, 7
	dad	sp
	mov	a, m
	dcr	a
	mov	m, a
	jnz	.Lmul_bit_loop

	; Product is in [SP+9..14]. The significant bits are in bytes 3..5
	; (the top 24 bits of the 48-bit result).
	; The implicit bit should be at bit 47 or 46 of the product.
	; If product[5] has bit 7 set, the product has bit 47 set (needs shift right by 1).
	; If only bit 6 is set, it's normalized.

	; Load top 3 bytes of product as our result mantissa
	lxi	h, 12
	dad	sp
	mov	c, m		; product[3] -> byte0
	inx	h
	mov	b, m		; product[4] -> byte1
	inx	h
	mov	e, m		; product[5] -> byte2

	; Check if bit 7 of E (= bit 47 of 48-bit product) is set.
	; If set: implicit bit already at position 23, just bump exponent.
	; If not set: bit 46 is the highest, need to shift left by 1.
	mov	a, e
	ani	0x80
	jnz	.Lmul_bit47

	; Bit 46 case: shift C:B:E left by 1 to put implicit at bit 23
	mov	a, c
	add	a
	mov	c, a
	mov	a, b
	ral
	mov	b, a
	mov	a, e
	ral
	mov	e, a
	jmp	.Lmul_normalized

.Lmul_bit47:
	; Bit 47 set: increment exponent (implicit bit already at bit 23)
	lxi	h, 3
	dad	sp
	mov	a, m
	inr	a
	mov	m, a
	cpi	0xFF
	jz	.Lmul_overflow

.Lmul_normalized:
	; Store mantissa back to scratch for packing
	lxi	h, 0
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e

	; Pack result (reuse addsf3's pack code pattern)
	; Clear implicit bit
	lxi	h, 2
	dad	sp
	mov	a, m
	ani	0x7F
	mov	m, a

	; Build byte2
	lxi	h, 3
	dad	sp
	mov	a, m		; exp
	ani	1
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	mov	d, a
	lxi	h, 2
	dad	sp
	mov	a, m
	ora	d
	mov	e, a

	; Build byte3
	lxi	h, 3
	dad	sp
	mov	a, m
	ora	a
	rar
	ani	0x7F
	mov	d, a
	lxi	h, 8
	dad	sp
	mov	a, m		; result_sign
	ora	d
	mov	d, a

	lxi	h, 0
	dad	sp
	mov	c, m
	inx	h
	mov	b, m

	; Clean up (16 bytes)
	lxi	h, 16
	dad	sp
	sphl
	ret

.Lmul_ret_zero:
	; Return signed zero
#ifdef UNDOC
	ldsi	8
	ldax	d		; result_sign
#else
	lxi	h, 8
	dad	sp
	mov	a, m		; result_sign
#endif
	mov	d, a		; byte3 = sign
	mvi	e, 0
	lxi	b, 0
	lxi	h, 16
	dad	sp
	sphl
	ret

.Lmul_overflow:
	; Return signed infinity
#ifdef UNDOC
	ldsi	8
	ldax	d		; result_sign
#else
	lxi	h, 8
	dad	sp
	mov	a, m		; result_sign
#endif
	ori	0x7F		; byte3 of inf
	mov	d, a
	mvi	e, 0x80
	lxi	b, 0
	lxi	h, 16
	dad	sp
	sphl
	ret

.Lmul_a_special:
	; exp_a == 0xFF (Inf or NaN)
	; If a is NaN, return NaN
	lxi	h, 0
	dad	sp
	mov	a, m
	inx	h
	ora	m
	inx	h
	mov	d, m
	mov	a, d
	ani	0x7F
	lxi	h, 0
	dad	sp
	ora	m
	inx	h
	ora	m
	jnz	.Lmul_ret_nan
	; a is Inf. If b is zero, return NaN (Inf * 0 = NaN)
	lxi	h, 7
	dad	sp
	mov	a, m
	ora	a
	jz	.Lmul_ret_nan
	; Return signed Inf
	jmp	.Lmul_overflow

.Lmul_b_special:
	; exp_b == 0xFF (Inf or NaN). a is normal.
	; Check mant_b for NaN
	mov	a, e
	ani	0x7F
	ora	b
	ora	c
	jnz	.Lmul_ret_nan
	; b is Inf. a is nonzero (checked earlier). Return signed Inf.
	jmp	.Lmul_overflow

.Lmul_ret_nan:
	lxi	b, 0x0000
	lxi	d, 0x7FC0
	lxi	h, 16
	dad	sp
	sphl
	ret

	.size	__mulsf3, .-__mulsf3

; ============================================================
; float __divsf3(float a, float b)
;
; IEEE 754 division.
; result_sign = sign_a XOR sign_b
; result_exp = exp_a - exp_b + 127
; result_mant = mant_a / mant_b (24-bit long division)
;
; Stack scratch layout (16 bytes):
;   [SP+0..2]   = remainder (3 bytes, starts as mant_a)
;   [SP+3]      = result exponent
;   [SP+4..6]   = divisor (mant_b, 3 bytes)
;   [SP+7]      = bit counter
;   [SP+8]      = result_sign
;   [SP+9..11]  = quotient (3 bytes, built bit by bit)
;   [SP+12..15] = pad
; Args at [SP+18..25].
; ============================================================
	.globl	__divsf3
	.type	__divsf3,@function
__divsf3:
	; Allocate 16 bytes scratch
	lxi	h, -16
	dad	sp
	sphl

	; Load a from [SP+18..21]
#ifdef UNDOC
	ldsi	18
	xchg
	call	.Lload32
#else
	lxi	h, 18
	dad	sp
	call	.Lload32
#endif

	; Compute result sign
	mov	a, d
	ani	0x80
	push	psw
	lxi	h, 27
	dad	sp		; b.byte3 at SP+25 before push, +2 for push = SP+27
	mov	a, m
	ani	0x80
	mov	h, a
	pop	psw
	xra	h
	lxi	h, 8
	dad	sp
	mov	m, a		; [SP+8] = result_sign

	; Reload a, extract exp
	lxi	h, 18
	dad	sp
	call	.Lload32
	call	.Lget_exp_regs
	ora	a
	jz	.Ldiv_ret_zero	; a is zero -> 0/b = 0
	cpi	0xFF
	jz	.Ldiv_a_special
	push	psw		; save exp_a

	; Set implicit bit, store mantissa_a
	; NOTE: push psw shifted SP by 2, so scratch[0] is at SP+2
	mov	a, e
	ori	0x80
	mov	e, a
	lxi	h, 2
	dad	sp
	mov	m, c		; scratch[0] = mant_a[0]
	inx	h
	mov	m, b		; scratch[1] = mant_a[1]
	inx	h
	mov	m, e		; scratch[2] = mant_a[2]

	; Load b, extract exp
	lxi	h, 24		; 22 + 2 (pushed exp_a)
	dad	sp
	call	.Lload32
	call	.Lget_exp_regs
	ora	a
	jz	.Ldiv_by_zero
	cpi	0xFF
	jz	.Ldiv_b_special

	mov	c, a		; C = exp_b
	; Set implicit bit, store mantissa_b
	mov	a, e
	ori	0x80
	mov	e, a
	lxi	h, 6		; SP+4 + 2 from push = 6
	dad	sp
	mov	m, e		; mant_b[2]
	dcx	h
	; Wait, need to reload b properly. Let me redo.
	; After push psw, stack offsets shifted by 2.
	; mant_b storage: [SP+4..6] original = [SP+6..8] with push
	; Actually, let me pop exp_a first to keep offsets clean.
	pop	psw		; A = exp_a
	mov	d, a		; D = exp_a

	; C = exp_b (from above)
	; Compute result exp = exp_a - exp_b + 127
	; Must handle positive/negative difference separately to avoid
	; 8-bit overflow confusion between underflow and valid results.
	mov	a, d		; exp_a
	sub	c		; exp_a - exp_b; carry = borrow if exp_a < exp_b
	jc	.Ldiv_exp_neg_diff

	; Positive or zero difference: A = exp_a - exp_b (0..253)
	adi	127		; A = diff + 127 (127..380)
	jc	.Ldiv_overflow	; diff > 128, result exp > 255
	cpi	0xFF
	jz	.Ldiv_overflow	; result exp = 255 = inf
	lxi	h, 3
	dad	sp
	mov	m, a		; [SP+3] = result exp
	jmp	.Ldiv_store_b

.Ldiv_exp_neg_diff:
	; Negative difference: A = 256 + (exp_a - exp_b)
	; True result exp = (exp_a - exp_b) + 127 = A - 256 + 127 = A - 129
	; If A < 130: result exp <= 0 -> underflow (flush to zero)
	; If A >= 130: result exp = A - 129, in [1, 126], valid
	cpi	130
	jc	.Ldiv_ret_zero	; A < 130, exp <= 0, underflow
	sui	129		; A = result exp (1..126)
	lxi	h, 3
	dad	sp
	mov	m, a		; [SP+3] = result exp
	jmp	.Ldiv_store_b

.Ldiv_store_b:
	; Re-load b to store mantissa_b
	lxi	h, 22
	dad	sp
	call	.Lload32
	mov	a, e
	ori	0x80
	mov	e, a
	lxi	h, 4
	dad	sp
	mov	m, c		; mant_b[0]
	inx	h
	mov	m, b		; mant_b[1]
	inx	h
	mov	m, e		; mant_b[2]

	; Clear quotient
	lxi	h, 9
	dad	sp
	mvi	m, 0
	inx	h
	mvi	m, 0
	inx	h
	mvi	m, 0

	; ============================================================
	; 24-bit restoring long division.
	; Algorithm: initial compare + 23 shift-and-subtract iterations.
	;
	; Step 0: Compare mant_a >= mant_b. If yes, subtract and set Q bit 0.
	; Steps 1-23: shift remainder left (25-bit with carry), compare,
	;   subtract if >=, shift quotient left and set bit.
	; Result: 24-bit quotient with implicit bit at bit 23 (if mant_a >= mant_b)
	;   or bit 22 (if mant_a < mant_b, needs 1 normalize shift).
	; ============================================================

	; --- Step 0: initial compare mant_a vs mant_b ---
	; Compare remainder[0..2] (= mant_a) >= divisor[0..2], MSB first
	lxi	h, 2
	dad	sp
	mov	d, m		; rem[2]
	lxi	h, 6
	dad	sp
	mov	a, d
	cmp	m		; rem[2] - div[2]
	jc	.Ldiv_init_lt
	jnz	.Ldiv_init_ge
	lxi	h, 1
	dad	sp
	mov	d, m		; rem[1]
	lxi	h, 5
	dad	sp
	mov	a, d
	cmp	m
	jc	.Ldiv_init_lt
	jnz	.Ldiv_init_ge
	lxi	h, 0
	dad	sp
	mov	d, m		; rem[0]
	lxi	h, 4
	dad	sp
	mov	a, d
	cmp	m
	jc	.Ldiv_init_lt

.Ldiv_init_ge:
	; mant_a >= mant_b: subtract divisor from remainder, set quotient = 1
	lxi	h, 4
	dad	sp
	mov	c, m		; div[0]
	inx	h
	mov	b, m		; div[1]
	inx	h
	mov	e, m		; div[2]
	lxi	h, 0
	dad	sp
	mov	a, m
	sub	c
	mov	m, a
	inx	h
	mov	a, m
	sbb	b
	mov	m, a
	inx	h
	mov	a, m
	sbb	e
	mov	m, a
	; Set quotient = 1
	lxi	h, 9
	dad	sp
	mvi	m, 1

.Ldiv_init_lt:
	; --- Main loop: 23 iterations ---
	mvi	a, 23
	lxi	h, 7
	dad	sp
	mov	m, a		; [SP+7] = 23

.Ldiv_loop:
	; 1. Shift quotient left by 1
	lxi	h, 9
	dad	sp
	mov	a, m
	add	a
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a

	; 2. Shift remainder left by 1 (produces 25-bit result, carry = bit 24)
	lxi	h, 0
	dad	sp
	mov	a, m
	add	a
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	inx	h
	mov	a, m
	ral
	mov	m, a
	; carry = 25th bit of remainder (overflow)
	; If carry set, remainder is > any 24-bit divisor
	jc	.Ldiv_rem_ge

	; 3. Compare remainder[0..2] >= divisor[0..2], MSB first
	; HL is at SP+2 = rem[2] after the shift above
	mov	d, m		; D = rem[2]
	; Get to div[2] at SP+6 using inx h (preserves flags)
	inx	h		; SP+3
	inx	h		; SP+4
	inx	h		; SP+5
	inx	h		; SP+6 = div[2]
	mov	a, d
	cmp	m		; rem[2] - div[2]
	jc	.Ldiv_rem_lt
	jnz	.Ldiv_rem_ge

	lxi	h, 1
	dad	sp
	mov	d, m		; rem[1]
	lxi	h, 5
	dad	sp
	mov	a, d
	cmp	m
	jc	.Ldiv_rem_lt
	jnz	.Ldiv_rem_ge

	lxi	h, 0
	dad	sp
	mov	d, m		; rem[0]
	lxi	h, 4
	dad	sp
	mov	a, d
	cmp	m
	jc	.Ldiv_rem_lt

.Ldiv_rem_ge:
	; remainder >= divisor: subtract divisor, set quotient bit
	lxi	h, 4
	dad	sp
	mov	c, m		; div[0]
	inx	h
	mov	b, m		; div[1]
	inx	h
	mov	e, m		; div[2]

	lxi	h, 0
	dad	sp
	mov	a, m
	sub	c
	mov	m, a		; rem[0] -= div[0]
	inx	h
	mov	a, m
	sbb	b
	mov	m, a		; rem[1] -= div[1]
	inx	h
	mov	a, m
	sbb	e
	mov	m, a		; rem[2] -= div[2]

	; Set LSB of quotient
	lxi	h, 9
	dad	sp
	mov	a, m
	ori	1
	mov	m, a

.Ldiv_rem_lt:
	; Decrement counter
	lxi	h, 7
	dad	sp
	mov	a, m
	dcr	a
	mov	m, a
	jnz	.Ldiv_loop

	; Load quotient into C, B, E
	lxi	h, 9
	dad	sp
	mov	c, m		; q[0]
	inx	h
	mov	b, m		; q[1]
	inx	h
	mov	e, m		; q[2]

	; Quotient has 24 bits. If mant_a >= mant_b, bit 23 is set (normalized).
	; If mant_a < mant_b, bit 22 is set, need one left shift and exp--.
	mov	a, e
	ani	0x80
	jnz	.Ldiv_q_normalized

	; Normalize: shift left by 1, decrement exponent
	mov	a, c
	add	a
	mov	c, a
	mov	a, b
	ral
	mov	b, a
	mov	a, e
	ral
	mov	e, a
	lxi	h, 3
	dad	sp
	mov	a, m
	dcr	a
	mov	m, a
	ora	a
	jz	.Ldiv_q_done	; underflow to zero

.Ldiv_q_normalized:
.Ldiv_q_done:
	; Store quotient as mantissa
	lxi	h, 0
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	inx	h
	mov	m, e

	; Pack result (same pattern as addsf3)
	; Clear implicit bit
	lxi	h, 2
	dad	sp
	mov	a, m
	ani	0x7F
	mov	m, a

	; Build byte2
	lxi	h, 3
	dad	sp
	mov	a, m
	ani	1
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	rlc
	mov	d, a
	lxi	h, 2
	dad	sp
	mov	a, m
	ora	d
	mov	e, a

	; Build byte3
	lxi	h, 3
	dad	sp
	mov	a, m
	ora	a
	rar
	ani	0x7F
	mov	d, a
	lxi	h, 8
	dad	sp
	mov	a, m
	ora	d
	mov	d, a

	lxi	h, 0
	dad	sp
	mov	c, m
	inx	h
	mov	b, m

	lxi	h, 16
	dad	sp
	sphl
	ret

.Ldiv_ret_zero:
#ifdef UNDOC
	ldsi	8
	ldax	d
#else
	lxi	h, 8
	dad	sp
	mov	a, m
#endif
	mov	d, a
	mvi	e, 0
	lxi	b, 0
	lxi	h, 16
	dad	sp
	sphl
	ret

.Ldiv_by_zero:
	; a / 0 = Inf (with appropriate sign)
	pop	psw		; clean pushed exp_a
	jmp	.Ldiv_overflow

.Ldiv_overflow:
#ifdef UNDOC
	ldsi	8
	ldax	d
#else
	lxi	h, 8
	dad	sp
	mov	a, m
#endif
	ori	0x7F
	mov	d, a
	mvi	e, 0x80
	lxi	b, 0
	lxi	h, 16
	dad	sp
	sphl
	ret

.Ldiv_a_special:
	; a is Inf or NaN
	; Load a mantissa to check NaN
#ifdef UNDOC
	ldsi	18
	xchg
	call	.Lload32
#else
	lxi	h, 18
	dad	sp
	call	.Lload32
#endif
	mov	a, e
	ani	0x7F
	ora	b
	ora	c
	jnz	.Ldiv_ret_nan	; a is NaN

	; a is Inf. Check b.
	lxi	h, 22
	dad	sp
	call	.Lload32
	call	.Lget_exp_regs
	cpi	0xFF
	jz	.Ldiv_ret_nan	; Inf / Inf = NaN
	; Inf / finite = Inf
	jmp	.Ldiv_overflow

.Ldiv_b_special:
	; b is Inf (or NaN), a is finite
	pop	psw		; clean pushed exp_a
	; Reload b to check NaN
	lxi	h, 22
	dad	sp
	call	.Lload32
	mov	a, e
	ani	0x7F
	ora	b
	ora	c
	jnz	.Ldiv_ret_nan	; b is NaN
	; finite / Inf = 0
	jmp	.Ldiv_ret_zero

.Ldiv_ret_nan:
	lxi	b, 0x0000
	lxi	d, 0x7FC0
	lxi	h, 16
	dad	sp
	sphl
	ret

	.size	__divsf3, .-__divsf3
