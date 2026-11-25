.altmacro

.extern _syscall_dispatch

.section .text
.global __syscall_entry
.global __syscall_invoke

__syscall_entry:
	mov sp, r3
	ldconst 80, r4
	addo r4, sp, sp

	stq g0, 0(r3)
	stq g4, 16(r3)
	stq g8, 32(r3)
	stt g12, 48(r3)

	mov pfp, r4
	st r4, 60(r3)
	modac 0, 0, r4
	st r4, 64(r3)
	mov fp, r4
	andnot 7, r4, r4
	ld 8(r4), r4
	st r4, 68(r3)
	modpc 0, 0, r4
	st r4, 72(r3)
	modtc 0, 0, r4
	st r4, 76(r3)

	mov r3, g0
	call _syscall_dispatch

	ld (r3), g0
	mov r3, sp
	ret

__syscall_invoke:
	calls 0
	ret

.section .bss
.global __syscall_stack
.align 4
__syscall_stack:
	.space 4096
