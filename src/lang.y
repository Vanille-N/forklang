
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno;

int yylex();

void yyerror(char *s) {
	fflush(stdout);
	fprintf(stderr, "%s\n at line %d", s, yylineno);
}

#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

/***************************************************************************/
/* Data structures for storing a programme.                                */

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

/****************************************************************************/
/* All data pertaining to the programme are accessible from these two vars. */

prog_t* program;

/****************************************************************************/
/* Functions for settting up data structures at parse time.                 */

var_t* make_ident (char* s) {
    var_t* v = malloc(sizeof(var_t));
    v->name = s;
    v->next = NULL;
    return v;
}

prog_t* make_prog (var_t* v, proc_t* p, check_t* c) {
    prog_t* prog = malloc(sizeof(prog_t));
    prog->vars = v;
    prog->procs = p;
    prog->checks = c;
    return prog;
}

assign_t* make_assign (char* s, expr_t* e) {
    assign_t* assign = malloc(sizeof(assign_t));
    assign->target = s;
    assign->expr = e;
    return assign;
}

branch_t* make_branch (expr_t* cond, stmt_t* stmt) {
    branch_t* branch = malloc(sizeof(branch_t));
    branch->cond = cond;
    branch->stmt = stmt;
    branch->next = NULL;
    return branch;
}

stmt_u assign_as_s (assign_t* assign) {
    stmt_u val;
    val.assign = assign;
    return val;
}

stmt_u branch_as_s (branch_t* branch) {
    stmt_u val;
    val.branch = branch;
    return val;
}

stmt_u null_as_s () {
    stmt_u val;
    val._ = 0;
    return val;
}

stmt_t* make_stmt (stmt_e type, stmt_u val) {
    stmt_t* stmt = malloc(sizeof(stmt_t));
    stmt->type = type;
    stmt->val = val;
    stmt->next = NULL;
    return stmt;
}

proc_t* make_proc (char* name, var_t* vars, stmt_t* stmts) {
    proc_t* proc = malloc(sizeof(proc_t));
    proc->name = name;
    proc->vars = vars;
    proc->stmts = stmts;
    proc->next = NULL;
    return proc;
}

expr_u int_as_e (int i) {
    expr_u val;
    val.digit = i;
    return val;
}

expr_u expr_as_e (expr_t* expr) {
    expr_u val;
    val.expr = expr;
    return val;
}

binop_t* make_binop (expr_t* lhs, expr_t* rhs) {
    binop_t* binop = malloc(sizeof(binop_t));
    binop->lhs = lhs;
    binop->rhs = rhs;
    return binop;
}

expr_u binop_as_e (binop_t* binop) {
    expr_u val;
    val.binop = binop;
    return val;
}

expr_u null_as_e () {
    return int_as_e(0);
}

expr_u str_as_e (char* ident) {
    expr_u val;
    val.ident = ident;
    return val;
}

expr_t* make_expr (expr_e type, expr_u val) {
    expr_t* expr = malloc(sizeof(expr_t));
    expr->type = type;
    expr->val = val;
    return expr;
}

check_t* make_check (expr_t* cond) {
    check_t* check = malloc(sizeof(check_t));
    check->cond = cond;
    check->next = NULL;
    return check;
}

%}

/****************************************************************************/

%locations
%define parse.error verbose
%define parse.trace

/* types used by terminals and non-terminals */

%union {
	char* ident;
    int digit;
	var_t* var;
    proc_t* proc;
    check_t* check;
    stmt_t* stmt;
    expr_t* expr;
    branch_t* branch;
    expr_e bin;
}

%type <var> vars decls
%type <proc> procs procdef
%type <check> checks reach
%type <stmt> stmts stmt
%type <expr> expr
%type <branch> branches branch

%token DECL SEQ BRANCH THEN IF FI DO OD ELSE BREAK NOT
    ASSIGN PROC END REACH SKIP OPEN CLOSE
%token OR AND EQUAL ADD SUB GREATER LESS
%token <ident> IDENT
%token <digit> INT

%left SEQ
%left COMMA

%left OR
%left AND
%right NOT

%left LESS GREATER EQUAL

%left ADD
%left SUB

%%

prog : decls procs checks { program = make_prog($1, $2, $3); }
     ;

decls : DECL vars { $$ = $2; }
      | { $$ = NULL; }
      ;

vars : IDENT SEQ { $$ = make_ident($1); }
     | IDENT COMMA vars { ($$ = make_ident($1))->next = $3; }
     | IDENT SEQ DECL vars { ($$ = make_ident($1))->next = $4; }
     ;

procs : procdef { $$ = $1; }
      | procdef procs { ($$ = $1)->next = $2; }
      ;

procdef : PROC IDENT decls stmts END { $$ = make_proc($2, $3, $4); }
        ;

stmts : stmt { $$ = $1; }
      | stmt SEQ stmts { ($$ = $1)->next = $3; }
      ;

stmt : IDENT ASSIGN expr { $$ = make_stmt(S_ASSIGN, assign_as_s(make_assign($1, $3))); }
     | DO BRANCH branches OD { $$ = make_stmt(S_DO, branch_as_s($3)); }
     | IF BRANCH branches FI { $$ = make_stmt(S_IF, branch_as_s($3)); }
     | BREAK { $$ = make_stmt(S_BREAK, null_as_s()); }
     | SKIP { $$ = make_stmt(S_SKIP, null_as_s()); }
     ;

branches : branch { $$ = $1; }
         | branch BRANCH branches { ($$ = $1)->next = $3; }
         ;

branch : expr THEN stmts { $$ = make_branch($1, $3); }
       ;

expr : INT { $$ = make_expr(E_VAL, int_as_e($1)); }
     | IDENT { $$ = make_expr(E_VAR, str_as_e($1)); }
     | expr ADD expr { $$ = make_expr(E_ADD, binop_as_e(make_binop($1, $3))); }
     | expr SUB expr { $$ = make_expr(E_SUB, binop_as_e(make_binop($1, $3))); }
     | expr OR expr { $$ = make_expr(E_OR, binop_as_e(make_binop($1, $3))); }
     | expr AND expr { $$ = make_expr(E_AND, binop_as_e(make_binop($1, $3))); }
     | expr EQUAL expr { $$ = make_expr(E_EQUAL, binop_as_e(make_binop($1, $3))); }
     | expr GREATER expr { $$ = make_expr(E_GREATER, binop_as_e(make_binop($1, $3))); }
     | expr LESS expr { $$ = make_expr(E_LESS, binop_as_e(make_binop($1, $3))); }
     | OPEN expr CLOSE { $$ = $2; }
     | NOT expr { $$ = make_expr(E_NOT, expr_as_e($2)); }
     | SUB expr { $$ = make_expr(E_NEG, expr_as_e($2)); }
     | ELSE { $$ = make_expr(E_ELSE, null_as_e()); }
     ;


checks : reach YYEOF { $$ = $1; }
       | reach checks { ($$ = $1)->next = $2; }
       ;

reach : REACH expr { $$ = make_check($2); }
      ;

    
%%

#include "lex.yy.c"

/***************************************************************************/
/* pretty-printer                                                          */

void pp_stmt (int, stmt_t*);

void pp_indent (int num) {
    for (int i = 0; i < num; i++) {
        printf("    ");
    }
}

void pp_var (int indent, var_t* var) {
    if (var) {
        pp_indent(indent);
        printf("VAR [%s]\n", var->name);
        pp_var(indent, var->next);
    }
}

const char* str_of_expr_e (expr_e e) {
    switch (e) {
        case E_VAR: return "Var";
        case E_VAL: return "Val";
        case E_LESS: return "Lt";
        case E_GREATER: return "Gt";
        case E_EQUAL: return "Eq";
        case E_AND: return "And";
        case E_OR: return "Or";
        case E_ADD: return "Add";
        case E_SUB: return "Sub";
        case E_NOT: return "Not";
        case E_ELSE: return "Else";
        default: UNREACHABLE();
    }
}

void pp_expr (expr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("(%s)", expr->val.ident);
            break;
        case E_VAL:
            printf("(%d)", expr->val.digit);
            break;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            printf("(%s ", str_of_expr_e(expr->type));
            pp_expr(expr->val.binop->lhs);
            printf(" ");
            pp_expr(expr->val.binop->rhs);
            printf(")");
            break;
        case E_NOT:
        case E_NEG:
            printf("(%s ", str_of_expr_e(expr->type));
            pp_expr(expr->val.expr);
            printf(")");
            break;
        case E_ELSE:
            printf("(%s)", str_of_expr_e(expr->type));
            break;
        default:
            UNREACHABLE();
    }
}

void pp_branch (int indent, branch_t* branch) {
    if (branch) {
        pp_indent(indent);
        printf("WHEN ");
        pp_expr(branch->cond);
        printf("\n");
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_assign (int indent, assign_t* assign) {
    printf("SET [%s] <- ", assign->target);
    pp_expr(assign->expr);
    printf("\n");
}

void pp_stmt (int indent, stmt_t* stmt) {
    if (stmt) {
        switch (stmt->type) {
            case S_IF:
                pp_indent(indent);
                printf("CHOICE {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_DO:
                pp_indent(indent);
                printf("LOOP {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_ASSIGN:
                pp_indent(indent);
                pp_assign(indent, stmt->val.assign);
                break;
            case S_BREAK:
                pp_indent(indent);
                printf("BREAK\n");
                break;
            case S_SKIP:
                pp_indent(indent);
                printf("SKIP\n");
                break;
            default:
                UNREACHABLE();
        }
        pp_stmt(indent, stmt->next);
    }
}

void pp_proc (proc_t* proc) {
    if (proc) {
        putchar('\n');
        printf("PROC {%s} {\n", proc->name);
        pp_var(1, proc->vars);
        pp_stmt(1, proc->stmts);
        printf("}\n");
        pp_proc(proc->next); 
    }
}

void pp_check (check_t* check) {
    if (check) {
        printf("REACH? ");
        pp_expr(check->cond);
        printf("\n");
        pp_check(check->next);
    }
}

void pp_prog (prog_t* prog) {
    pp_var(0, prog->vars);
    pp_proc(prog->procs);
    pp_check(prog->checks);
}



/****************************************************************************/
/* programme interpreter      :                                             */


/****************************************************************************/


int main (int argc, char **argv) {
	if (argc <= 1) { yyerror("no file specified"); exit(1); }
	yyin = fopen(argv[1],"r");
	if (!yyparse()) pp_prog(program);
}
