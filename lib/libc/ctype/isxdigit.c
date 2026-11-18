#include <ctype.h>

int isxdigit(int c) {
	if(isdigit(c)) return 1;
	if(0x41 <= c && c <= 0x46) return 1;
	if(0x61 <= c && c <= 0x66) return 1;

	return 0;
}
