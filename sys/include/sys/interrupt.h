#ifndef __SYS_INTERRUPT_H__
#define __SYS_INTERRUPT_H__

#include <sys/cdefs.h>
#include <machine/interrupt.h>
#include <machine/regs.h>

typedef void (*intr_handler_t)(unsigned int vector, struct iregs* iregs, void* arg);
typedef void (*intr_raw_handler_t)(void);

__BEGIN_EXTERN_C__

void intr_init(void);
int  intr_register(unsigned int vector, intr_handler_t handler, void* arg);
void intr_unregister(unsigned int vector);
void intr_dispatch(unsigned int vector, struct iregs* iregs);

void	     _intr_init(void);
intr_flags_t _intr_save(void);
intr_flags_t _intr_restore(intr_flags_t flags);
int	     _intr_vector(unsigned int irq);
void	     _intr_set(unsigned int irq, intr_raw_handler_t handler);

__END_EXTERN_C__

#endif
