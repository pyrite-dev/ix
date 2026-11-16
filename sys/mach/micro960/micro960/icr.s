.section .text
.global _icr_init

icr:
	.word 0xff090800

_icr_init:
	lda icr, r3
	lda 0xff000004, r4
	synmov r4, r3
	ret
