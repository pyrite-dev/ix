#include <string.h>
#include <errno.h>

static const char* unknown = "Unknown error";

char* strerror(int errnum) {
	if(errnum >= sys_nerr) return (char*)unknown;

	return (char*)sys_errlist[errnum];
}
