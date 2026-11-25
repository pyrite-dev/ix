#include <stdio.h>
#include <stdarg.h>

int dprintf(int fd, const char* fmt, ...) {
	va_list ap;
	int	s;

	va_start(ap, fmt);
	s = vdprintf(fd, fmt, ap);
	va_end(ap);

	return s;
}
