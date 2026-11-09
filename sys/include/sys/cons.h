#ifndef __SYS_CONS_H__
#define __SYS_CONS_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

void consinit(void);
void consputc(int c);
int  consgetc(void);

/* platform independent funcitons follows */
void consprintf(const char* fmt, ...);

__END_EXTERN_C__

#endif
