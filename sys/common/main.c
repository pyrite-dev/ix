#include <sys/cons.h>
#include <sys/machdep.h>

void kmain(void) {
	machdep_init();
	consinit();
}
