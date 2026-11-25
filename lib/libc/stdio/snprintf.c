#include <stdio.h>
#include <stdarg.h>

int snprintf(char* str, size_t size, const char* fmt, ...) {
	va_list ap;
	int	s;

	va_start(ap, fmt);
	s = vsnprintf(str, size, fmt, ap);
	va_end(ap);

	return s;
}
