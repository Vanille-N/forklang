#ifndef AST_H
#define AST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct expr;
struct stmt;

// a variable
typedef struct var {
    char* name;
    struct var* next;
    int id;
} var_t;

// a nondeterministic choice
typedef struct branch {
    struct expr* cond;
    struct stmt* stmt;
    struct branch* next;
} branch_t;

// an assignment statement
typedef struct {
    char* target;
    struct expr* expr;
} assign_t;

// a statement
typedef enum { S_IF, S_DO, S_ASSIGN, S_BREAK, S_SKIP } stmt_e;
typedef union {
    branch_t* branch;
    assign_t* assign;
    int _;
} stmt_u;
typedef struct stmt {
    stmt_e type;
    stmt_u val;
    struct stmt* next;
    int id;
} stmt_t;

// a binary operation
typedef struct {
    struct expr* lhs;
    struct expr* rhs;
} binop_t;

// an expression
typedef enum {
    E_VAR, E_VAL,
    E_LESS, E_GREATER, E_EQUAL,
    E_NOT, E_AND, E_OR,
    E_ADD, E_SUB, E_NEG,
} expr_e;
typedef union {
    char* ident;
    struct expr* subexpr;
    binop_t* binop;
    unsigned digit;
} expr_u;
typedef struct expr {
    expr_e type;
    expr_u val;
} expr_t;

// a procedure
typedef struct proc {
    char* name;
    var_t* vars;
    stmt_t* stmts;
    struct proc* next;
} proc_t;

// a reachability test
typedef struct check {
    expr_t* cond;
    struct check* next;
} check_t;

// a program
typedef struct {
    var_t* vars;
    proc_t* procs;
    check_t* checks;
    uint nbvar;
    uint nbstmt;
} prog_t;

// Builders
var_t* make_ident (char* s, uint id);
prog_t* make_prog (var_t* v, proc_t* p, check_t* c);
assign_t* make_assign (char* s, expr_t* e);
branch_t* make_branch (expr_t* cond, stmt_t* stmt);
stmt_u assign_as_s (assign_t* assign);
stmt_u branch_as_s (branch_t* branch);
stmt_u null_as_s ();
stmt_t* make_stmt (stmt_e type, stmt_u val, uint id);
proc_t* make_proc (char* name, var_t* vars, stmt_t* stmts);
expr_u uint_as_e (unsigned i);
expr_u expr_as_e (expr_t* expr);
binop_t* make_binop (expr_t* lhs, expr_t* rhs);
expr_u binop_as_e (binop_t* binop);
expr_u str_as_e (char* ident);
expr_t* make_expr (expr_e type, expr_u val);
check_t* make_check (expr_t* cond);

#endif // AST_H
