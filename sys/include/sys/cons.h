#ifndef __SYS_CONS_H__
#define __SYS_CONS_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

void cons_init(void);
void cons_putc(int c);
int  cons_getc(void);

/* platform independent funcitons follows */
void cons_printf(const char* fmt, ...);

__END_EXTERN_C__

#endif
