#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include "repr.h"

typedef int* env_t;
typedef rstep_t** state_t;

struct compute;
struct diff;
typedef struct diff* sat_t;
// NULL if not satisfied, otherwise pointer
// to a computation state that satisfies
// the condition

// A state of the computation
typedef struct compute {
    env_t env; // assignment for each variable
    state_t state; // step for each process
    sat_t* sat; // satisfied checks
    rprog_t* prog;
    struct diff* diff; // which state this was forked from
} compute_t;

typedef struct diff {
    struct diff* parent;
    uint pid_advance;
    rstep_t* new_step;
    var_t* var_assign;
    int val_assign;
} diff_t;

compute_t* dup_compute (compute_t* comp);
void free_compute (compute_t* comp);

env_t blank_env (rprog_t* prog);

sat_t* exec_prog_random (rprog_t* prog);
sat_t* exec_prog_all (rprog_t* prog);

#endif // EXEC_H
