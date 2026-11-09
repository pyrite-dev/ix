#ifndef __SYS_KMEM_H__
#define __SYS_KMEM_H__

#include <sys/types.h>
#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_EXTERN_C__

void  kmem_init(void* start, void* end);
void* kmalloc(size_t size);
void* krealloc(void* ptr, size_t size);
void  kfree(void* ptr);

__END_EXTERN_C__

#endif
