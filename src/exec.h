#ifndef EXEC_H
#define EXEC_H

#include <stdio.h>
#include <stdlib.h>
#include "repr.h"

typedef bool* sat_t;
typedef int* env_t;
typedef rstep_t** state_t;

typedef struct {
    env_t env;
    state_t state;
    sat_t sat;
    rprog_t* prog;
} compute_t;

compute_t* dup_compute (compute_t* comp);

sat_t exec_prog_random (rprog_t* prog);
sat_t exec_prog_all (rprog_t* prog);

#include "hashset.h"

#endif // EXEC_H
