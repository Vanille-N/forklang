#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include "repr.h"

typedef int* Env;
typedef RStep** State;

struct Compute;
struct Diff;
typedef struct Diff* Sat;
// NULL if not satisfied, otherwise pointer
// to a computation state that satisfies
// the condition

// A state of the computation
typedef struct Compute {
    Env env; // assignment for each variable
    State state; // step for each process
    Sat* sat; // satisfied checks
    RProg* prog;
    struct Diff* diff; // which state this was forked from
} Compute;

typedef struct Diff {
    struct Diff* parent;
    uint pid_advance;
    RStep* new_step;
    Var* var_assign;
    int val_assign;
} Diff;

Compute* dup_compute (Compute* comp);
void free_compute (Compute* comp);

Env blank_env (RProg* prog);

Sat* exec_prog_random (RProg* prog);
Sat* exec_prog_all (RProg* prog);
void free_sat ();

#endif // EXEC_H
