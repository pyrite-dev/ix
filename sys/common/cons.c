#include <sys/cons.h>

#include <stdarg.h>
#include <stdlib.h>

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
				long long n;
				unsigned long long n2;
				int uns = 0;
				char*	  out = malloc(256);
				int	  p   = 256 - 2;
				int	  s = 0;
				int	  b = fmt[i] == 'x' ? 16 : 10;
				int	  j;

				if(out == NULL) break;

				if(fmt[i] == 'u'){
					if(inttype == 0) n2 = va_arg(va, unsigned int);
					if(inttype == 'l') n2 = va_arg(va, unsigned long);
					if(inttype == 'L') n2 = va_arg(va, unsigned long long);

					uns = 1;
				}else{
					if(inttype == 0) n = va_arg(va, int);
					if(inttype == 'l') n = va_arg(va, long);
					if(inttype == 'L') n = va_arg(va, long long);
				}

				if(!uns){
					s = n < 0 ? 1 : 0;
					if(s){
						n2 = -n;
						n = -n;
					}else{
						n2 = n;
					}

					if(n < 0){
						free(out);
						break;
					}
				}

				out[256 - 1] = 0;

				do {
					out[p--] = num[n2 % b];
					n2 /= b;
				} while(n2 > 0);

				if(s) kputc('-');

				if(fmt[i] == 'x') {
					kputc('0');
					kputc('x');
				}

				for(j = p + 1; out[j] != 0; j++) kputc(out[j]);

				free(out);

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
