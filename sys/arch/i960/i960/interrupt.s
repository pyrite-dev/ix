.altmacro

.extern _intr_dispatch

.section .text
.global __intr_init
.global __intr_save
.global __intr_restore

.macro INTR_STUB n
intr_\n:
	st g0, (sp)
	st g1, 4(sp)
	st g2, 8(sp)
	st g3, 12(sp)
	st g4, 16(sp)
	st g5, 20(sp)
	st g6, 24(sp)
	st g7, 28(sp)
	st g8, 32(sp)
	st g9, 36(sp)
	st g10, 40(sp)
	st g11, 44(sp)
	st g12, 48(sp)
	st g13, 52(sp)
	st g14, 56(sp)
	st fp, 60(sp)

	modac 0, 0, r3
	st r3, 64(sp)

	st rip, 68(sp)

	modpc 0, 0, r3
	st r3, 72(sp)

	modtc 0, 0, r3
	st r3, 76(sp)

	mov g0, r3
	ldconst \n, g0

	mov g1, r4
	mov sp, g1

	ldconst 84, r3
	addo r3, sp, sp

	call _intr_dispatch

	ldconst 84, r3
	subo r3, sp, sp

	mov r4, g1
	mov r3, g0
	ret
.endm

.set vec, 8
.rept (256-8)
INTR_STUB %vec
.set vec, vec + 1
.endr

.macro INTR_REG n
	lda intr_\n, r3
	lda _int + (\n + 1) * 4, r4
	st r3, (r4)
.endm

__intr_init:
.set vec, 8
.rept (256-8)
INTR_REG %vec
.set vec, vec + 1
.endr
	shlo 16, 0, r3
	shlo 16, 31, r4
	modpc r3, r4, r5
	ret

__intr_save:
	modpc g0, 0, g0

	shlo 16, 31, r3
	modpc r3, r3, r3

	ret

__intr_restore:
	shlo 16, 31, r3
	modpc g0, r3, r3
	ret

.section .int
.global _int
.global _int_stack
_int:
	.space 1028
	.space 12
_int_stack:
	.space 4096
