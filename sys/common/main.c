#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>

void kmain(void) {
	machdep_init();
	consinit();
	intr_init();
}
