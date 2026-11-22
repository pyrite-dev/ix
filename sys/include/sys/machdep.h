#ifndef __SYS_MACHDEP_H__
#define __SYS_MACHDEP_H__

#include <sys/cdefs.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void machdep_init(void);
#endif

__END_EXTERN_C__

#endif
