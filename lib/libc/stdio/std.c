#include <stdio.h>

FILE  _iob[120];
FILE* stdin  = &_iob[0];
FILE* stdout = &_iob[1];
FILE* stderr = &_iob[2];
