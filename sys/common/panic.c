#include <sys/panic.h>
#include <sys/osinfo.h>
#include <sys/cons.h>

void panic(const char* str){
	kprintf("**** PANIC! IX/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
	kprintf("** Message: %s\r\n", str);

	while(1);
}
