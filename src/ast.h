#ifndef AST_H
#define AST_H

typedef unsigned int uint;

// Keep track of allocations for the ast
void free_ast ();
void free_var ();

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
typedef union {
    Branch* branch;
    Assign* assign;
    int _;
} StmtData;
typedef struct Stmt {
    StmtKind type;
    StmtData val;
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
    E_LESS, E_GREATER, E_EQUAL,
    E_NOT, E_AND, E_OR,
    E_ADD, E_SUB, E_NEG,
} ExprKind;
typedef union {
    char* ident;
    struct Expr* subexpr;
    Binop* binop;
    uint digit;
} ExprData;
typedef struct Expr {
    ExprKind type;
    ExprData val;
} Expr;

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
Var* make_ident (char* s, uint id);
Prog* make_prog (Var* v, Proc* p, Check* c);
Assign* make_assign (char* s, Expr* e);
Branch* make_branch (Expr* cond, Stmt* stmt);
StmtData assign_as_s (Assign* assign);
StmtData branch_as_s (Branch* branch);
StmtData null_as_s ();
Stmt* make_stmt (StmtKind type, StmtData val, uint id);
Proc* make_proc (char* name, Var* vars, Stmt* stmts);
ExprData uint_as_e (uint i);
ExprData expr_as_e (Expr* expr);
Binop* make_binop (Expr* lhs, Expr* rhs);
ExprData binop_as_e (Binop* binop);
ExprData str_as_e (char* ident);
Expr* make_expr (ExprKind type, ExprData val);
Check* make_check (Expr* cond);

#endif // AST_H
