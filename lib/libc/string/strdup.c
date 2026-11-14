#include <string.h>
#include <stdlib.h>

char* strdup(const char* s) {
	char* r = malloc(strlen(s) + 1);

	strcpy(r, s);

	return r;
}
