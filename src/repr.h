#ifndef REPR_H
#define REPR_H

#include <stdbool.h>

#include "ast.h"

typedef unsigned uint;

// A different representation, more suited for execution

struct rexpr;
struct rstep;
struct rguard;

// a variable with its value
typedef struct {
    char* name;
    int value;
    uint id;
} rvar_t;

// an binary operation
typedef struct {
    struct rexpr* lhs;
    struct rexpr* rhs;
} rbinop_t;

// an expression
typedef union {
    rvar_t* var;
    unsigned digit;
    rbinop_t* binop;
    struct rexpr* subexpr;
} rexpr_u;
typedef struct rexpr {
    expr_e type;
    rexpr_u val;
} rexpr_t;

// an assignment operation
typedef struct {
    rvar_t* target;
    rexpr_t* expr;
} rassign_t;

// represents a choice (possibly only one solution)
typedef struct rstep {
    bool advance; // avoid looping
    rassign_t* assign; // maybe assign a new value
    unsigned nbguarded;
    struct rguard* guarded; // choose any if satisfied
    struct rstep* unguarded; // otherwise go here
    uint id;
} rstep_t;

// represents a guarded instruction
typedef struct rguard {
    rexpr_t* cond;
    rstep_t* next;
} rguard_t; 

// The idea behind these two structures
// is that each step of the computation consists of
// -> if nbguarded == 0
//    jump to unguarded and execute it
// -> else if one of the guarded has its guard satisfied
//    jump to any of them and execute it
// -> else if unguarded is not NULL execute it
// -> else the process is blocked

// a procedure
typedef struct {
    char* name;
    uint nbvar;
    rvar_t* vars;
    rstep_t* entrypoint;
} rproc_t;

// a reachability test
typedef struct {
    rexpr_t* cond;
} rcheck_t;

// a full program
typedef struct {
    uint nbvar;
    rvar_t* vars;
    uint nbproc;
    rproc_t* procs;
    uint nbcheck;
    rcheck_t* checks;
    uint nbstep;
} rprog_t;

rprog_t* tr_prog (prog_t* in);
void tr_var_list (uint* nb, rvar_t** loc, var_t* in);
void tr_check_list (uint* nb, rcheck_t** loc, check_t* in);
rexpr_t* tr_expr (expr_t* in);
rvar_t* locate_var (char* ident, uint nb, rvar_t* locs);
void tr_proc_list (uint* nb, rproc_t** loc, proc_t* in);

void tr_stmt (
    rstep_t** out, stmt_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto);
rstep_t* tr_branch_list (
    uint* nb, rguard_t** loc, branch_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto);

#endif // REPR_H
