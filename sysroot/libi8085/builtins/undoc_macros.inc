; undoc_macros.inc — GAS macros for undocumented 8085 instructions.
;
; When UNDOC is defined, macros expand to use LDSI/LHLX/SHLX for
; shorter and faster stack access.  When UNDOC is not defined, they
; expand to the standard lxi+dad sequences.
;
; IMPORTANT:
;   - All UNDOC macros clobber DE.  Callers must ensure DE is dead.
;   - Non-UNDOC macros clobber HL (already the normal case).

#ifdef UNDOC

; Load 8-bit value from [SP+offset] into A.  Clobbers DE.
.macro sp_ld_a offset
	ldsi	\offset
	ldax	d
.endm

; Store A to [SP+offset].  Clobbers DE.
.macro sp_st_a offset
	ldsi	\offset
	stax	d
.endm

; Load 16-bit value from [SP+offset] into HL.  Clobbers DE.
.macro sp_ld_hl offset
	ldsi	\offset
	lhlx
.endm

; Store HL to [SP+offset].  Clobbers DE.
.macro sp_st_hl offset
	ldsi	\offset
	shlx
.endm

; Load 16-bit value from [SP+offset] into rhi:rlo.  Clobbers DE.
.macro sp_ld16 rhi, rlo, offset
	ldsi	\offset
	lhlx
	mov	\rlo, l
	mov	\rhi, h
.endm

; Set DE = SP + offset (address computation).  Clobbers nothing else.
.macro sp_addr_de offset
	ldsi	\offset
.endm

#else  /* !UNDOC */

.macro sp_ld_a offset
	lxi	h, \offset
	dad	sp
	mov	a, m
.endm

.macro sp_st_a offset
	lxi	h, \offset
	dad	sp
	mov	m, a
.endm

; Load 16-bit from [SP+offset] into HL.
; Uses A as temp (low byte must be read before H is overwritten).
.macro sp_ld_hl offset
	lxi	h, \offset
	dad	sp
	mov	a, m
	inx	h
	mov	h, m
	mov	l, a
.endm

; Store HL to [SP+offset].
; HL is both value and needed for address, so save via BC.
; The offset is adjusted +2 for the push.
.macro sp_st_hl offset
	push	b
	mov	b, h
	mov	c, l
	lxi	h, \offset+2
	dad	sp
	mov	m, c
	inx	h
	mov	m, b
	pop	b
.endm

; Load 16-bit from [SP+offset] into rhi:rlo.
.macro sp_ld16 rhi, rlo, offset
	lxi	h, \offset
	dad	sp
	mov	\rlo, m
	inx	h
	mov	\rhi, m
.endm

; Set HL = SP + offset (address computation).  Clobbers carry.
.macro sp_addr_de offset
	lxi	h, \offset
	dad	sp
	xchg
.endm

#endif /* UNDOC */
