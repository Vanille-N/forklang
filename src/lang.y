
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "printer.h"
#include "exec.h"
#include "repr.h"

extern int yylineno;
char* fname_src;

int yylex ();

void yyerror (const char *s) {
	fflush(stdout);
	fprintf(stderr, "%s\nat %s:%d", s, fname_src, yylineno);
}

/***************************************************************************/
/* All output data available here */

Prog* program;

// Helper variables to assign unique identifiers to variables and statements
uint unique_var_id;
uint unique_stmt_id;

%}

/****************************************************************************/

%locations
%define parse.error verbose
%define parse.trace

// Types used by terminals and non-terminals

%union {
	char* ident;
    uint digit;
	Var* var;
    Proc* proc;
    Check* check;
    Stmt* stmt;
    Expr* expr;
    Branch* branch;
}

%type <var> vars decls
%type <proc> procs procdef
%type <check> checks reach
%type <stmt> stmts stmt
%type <expr> expr
%type <branch> branches branch else

%token DECL SEQ BRANCH THEN IF FI DO OD
%token ELSE BREAK NOT ASSIGN PROC END REACH SKIP OPEN CLOSE
%token OR AND EQ ADD SUB GT LT GEQ LEQ DIV MOD MUL
%token <ident> IDENT
%token <digit> INT

// Bind with low priority to separate statements
%left SEQ
%left COMMA

// Use as logical operators (not bitwise) makes low priority appropriate
%left OR
%left AND

%left LT GT GEQ LEQ EQ

%left ADD SUB
%left MUL DIV MOD 

// Negation affects only closest item
%right NOT

%%

prog : decls procs checks YYEOF {
        program = make_prog($1, $2, $3);
        program->nbvar = unique_var_id;
        program->nbstmt = unique_stmt_id;
     }
     ;

decls : DECL vars { $$ = $2; }
      | { $$ = NULL; }
      ;

vars : IDENT SEQ { $$ = make_ident($1, unique_var_id++); }
     | IDENT COMMA vars { ($$ = make_ident($1, unique_var_id++))->next = $3; }
     | IDENT SEQ DECL vars { ($$ = make_ident($1, unique_var_id++))->next = $4; }
     ;

procs : procdef procs { ($$ = $1)->next = $2; }
      | { $$ = NULL; }
      ;

procdef : PROC IDENT decls stmts END { $$ = make_proc($2, $3, $4); }
        ;

stmts : stmt { $$ = $1; }
      | stmt SEQ stmts { ($$ = $1)->next = $3; }
      ;

stmt : IDENT ASSIGN expr { $$ = make_stmt(S_ASSIGN, assign_as_s(make_assign($1, $3)), unique_stmt_id++); }
     | DO BRANCH branches OD { $$ = make_stmt(S_DO, branch_as_s($3), unique_stmt_id++); }
     | IF BRANCH branches FI { $$ = make_stmt(S_IF, branch_as_s($3), unique_stmt_id++); }
     | BREAK { $$ = make_stmt(S_BREAK, null_as_s(), unique_stmt_id++); }
     | SKIP { $$ = make_stmt(S_SKIP, null_as_s(), unique_stmt_id++); }
     ;

branches : branch { $$ = $1; }
         | else { $$ = $1; }
         | branch BRANCH branches { ($$ = $1)->next = $3; }
         ;

branch : expr THEN stmts { $$ = make_branch($1, $3); }
       ;

else : ELSE THEN stmts { $$ = make_branch(NULL, $3); }
     ;

expr : INT { $$ = make_expr(E_VAL, uint_as_e($1)); }
     | IDENT { $$ = make_expr(E_VAR, str_as_e($1)); }
     | expr ADD expr { $$ = make_expr(E_ADD, binop_as_e(make_binop($1, $3))); }
     | expr SUB expr { $$ = make_expr(E_SUB, binop_as_e(make_binop($1, $3))); }
     | expr OR expr { $$ = make_expr(E_OR, binop_as_e(make_binop($1, $3))); }
     | expr AND expr { $$ = make_expr(E_AND, binop_as_e(make_binop($1, $3))); }
     | expr EQ expr { $$ = make_expr(E_EQ, binop_as_e(make_binop($1, $3))); }
     | expr GT expr { $$ = make_expr(E_GT, binop_as_e(make_binop($1, $3))); }
     | expr GEQ expr { $$ = make_expr(E_GEQ, binop_as_e(make_binop($1, $3))); }
     | expr LT expr { $$ = make_expr(E_LT, binop_as_e(make_binop($1, $3))); }
     | expr LEQ expr { $$ = make_expr(E_LEQ, binop_as_e(make_binop($1, $3))); }
     | expr MUL expr { $$ = make_expr(E_MUL, binop_as_e(make_binop($1, $3))); }
     | expr MOD expr { $$ = make_expr(E_MOD, binop_as_e(make_binop($1, $3))); }
     | expr DIV expr { $$ = make_expr(E_DIV, binop_as_e(make_binop($1, $3))); }
     | OPEN expr CLOSE { $$ = $2; }
     | NOT expr { $$ = make_expr(E_NOT, expr_as_e($2)); }
     | SUB expr { $$ = make_expr(E_NEG, expr_as_e($2)); }
     ;


checks : reach checks { ($$ = $1)->next = $2; }
       | { $$ = NULL; }
       ;

reach : REACH expr { $$ = make_check($2); }
      ;

    
%%

#include "lex.yy.c"
#include "argparse.h"

int main (int argc, char **argv) {
    Args* args = parse_args(argc, argv);
	if (!(yyin = fopen(args->fname_src, "r"))) {
        fprintf(stderr, "File not found '%s'\n", args->fname_src);
        show_help(false);
        exit(2);
    }
    fname_src = args->fname_src;
    unique_var_id = 0;
    unique_stmt_id = 0;
	if (!yyparse()) {
        if (args->flags&SHOW_AST) pp_ast(stdout, !(args->flags&NO_COLOR), program);
        RProg* repr = tr_prog(program);
        free_ast();
        fclose(yyin);
        yylex_destroy();
        if (args->flags&SHOW_REPR) pp_repr(stdout, !(args->flags&NO_COLOR), repr);
        if (args->flags&SHOW_DOT) make_dot(args->fname_src, repr);
        if (args->flags&EXEC_RAND) {
            Sat* sat = exec_prog_random(repr);
            pp_sat(repr, sat, !(args->flags&NO_COLOR), args->flags&SHOW_TRACE);
            free_sat();
        }
        if (args->flags&EXEC_ALL) {
            Sat* sat = exec_prog_all(repr);
            pp_sat(repr, sat, !(args->flags&NO_COLOR), args->flags&SHOW_TRACE);
            free_sat();
        }
        free_var();
        free_repr();
    } else {
        fclose(yyin);
        yylex_destroy();
    }
    free(args);
    free_ident();
}
