
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "printer.h"
#include "exec.h"
#include "repr.h"

extern int yylineno;

int yylex ();

void yyerror (const char *s) {
	fflush(stdout);
	fprintf(stderr, "%s\nat line %d:%d", s, yylineno);
}

/***************************************************************************/
/* All output data available here */

prog_t* program;

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
    int digit;
	var_t* var;
    proc_t* proc;
    check_t* check;
    stmt_t* stmt;
    expr_t* expr;
    branch_t* branch;
    expr_e bin;
}

%type <var> vars glob_decls loc_decls
%type <proc> procs procdef
%type <check> checks reach
%type <stmt> stmts stmt
%type <expr> expr
%type <branch> branches branch else

%token DECL SEQ BRANCH THEN IF FI DO OD
%token ELSE BREAK NOT ASSIGN PROC END REACH SKIP OPEN CLOSE
%token OR AND EQUAL ADD SUB GREATER LESS
%token <ident> IDENT
%token <digit> INT

// Bind with low priority to separate statements
%left SEQ
%left COMMA

%left OR
%left AND

%left LESS GREATER EQUAL

%left ADD
%left SUB

%right NOT

%%

prog : glob_decls procs checks {
        program = make_prog($1, $2, $3);
        program->nbvar = unique_var_id;
        program->nbstmt = unique_stmt_id;
     }
     ;

glob_decls : DECL vars { $$ = $2; }
           ;

vars : IDENT SEQ { $$ = make_ident($1, unique_var_id++); }
     | IDENT COMMA vars { ($$ = make_ident($1, unique_var_id++))->next = $3; }
     | IDENT SEQ DECL vars { ($$ = make_ident($1, unique_var_id++))->next = $4; }
     ;

procs : procdef { $$ = $1; }
      | procdef procs { ($$ = $1)->next = $2; }
      ;

procdef : PROC IDENT loc_decls stmts END { $$ = make_proc($2, $3, $4); }
        ;

loc_decls : DECL vars { $$ = $2; }
          | { $$ = NULL; }
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
     | expr EQUAL expr { $$ = make_expr(E_EQUAL, binop_as_e(make_binop($1, $3))); }
     | expr GREATER expr { $$ = make_expr(E_GREATER, binop_as_e(make_binop($1, $3))); }
     | expr LESS expr { $$ = make_expr(E_LESS, binop_as_e(make_binop($1, $3))); }
     | OPEN expr CLOSE { $$ = $2; }
     | NOT expr { $$ = make_expr(E_NOT, expr_as_e($2)); }
     | SUB expr { $$ = make_expr(E_NEG, expr_as_e($2)); }
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
	if (argc <= 1) {
        yyerror("No file specified");
        exit(1);
    }
    bool show_ast = false;
    bool show_repr = false;
    bool execrand = false;
    bool execall = false;
    for (int i = 2; i < argc; i++) {
        if (0 == strcmp("--ast", argv[i])) {
            show_ast = true;
        } else if (0 == strcmp("--repr", argv[i])) {
            show_repr = true;
        } else if (0 == strcmp("--rand", argv[i])) {
            execrand = true;
        } else if (0 == strcmp("--all", argv[i])) {
            execall = true;
        } else {
            fprintf(stderr, "No such option '%s'", argv[i]);
            exit(2);
        }
    } 
	yyin = fopen(argv[1], "r");
    unique_var_id = 0;
    unique_stmt_id = 0;
	if (!yyparse()) {
        if (show_ast) {
            printf("\n=== PARSED AST ===\n\n");
            pp_prog(program);
        }
        rprog_t* repr = tr_prog(program);
        free_ast();
        if (show_repr) {
            printf("\n=== INTERNAL REPRESENTATION ===\n\n");
            pp_rprog(repr);
        }
        if (execrand) {
            exec_prog_random(repr);
        }
        if (execall) {
            exec_prog_all(repr);
        }
        free_var();
        free_repr();
    }
    fclose(yyin);
}
