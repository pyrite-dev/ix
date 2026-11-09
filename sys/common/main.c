#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/device.h>
#include <stddef.h>

void kmain(void) {
	int i;

	machdep_init();
	consinit();
	intr_init();

	consprintf("*** IX Kernel Version %s (%s: %s) ***\r\n", os_version, os_ident, os_date);

	for(i = 0; devices[i].name != NULL; i++) {
		consprintf("%s\r\n", devices[i].name);
	}
}
