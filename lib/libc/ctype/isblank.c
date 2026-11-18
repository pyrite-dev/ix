#include <ctype.h>

int isblank(int c) {
	if(c == 0x20 || c == 0x09) return 1;

	return 0;
}
