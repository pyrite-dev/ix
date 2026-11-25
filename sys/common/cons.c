#include <sys/cons.h>

#include <stdarg.h>
#include <stddef.h>

static void helper(char c) {
	cons_putc(c);
}

#define FUNCNAME vkprintf
#include "../../lib/libc/stdio/_vprintf.h"

void kprintf(const char* fmt, ...) {
	va_list va;

	va_start(va, fmt);
	vkprintf(fmt, va);
	va_end(va);
}
