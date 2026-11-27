#include <sys/cons.h>
#include <sys/log.h>
#include <sys/msgbuf.h>
#include <sys/spinlock.h>

#include <stdarg.h>
#include <stddef.h>

static void (*_cons_putc)(int c) = NULL;
static int (*_cons_getc)(void)	 = NULL;

static spinlock_t cons_lock;

static int cons_tocons = 1;

void cons_init(void) {
	spinlock_init(&cons_lock);
}

void cons_putc(int c) {
	if(_cons_putc != NULL) _cons_putc(c);
}

int cons_getc(void) {
	if(_cons_getc != NULL) return _cons_getc();
	return 0;
}

void cons_output_register(void (*putc)(int c)) {
	intr_flags_t flags = spinlock_lock_irqsave(&cons_lock);

	_cons_putc = putc;

	spinlock_unlock_irqrestore(&cons_lock, flags);
}

void cons_input_register(int (*getc)(void)) {
	intr_flags_t flags = spinlock_lock_irqsave(&cons_lock);

	_cons_getc = getc;

	spinlock_unlock_irqrestore(&cons_lock, flags);
}

static void helper(char c) {
	msgbuf_putc(c);
	if(cons_tocons) cons_putc(c);
}

void kprintf(const char* format, ...) {
	va_list	     ap;
	intr_flags_t flags;

	va_start(ap, format);
	flags	    = spinlock_lock_irqsave(&cons_lock);
	cons_tocons = 1;
	vkprintf(format, ap);
	spinlock_unlock_irqrestore(&cons_lock, flags);
	va_end(ap);
}

void log(int level, const char* format, ...) {
	va_list	     ap;
	intr_flags_t flags;

	va_start(ap, format);
	flags	    = spinlock_lock_irqsave(&cons_lock);
	cons_tocons = 0;
	vkprintf(format, ap);
	spinlock_unlock_irqrestore(&cons_lock, flags);
	va_end(ap);
}

#define FUNCNAME vkprintf
#include "../../lib/libc/stdio/_vprintf.h"
