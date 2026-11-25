#ifndef __SYS_CONS_H__
#define __SYS_CONS_H__

#include <sys/cdefs.h>

#include <stdarg.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void cons_init(void);
void cons_putc(int c);
int  cons_getc(void);
void cons_output_register(void (*putc)(int c));
void cons_input_register(int (*getc)(void));

/* platform independent funcitons follows */
void kprintf(const char* format, ...);
int  vkprintf(const char* format, va_list ap);
#endif

__END_EXTERN_C__

#endif
