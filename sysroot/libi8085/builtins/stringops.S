; Hand-written string operations for the i8085 target.
;
; These override picolibc's generic C implementations via link order
; (libgcc.a is linked before libc.a).
;
; Calling convention (stack-based, cdecl-ish):
;   Arguments pushed right-to-left.  Return value in BC.
;
;   strlen(const char *s):
;     [SP+0] = return address (2 bytes)
;     [SP+2] = s low
;     [SP+3] = s high
;     Returns length (16-bit) in BC.
;
;   strcmp(const char *s1, const char *s2):
;     [SP+0] = return address (2 bytes)
;     [SP+2] = s1 low
;     [SP+3] = s1 high
;     [SP+4] = s2 low
;     [SP+5] = s2 high
;     Returns int (16-bit) in BC: <0 if s1<s2, 0 if equal, >0 if s1>s2.

	.text

; ============================================================
; size_t strlen(const char *s)
; ============================================================
	.globl	strlen
	.type	strlen,@function
strlen:
	; Load s into HL
	lxi	h, 2
	dad	sp
	mov	a, m		; s low
	inx	h
	mov	h, m		; s high
	mov	l, a		; HL = s

	; Counter in BC = 0
	lxi	b, 0

.Lstrlen_loop:
	mov	a, m		; A = *s
	ora	a		; test for NUL
	jz	.Lstrlen_done
	inx	h		; s++
	inx	b		; count++
	jmp	.Lstrlen_loop

.Lstrlen_done:
	; BC already has the count
	ret
	.size	strlen, .-strlen

; ============================================================
; int strcmp(const char *s1, const char *s2)
; ============================================================
	.globl	strcmp
	.type	strcmp,@function
strcmp:
	; Load s2 into DE
	lxi	h, 4
	dad	sp
	mov	e, m		; s2 low
	inx	h
	mov	d, m		; s2 high

	; Load s1 into HL
	lxi	h, 2
	dad	sp
	mov	a, m		; s1 low
	inx	h
	mov	h, m		; s1 high
	mov	l, a		; HL = s1

	; HL = s1, DE = s2
.Lstrcmp_loop:
	mov	a, m		; A = *s1
	push	psw		; save *s1
	ldax	d		; A = *s2
	mov	b, a		; B = *s2
	pop	psw		; A = *s1

	; Compare *s1 and *s2
	sub	b		; A = *s1 - *s2
	jnz	.Lstrcmp_diff

	; Bytes equal — check if we hit NUL
	mov	a, m		; reload *s1
	ora	a
	jz	.Lstrcmp_equal	; both are NUL, strings equal

	; Advance pointers
	inx	h		; s1++
	inx	d		; s2++
	jmp	.Lstrcmp_loop

.Lstrcmp_diff:
	; A = *s1 - *s2 (non-zero, flags set from SUB)
	jc	.Lstrcmp_less
	; *s1 > *s2: return 1
	lxi	b, 1
	ret
.Lstrcmp_less:
	; *s1 < *s2: return -1
	lxi	b, 0xFFFF
	ret

.Lstrcmp_equal:
	lxi	b, 0
	ret
	.size	strcmp, .-strcmp
