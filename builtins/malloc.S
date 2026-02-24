;; ==========================================================================
;; Minimal malloc/free for i8085 freestanding C++ (new/delete support)
;;
;; Pure bump allocator. free() is a no-op.
;; Uses linker symbols __heap_start and __heap_end.
;;
;; Calling convention: arg on stack at [SP+2], return in BC
;; ==========================================================================

  .text

;; -------------------------------------------------------------------------
;; void *malloc(size_t size)
;; Argument: size at [SP+2] (low), [SP+3] (high)
;; Returns: pointer in BC (0 if failed)
;;
;; Bump allocator: advances __malloc_heap_ptr by (size rounded up to even).
;; No header overhead - free is a no-op.
;; -------------------------------------------------------------------------
  .globl malloc
  .type malloc, @function
malloc:
  ;; Load requested size into DE
  LXI H, 0x0002
  DAD SP
  MOV E, M            ; size low byte
  INX H
  MOV D, M            ; size high byte

  ;; Check size == 0 -> return NULL
  MOV A, D
  ORA E
  JNZ malloc_nonzero
  LXI B, 0x0000
  RET

malloc_nonzero:
  ;; Round size up to even (add 1 if odd)
  MOV A, E
  ANI 0x01
  JZ malloc_aligned
  INX D               ; size++
malloc_aligned:

  ;; Load current heap pointer into BC
  LXI H, __malloc_heap_ptr
  MOV C, M
  INX H
  MOV B, M            ; BC = current heap ptr (this will be returned)

  ;; Calculate new_ptr = heap_ptr + size
  MOV H, B
  MOV L, C            ; HL = heap_ptr
  DAD D               ; HL = heap_ptr + size = new_heap_ptr

  ;; Check: new_heap_ptr <= __heap_end?
  ;; __heap_end is a linker-provided absolute symbol
  PUSH B              ; save old heap_ptr (= result)
  LXI D, __heap_end   ; DE = __heap_end (absolute value from linker)
  ;; Compare: DE >= HL  =>  DE - HL >= 0  =>  no carry
  MOV A, E
  SUB L
  MOV A, D
  SBB H
  JC malloc_oom       ; carry means new_ptr > heap_end, out of memory

  ;; Success: store new_heap_ptr (which is in HL)
  PUSH H              ; save new_heap_ptr
  LXI H, __malloc_heap_ptr
  POP D               ; DE = new_heap_ptr
  MOV M, E            ; store low
  INX H
  MOV M, D            ; store high

  POP B               ; BC = old heap_ptr = allocated address
  RET

malloc_oom:
  POP B               ; discard saved old heap_ptr
  LXI B, 0x0000       ; return NULL
  RET

;; -------------------------------------------------------------------------
;; void free(void *ptr)
;; No-op for bump allocator.
;; -------------------------------------------------------------------------
  .globl free
  .globl cfree
  .type free, @function
  .type cfree, @function
free:
cfree:
  RET

;; -------------------------------------------------------------------------
;; Data: heap pointer, initialized to __heap_start by the linker
;; -------------------------------------------------------------------------
  .section .data
  .type __malloc_heap_ptr, @object
  .globl __malloc_heap_ptr
__malloc_heap_ptr:
  .word __heap_start
