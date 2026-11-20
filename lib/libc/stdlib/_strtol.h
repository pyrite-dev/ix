#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

RETTYPE FUNCNAME(const char* nptr, char** endptr, int base) {
	unsigned long long ur = 0, limit, cutoff;
	int		   cutlim, overflow = 0;
	int	i;
	RETTYPE s      = 1;
	char	num[]  = "0123456789";
	char	alp[]  = "abcdefghijklmnopqrstuvwxyz";
	char	alpu[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int	d      = 0;

	if(endptr != NULL) *endptr = (char*)nptr + strlen(nptr);

	if(base < 0 || base == 1 || base > 36) {
		errno = EINVAL;
		return 0;
	}

	for(i = 0; nptr[i] != 0; i++) {
		if(!isspace(nptr[i])) break;
	}

	if(
#ifndef UNSIGNED
	    nptr[i] == '-' ||
#endif
	    nptr[i] == '+') {
		if(nptr[i] == '-') s = -1;
		i++;
	}

	if((base == 0 || base == 2) && nptr[i] == '0' && (nptr[i + 1] == 'b' || nptr[i + 1] == 'B') && nptr[i + 2] != 0) {
		base = 2;
		i += 2;
	} else if((base == 0 || base == 16) && nptr[i] == '0' && (nptr[i + 1] == 'x' || nptr[i + 1] == 'X') && nptr[i + 2] != 0) {
		base = 16;
		i += 2;
	} else if((base == 0 || base == 8) && nptr[i] == '0') {
		base = 8;
	}
	if(base == 0) base = 10;

#ifdef UNSIGNED
	limit = (unsigned long long)(RETTYPE)~0ULL;
#else
	limit = (1ULL << (sizeof(RETTYPE) * 8 - 1)) - 1;
	if(s < 0) limit += 1;
#endif
	cutoff = limit / (unsigned)base;
	cutlim = (int)(limit % (unsigned)base);

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
		} else {
			if(endptr != NULL) *endptr = (char*)&nptr[i];
			break;
		}

		if(!overflow) {
			if(ur > cutoff || (ur == cutoff && n > cutlim))
				overflow = 1;
			else
				ur = ur * base + n;
		}
	}

	if(!d) errno = EINVAL;

	if(overflow) {
		errno = ERANGE;
#ifdef UNSIGNED
		return (RETTYPE)~0ULL;
#else
		if(s < 0) return (RETTYPE)(-(RETTYPE)(limit - 1) - 1);
		return (RETTYPE)limit;
#endif
	}

#ifdef UNSIGNED
	return (RETTYPE)ur;
#else
	if(s < 0) {
		if(ur == limit) return (RETTYPE)(-(RETTYPE)(limit - 1) - 1);
		return (RETTYPE)(-(RETTYPE)ur);
	}
	return (RETTYPE)ur;
#endif
}
