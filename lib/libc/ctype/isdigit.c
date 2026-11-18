#include <ctype.h>

int isdigit(int c) {
	if(0x30 <= c && c <= 0x39) return 1;

	return 0;
}
