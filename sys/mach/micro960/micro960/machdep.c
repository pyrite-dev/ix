#include <sys/machdep.h>
#include <sys/kmem.h>

extern char heap_start;
extern char heap_end;

extern void icr_init(void);

void machdep_init(void){
	kmem_init(&heap_start, &heap_end);
	icr_init();
}
