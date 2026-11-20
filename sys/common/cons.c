#include <sys/cons.h>

#include <stdarg.h>
#include <stddef.h> /* NULL; no heap dependency */

static void kputc(char c) {
	cons_putc(c);
}

/* bare minimal */
void kprintf(const char* fmt, ...) {
	va_list	   va;
	int	   i;
	const char num[] = "0123456789abcdef";

	va_start(va, fmt);
	for(i = 0; fmt[i] != 0; i++) {
		if(fmt[i] == '%') {
			char inttype = 0;

		tryagain:;
			switch(fmt[++i]) {
			case 's':
			{
				char* s = va_arg(va, char*);

				if(s == NULL) s = "(null)";

				while(*s) kputc(*s++);
				break;
			}
			case 'l':
			{
				if(inttype == 0) {
					inttype = 'l';
				} else if(inttype == 'l') {
					inttype = 'L';
				}
				goto tryagain;
				break;
			}
			case 'd':
			case 'u':
			case 'x':
			{
				long long	   n = 0;
				unsigned long long mag;
				unsigned long long div;
				int		   s = 0;
				int		   b = fmt[i] == 'x' ? 16 : 10;

				if(fmt[i] == 'u') {
					if(inttype == 0) mag = va_arg(va, unsigned int);
					if(inttype == 'l') mag = va_arg(va, unsigned long);
					if(inttype == 'L') mag = va_arg(va, unsigned long long);
				} else {
					if(inttype == 0) n = va_arg(va, int);
					if(inttype == 'l') n = va_arg(va, long);
					if(inttype == 'L') n = va_arg(va, long long);

					s = n < 0 ? 1 : 0;

					mag = s ? (unsigned long long)(-(n + 1)) + 1
						: (unsigned long long)n;
				}

				div = 1;

				while(mag / div >= (unsigned long long)b)
					div *= b;

				if(s) kputc('-');

				if(fmt[i] == 'x') {
					kputc('0');
					kputc('x');
				}

				while(div > 0) {
					kputc(num[mag / div]);
					mag %= div;
					div /= b;
				}

				break;
			}
			case '%':
			{
				kputc('%');
				break;
			}
			}
		} else {
			kputc(fmt[i]);
		}
	}
	va_end(va);
}
