#include <stdio.h>
#include <stdarg.h>

int printf(const char* fmt, ...){
	va_list ap;
	int s;

	va_start(ap, fmt);
	s = vprintf(fmt, ap);
	va_end(ap);

	return s;
}
