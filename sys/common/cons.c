#include <sys/cons.h>

#include <stdarg.h>
#include <stddef.h>

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
