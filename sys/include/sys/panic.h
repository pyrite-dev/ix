#ifndef __SYS_PANIC_H__
#define __SYS_PANIC_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void panic(const char* str);
#endif

__END_EXTERN_C__

#endif
