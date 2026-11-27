#ifndef __SYS_MSGBUF_H__
#define __SYS_MSGBUF_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void msgbuf_putc(int c);
int  msgbuf_getc(void);
int  msgbuf_peek(unsigned int* pos);
#endif

__END_EXTERN_C__

#endif
