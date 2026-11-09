#ifndef __SYS_INTERRUPT_H__
#define __SYS_INTERRUPT_H__

#include <sys/cdefs.h>

typedef void (*intr_handler_t)(unsigned int vector, void* arg);

__BEGIN_EXTERN_C__

void intr_init(void);
void intr_arch_init(void);
int  intr_register(unsigned int vector, intr_handler_t handler, void* arg);
void intr_unregister(unsigned int vector);
void intr_dispatch(unsigned int vector);

__END_EXTERN_C__

#endif
