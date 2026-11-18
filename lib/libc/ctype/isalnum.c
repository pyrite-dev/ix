#include <ctype.h>

int isalnum(int c) {
	if(isdigit(c)) return 1;
	if(isalpha(c)) return 1;

	return 0;
}
