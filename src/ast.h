#ifndef AST_H
#define AST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct expr;
struct stmt;

typedef struct var {
    char* name;
    struct var* next;
} var_t;

typedef struct branch {
    struct expr* cond;
    struct stmt* stmt;
    struct branch* next;
} branch_t;

typedef struct {
    char* target;
    struct expr* expr;
} assign_t;

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
} stmt_t;

typedef struct {
    struct expr* lhs;
    struct expr* rhs;
} binop_t;

typedef enum {
    E_VAR, E_VAL,
    E_LESS, E_GREATER, E_EQUAL,
    E_NOT, E_AND, E_OR, E_ELSE,
    E_ADD, E_SUB, E_NEG,
} expr_e;
typedef union {
    char* ident;
    struct expr* expr;
    binop_t* binop;
    int digit;
} expr_u;
typedef struct expr {
    expr_e type;
    expr_u val;
} expr_t;


typedef struct proc {
    char* name;
    var_t* vars;
    stmt_t* stmts;
    struct proc* next;
} proc_t;

typedef struct check {
    expr_t* cond;
    struct check* next;
} check_t;

typedef struct {
    var_t* vars;
    proc_t* procs;
    check_t* checks;
} prog_t;

var_t* make_ident (char* s);
prog_t* make_prog (var_t* v, proc_t* p, check_t* c);
assign_t* make_assign (char* s, expr_t* e);
branch_t* make_branch (expr_t* cond, stmt_t* stmt);
stmt_u assign_as_s (assign_t* assign);
stmt_u branch_as_s (branch_t* branch);
stmt_u null_as_s ();
stmt_t* make_stmt (stmt_e type, stmt_u val);
proc_t* make_proc (char* name, var_t* vars, stmt_t* stmts);
expr_u int_as_e (int i);
expr_u expr_as_e (expr_t* expr);
binop_t* make_binop (expr_t* lhs, expr_t* rhs);
expr_u binop_as_e (binop_t* binop);
expr_u null_as_e ();
expr_u str_as_e (char* ident);
expr_t* make_expr (expr_e type, expr_u val);
check_t* make_check (expr_t* cond);

#endif // AST_H