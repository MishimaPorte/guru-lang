#ifndef GURU_COMMON
#define GURU_COMMON
#include <stdio.h>

#define error(msg) do {\
    perror(msg);\
    exit(1); } while (0)


#endif
