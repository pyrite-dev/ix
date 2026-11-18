#include <ctype.h>

int islower(int c) {
	if(0x61 <= c && c <= 0x7a) return 1;

	return 0;
}
