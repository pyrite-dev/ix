#ifndef __SYS_UACCESS_H__
#define __SYS_UACCESS_H__

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_EXTERN_C__

int copyin(void* kaddr, const void* uaddr, size_t len);
int copyout(void* uaddr, const void* kaddr, size_t len);

__END_EXTERN_C__

#endif
