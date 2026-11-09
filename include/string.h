#ifndef __STRING_H__
#define __STRING_H__

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_EXTERN_C__

void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* dst, int c, size_t n);

__END_EXTERN_C__

#endif
