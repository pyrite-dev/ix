#include <string.h>
#include <errno.h>

char* strerror(int errnum) {
	if(errnum >= sys_nerr) return "Unknown error";

	return sys_errlist[errnum];
}
