#ifndef EXEC_H
#define EXEC_H

#include <stdio.h>
#include <stdlib.h>
#include "repr.h"

typedef bool* sat_t;

sat_t exec_prog (rprog_t* prog);

#endif // EXEC_H
