#ifndef __MACHINE_INTERRUPT_H__
#define __MACHINE_INTERRUPT_H__

#include <sys/cdefs.h>

typedef unsigned int intr_flags_t;

__BEGIN_EXTERN_C__

void	     _intr_init(void);
intr_flags_t _intr_save(void);
intr_flags_t _intr_restore(intr_flags_t flags);

__END_EXTERN_C__

#endif
