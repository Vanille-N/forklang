#ifndef AST_H
#define AST_H

#include "prelude.h"

// Keep track of allocations for the ast
void free_ast (); // to be called as soon as the translation is performed
void free_var (); // to be called when the translated result is freed (vars are shared)

struct Expr;
struct Stmt;

// a variable
typedef struct Var {
    char* name;
    struct Var* next;
    uint id;
} Var;

// a nondeterministic choice
typedef struct Branch {
    struct Expr* cond;
    struct Stmt* stmt;
    struct Branch* next;
} Branch;

// an assignment statement
typedef struct {
    char* target;
    struct Expr* value;
} Assign;

// a statement
typedef enum { S_IF, S_DO, S_ASSIGN, S_BREAK, S_SKIP } StmtKind;
typedef struct Stmt {
    StmtKind type;
    union {
        Branch* branch;
        Assign* assign;
        int _;
    } val;
    struct Stmt* next;
    uint id;
} Stmt;

// a binary operation
typedef struct {
    struct Expr* lhs;
    struct Expr* rhs;
} Binop;

// an expression
typedef enum {
    E_VAR, E_VAL,
    E_LT, E_GT, E_EQ, E_GEQ, E_LEQ,
    E_NOT, E_AND, E_OR, E_NEG,
    E_ADD, E_SUB,
    E_MUL, E_MOD, E_DIV,
    E_RANGE,
} ExprKind;
typedef struct Expr {
    ExprKind type;
    union {
        char* ident;
        struct Expr* subexpr;
        Binop* binop;
        uint digit;
    } val;
} Expr;

#define MATCH_ANY_BINOP() \
    E_LT: case E_GT: case E_EQ: case E_GEQ: \
    case E_LEQ: case E_AND: case E_OR: \
    case E_ADD: case E_SUB: case E_MUL: \
    case E_MOD: case E_DIV: case E_RANGE
// usage:
// switch (op) {
//     case MATCH_ANY_BINOP(): foo(); break
//     default: UNREACHABLE();
// }

#define MATCH_ANY_MONOP() \
    E_NOT: case E_NEG

// a procedure
typedef struct Proc {
    char* name;
    Var* locs; // local variables
    Stmt* stmts;
    struct Proc* next;
} Proc;

// a reachability test
typedef struct Check {
    Expr* cond;
    struct Check* next;
} Check;

// a program
typedef struct {
    Var* globs;
    Proc* procs;
    Check* checks;
    uint nbvar;
    uint nbglob;
    uint nbstmt;
} Prog;

// Builders
Var* make_ident (char* name, uint id);
Prog* make_prog (Var* var, Proc* proc, Check* check);
Assign* make_assign (char* target, Expr* expr);
Branch* make_branch (Expr* cond, Stmt* stmt);
Stmt* make_stmt (StmtKind type, uint id);
Proc* make_proc (char* name, Var* vars, Stmt* stmts);
Binop* make_binop (Expr* lhs, Expr* rhs);
Expr* make_expr (ExprKind type);
Check* make_check (Expr* cond);

#endif // AST_H
