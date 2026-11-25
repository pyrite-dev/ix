#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef ARG1
#define ARG1CALL ARG1,
#else
#define ARG1CALL
#endif
#ifdef ARG2
#define ARG2CALL ARG2,
#else
#define ARG2CALL
#endif

static void helper(
#ifdef ARG1
    ARG1TYPE ARG1,
#endif
#ifdef ARG2
    ARG2TYPE ARG2,
#endif
    char c);

#define HELPER(x) \
	{ \
		helper(ARG1CALL ARG2CALL(x)); \
		r++; \
	}

int FUNCNAME(
#ifdef ARG1
    ARG1TYPE ARG1,
#endif
#ifdef ARG2
    ARG2TYPE ARG2,
#endif
    const char* format, va_list ap) {
	int	   i;
	int	   j;
	char	   typespec[32];
	const char num[] = "0123456789abcdef";
	int	   r	 = 0;

	for(i = 0; format[i] != 0; i++) {
		if(format[i] == '%') {
			char c;

			typespec[0] = 0;

			while((c = format[++i]) != 0) {
				int brk = 0;

				switch(c) {
				case 'l':
				{
					int l = strlen(typespec);

					if(l == (sizeof(typespec) - 1)) break;

					typespec[l]	= c;
					typespec[l + 1] = 0;

					break;
				}
				case 's':
				{
					char* v = va_arg(ap, char*);

					if(v == NULL) v = "(null)";

					for(j = 0; v[j] != 0; j++) HELPER(v[j]);

					brk = 1;
					break;
				}
				case 'd':
				case 'u':
				case 'x':
				case 'c':
				{
					long long	   n;
					unsigned long long mag;
					int		   s = 0;
					int		   b = c == 'x' ? 16 : 10;

					if(c == 'u') {
						if(strcmp(typespec, "l") == 0) {
							mag = va_arg(ap, unsigned long);
						} else if(strcmp(typespec, "ll") == 0) {
							mag = va_arg(ap, unsigned long long);
						} else {
							mag = va_arg(ap, unsigned int);
						}
					} else {
						if(strcmp(typespec, "l") == 0) {
							n = va_arg(ap, long);
						} else if(strcmp(typespec, "ll") == 0) {
							n = va_arg(ap, long long);
						} else {
							n = va_arg(ap, int);
						}

						s = n < 0 ? 1 : 0;

						mag = s ? (unsigned long long)(-(n + 1)) + 1 : (unsigned long long)n;
					}

					switch(c) {
					case 'd':
					case 'u':
					case 'x':
					{
						unsigned long long div = 1;

						while(mag / div >= (unsigned long long)b) div *= b;

						if(s) HELPER('-');

						if(c == 'x') {
							HELPER('0');
							HELPER('x');
						}

						while(div > 0) {
							HELPER(num[mag / div]);

							mag %= div;
							div /= b;
						}
						break;
					}
					case 'c':
					{
						HELPER((char)n);
						break;
					}
					}

					brk = 1;
					break;
				}
				}

				if(brk) break;
			}
		} else {
			HELPER(format[i]);
		}
	}

	return r;
}

#undef ARG2CALL
#undef ARG1CALL
