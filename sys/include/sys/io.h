#ifndef __SYS_IO_H__
#define __SYS_IO_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
unsigned char  inb(unsigned short port);
unsigned short inw(unsigned short port);
unsigned int   inl(unsigned short port);

void outb(unsigned short port, unsigned char value);
void outw(unsigned short port, unsigned short value);
void outl(unsigned short port, unsigned int value);
#endif

__END_EXTERN_C__

#endif
