#include <ctype.h>

int isalpha(int c) {
	if(isupper(c)) return 1;
	if(islower(c)) return 1;

	return 0;
}
