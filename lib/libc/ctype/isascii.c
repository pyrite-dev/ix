#include <ctype.h>

int isascii(int c) {
	if(0 <= c && c <= 0x7f) return 1;

	return 0;
}
