#include <sys/cons.h>
#include <stdarg.h>

/* bare minimal */
void cons_printf(const char* fmt, ...) {
	va_list va;
	int	i;

	va_start(va, fmt);
	for(i = 0; fmt[i] != 0; i++) {
		if(fmt[i] == '%') {
			switch(fmt[++i]) {
			case 's':
			{
				char* s = va_arg(va, char*);

				while(*s) cons_putc(*s++);
			}
			}
		} else {
			cons_putc(fmt[i]);
		}
	}
	va_end(va);
}
