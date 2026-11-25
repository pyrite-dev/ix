#include <stdio.h>
#include <stdarg.h>

int fprintf(FILE* stream, const char* fmt, ...) {
	va_list ap;
	int	s;

	va_start(ap, fmt);
	s = vfprintf(stream, fmt, ap);
	va_end(ap);

	return s;
}
