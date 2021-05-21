#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include "repr.h"

typedef bool* sat_t;
typedef int* env_t;
typedef rstep_t** state_t;

// A state of the computation
typedef struct {
    env_t env; // assignment for each variable
    state_t state; // step for each process
    sat_t sat; // satisfied checks
    rprog_t* prog;
} compute_t;

compute_t* dup_compute (compute_t* comp);
void free_compute (compute_t* comp);

sat_t exec_prog_random (rprog_t* prog);
sat_t exec_prog_all (rprog_t* prog);

#endif // EXEC_H
