
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "printer.h"

extern int yylineno;

int yylex ();

void yyerror (const char *s) {
	fflush(stdout);
	fprintf(stderr, "%s\nat line %d", s, yylineno);
}

/****************************************************************************/
/* All data pertaining to the programme are accessible from these two vars. */

var_t* globals;
var_t* curr_locals;
prog_t* program;

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

%token DECL SEQ BRANCH THEN IF FI DO OD
%token ELSE BREAK NOT ASSIGN PROC END REACH SKIP OPEN CLOSE
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

/****************************************************************************/


int main (int argc, char **argv) {
	if (argc <= 1) { yyerror("no file specified"); exit(1); }
	yyin = fopen(argv[1],"r");
	if (!yyparse()) pp_prog(program);
}
