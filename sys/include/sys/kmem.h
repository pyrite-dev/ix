#ifndef __SYS_KMEM_H__
#define __SYS_KMEM_H__

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stddef.h>

__BEGIN_EXTERN_C__

extern vaddr_t kmem_avail;

void  kmem_init(void* start, void* end);
void* kmalloc(size_t size);
void* krealloc(void* ptr, size_t size);
void  kfree(void* ptr);

__END_EXTERN_C__

#endif
