#include <sys/interrupt.h>

int _intr_vector(unsigned int irq) {
	if(irq == 1) return 8;
	if(irq == 2) return 9;

	return -1;
}
