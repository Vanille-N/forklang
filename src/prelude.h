#ifndef PRELUDE_H
#define PRELUDE_H

typedef unsigned int uint;

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

#endif // PRELUDE_H
