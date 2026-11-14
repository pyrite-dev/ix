#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/device.h>

void kmain(void) {
	machdep_init();
	cons_init();
	intr_init();

	kprintf("IX/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);

	device_init();

	panic("Test panic");
}
