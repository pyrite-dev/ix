.section .text
.global __syscall

__syscall:
	calls 0
	ret
