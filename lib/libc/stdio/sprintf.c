#include <stdio.h>
#include <stdarg.h>

int sprintf(char* str, const char* fmt, ...) {
	va_list ap;
	int	s;

	va_start(ap, fmt);
	s = vsprintf(str, fmt, ap);
	va_end(ap);

	return s;
}
