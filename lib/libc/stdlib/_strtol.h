#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* TODO: Check if we should do ERANGE */
RETTYPE FUNCNAME(const char* nptr, char** endptr, int base) {
	RETTYPE r = 0;
	int	i;
	RETTYPE s      = 1;
	char	num[]  = "0123456789";
	char	alp[]  = "abcdefghijklmnopqrstuvwxyz";
	char	alpu[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int	d      = 0;

	if(endptr != NULL) *endptr = (char*)nptr + strlen(nptr);

	if(base < 2 || base > 36) {
		errno = EINVAL;
		return 0;
	}

	for(i = 0; nptr[i] != 0; i++) {
		if(!isspace(nptr[i])) break;
	}

	if(base == 2 && strlen(nptr) > 2 && nptr[0] == '0' && (nptr[1] == 'b' || nptr[1] == 'B')) i += 2;
	if(base == 16 && strlen(nptr) > 2 && nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) i += 2;

	for(; nptr[i] != 0; i++) {
		int n = -1;

		if('0' <= nptr[i] && nptr[i] <= num[(base > 10 ? 10 : base) - 1]) {
			n = nptr[i] - '0';
			d = 1;
		} else if(base > 10 && 'a' <= nptr[i] && nptr[i] <= alp[base - 11]) {
			n = nptr[i] - 'a' + 10;
			d = 1;
		} else if(base > 10 && 'A' <= nptr[i] && nptr[i] <= alpu[base - 11]) {
			n = nptr[i] - 'A' + 10;
			d = 1;
#ifndef UNSIGNED
		} else if(i == 0 && nptr[i] == '-') {
			s = -1;
			continue;
#endif
		} else if(i == 0 && nptr[i] == '+') {
		} else {
			if(endptr != NULL) *endptr = (char*)&nptr[i];
			break;
		}

		r = r * base;
		r = r + n;
	}

	if(!d) errno = EINVAL;

	return r * s;
}
