.altmacro

.extern _syscall_dispatch

.section .text
.global __syscall_stub
.global __syscall_invoke

__syscall_stub:
	mov sp, r6
	ldconst 80, r5
	addo r5, sp, sp

	st g0, (r6)
	st g1, 4(r6)
	st g2, 8(r6)
	st g3, 12(r6)
	st g4, 16(r6)
	st g5, 20(r6)
	st g6, 24(r6)
	st g7, 28(r6)
	st g8, 32(r6)
	st g9, 36(r6)
	st g10, 40(r6)
	st g11, 44(r6)
	st g12, 48(r6)
	st g13, 52(r6)
	st g14, 56(r6)
	st pfp, 60(r6)

	ld -12(fp), r3
	st r3, 64(r6)

	ldconst 0xfffffff0, r3
	and r3, pfp, r3
	mov r3, r7

	flushreg

	ld 8(r7), r3
	st r3, 68(r6)

	ld -16(fp), r3
	st r3, 72(r6)

	modtc 0, 0, r3
	st r3, 76(r6)

	# r8 = the caller's stack pointer
	ld 4(r7), r8

	# Add the frame size (64 bytes)
	ldconst 64, r5
	addo r5, r8, r9

	# Copy frame
	ld (r6), r10
	ld 4(r6), r11
	st r10, (r9)
	st r11, 4(r9)
	ld 8(r6), r10
	ld 12(r6), r11
	st r10, 8(r9)
	st r11, 12(r9)
	ld 16(r6), r10
	ld 20(r6), r11
	st r10, 16(r9)
	st r11, 20(r9)
	ld 24(r6), r10
	ld 28(r6), r11
	st r10, 24(r9)
	st r11, 28(r9)
	ld 32(r6), r10
	ld 36(r6), r11
	st r10, 32(r9)
	st r11, 36(r9)
	ld 40(r6), r10
	ld 44(r6), r11
	st r10, 40(r9)
	st r11, 44(r9)
	ld 48(r6), r10
	ld 52(r6), r11
	st r10, 48(r9)
	st r11, 52(r9)
	ld 56(r6), r10
	ld 60(r6), r11
	st r10, 56(r9)
	st r11, 60(r9)
	ld 64(r6), r10
	ld 68(r6), r11
	st r10, 64(r9)
	st r11, 68(r9)
	ld 72(r6), r10
	ld 76(r6), r11
	st r10, 72(r9)
	st r11, 76(r9)

	lda __syscall_trampoline, r3
	st r3, 8(r7)

	mov r9, g0

	mov r6, sp
	ret

__syscall_trampoline:
	call __syscall_body
	ret

__syscall_body:
	mov g0, r4

	ldconst 80, r5
	addo r5, sp, sp

	mov r4, g0
	call _syscall_dispatch

	ld (r4), g0

	ldconst 80, r5
	subo r5, sp, sp

	ret

__syscall_invoke:
	lda __syscall_iac, r4
	ldconst 0xff000010, r5
	synmovq r5, r4
	ret

.align 4
__syscall_iac:
	.word 0x400f0000
	.word 0
	.word 0
	.word 0
