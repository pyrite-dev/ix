#include <ctype.h>

int iscntrl(int c) {
	if((0 <= c && c <= 0x1f) || c == 0x7f) return 1;

	return 0;
}
