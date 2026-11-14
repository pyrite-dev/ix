#ifndef __SYS_PANIC_H__
#define __SYS_PANIC_H__

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_EXTERN_C__

void panic(const char* str);

__END_EXTERN_C__

#endif
