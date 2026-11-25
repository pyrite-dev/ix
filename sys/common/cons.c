#include <sys/cons.h>

#include <stdarg.h>
#include <stddef.h>

static void (*_cons_putc)(int c) = NULL;
static int (*_cons_getc)(void)	 = NULL;

void cons_putc(int c) {
	if(_cons_putc != NULL) _cons_putc(c);
}

int cons_getc(void) {
	if(_cons_getc != NULL) return _cons_getc();
	return 0;
}

void cons_output_register(void (*putc)(int c)) {
	_cons_putc = putc;
}

void cons_input_register(int (*getc)(void)) {
	_cons_getc = getc;
}

static void helper(char c) {
	cons_putc(c);
}

void kprintf(const char* format, ...) {
	va_list ap;

	va_start(ap, format);
	vkprintf(format, ap);
	va_end(ap);
}

#define FUNCNAME vkprintf
#include "../../lib/libc/stdio/_vprintf.h"
