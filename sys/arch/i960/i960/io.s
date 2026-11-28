.section .text
.global _inb
.global _inw
.global _inl
.global _outb
.global _outw
.global _outl

_inb:
	mov 0, g0
	ret

_inw:
	mov 0, g0
	ret

_inl:
	mov 0, g0
	ret

_outb:
	ret

_outw:
	ret

_outl:
	ret
