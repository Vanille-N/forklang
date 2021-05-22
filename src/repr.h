#ifndef REPR_H
#define REPR_H

#include <stdbool.h>

#include "ast.h"

typedef unsigned uint;

#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

void free_repr ();

// A different representation, more suited for execution
// Not a tree but an execution graph

struct RExpr;
struct RStep;
struct RGuard;

// a binary operation
typedef struct {
    struct RExpr* lhs;
    struct RExpr* rhs;
} RBinop;

// an expression
typedef union {
    Var* var;
    unsigned digit;
    RBinop* binop;
    struct RExpr* subexpr;
} rexpr_u;
typedef struct RExpr {
    expr_e type;
    rexpr_u val;
} RExpr;

// an assignment operation
typedef struct {
    Var* target;
    RExpr* expr;
} RAssign;

// represents a choice (possibly only one solution)
typedef struct RStep {
    bool advance; // avoid looping during pretty-print
    RAssign* assign; // maybe assign a new value
    unsigned nbguarded;
    struct RGuard* guarded; // choose any if satisfied
    struct RStep* unguarded; // otherwise go here
    uint id;
} RStep;

// represents a guarded instruction
typedef struct RGuard {
    RExpr* cond;
    RStep* next;
} RGuard; 

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
    uint nbloc;
    Var* locs;
    RStep* entrypoint;
} RProc;

// a reachability test
typedef struct {
    RExpr* cond;
} RCheck;

// a full program
typedef struct {
    uint nbglob;
    uint nbvar;
    Var* globs;
    uint nbproc;
    RProc* procs;
    uint nbcheck;
    RCheck* checks;
    uint nbstep;
} RProg;

RProg* tr_prog (Prog* in);
uint tr_var_list (Var** loc, Var* in);
uint tr_check_list (RCheck** loc, Check* in);
RExpr* tr_expr (Expr* in);
Var* locate_var (char* ident);
uint tr_proc_list (RProc** loc, Proc* in);

void tr_stmt (
    RStep** out, Stmt* in,
    bool advance, RStep* skipto, RStep* breakto);
RStep* tr_branch_list (
    uint* nb, RGuard** loc, Branch* in,
    bool advance, RStep* skipto, RStep* breakto);

#endif // REPR_H
