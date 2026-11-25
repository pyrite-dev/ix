#include <sys/panic.h>
#include <sys/osinfo.h>
#include <sys/cons.h>

void panic(const char* str) {
	kprintf("******* %s\r\n", str);
	kprintf("*** Marc/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
	kprintf("*** A system crash has occurred. (See the above messages for details.)\r\n");
	kprintf("*** System halted.\r\n");

	while(1);
}
