#include <ctype.h>

int isupper(int c) {
	if(0x41 <= c && c <= 0x5a) return 1;

	return 0;
}
