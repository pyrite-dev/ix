.altmacro

.extern _syscall_dispatch

.section .text
.global __syscall_entry
.global __syscall_invoke

__syscall_entry:
	mov sp, r4
	ldconst 80, r5
	addo r5, sp, sp

	stq g0, 0(r4)
	stq g4, 16(r4)
	stq g8, 32(r4)
	stt g12, 48(r4)

	mov pfp, r5
	st r5, 60(r4)
	modac 0, 0, r5
	st r5, 64(r4)
	mov fp, r5
	andnot 7, r5, r5
	ld 8(r5), r5
	st r5, 68(r4)
	modpc 0, 0, r5
	st r5, 72(r4)
	modtc 0, 0, r5
	st r5, 76(r4)

	mov r4, g0
	call _syscall_dispatch

	ld (r4), g0
	mov r4, sp
	ret

__syscall_invoke:
	calls 0
	ret
