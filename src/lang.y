
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
%token OR AND EQUAL ADD SUB GREATER LESS
%token <ident> IDENT
%token <digit> INT

// Bind with low priority to separate statements
%left SEQ
%left COMMA

// Use as logical operators (not bitwise) makes low priority appropriate
%left OR
%left AND

%left LESS GREATER EQUAL

%left ADD
%left SUB

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
     | expr EQUAL expr { $$ = make_expr(E_EQUAL, binop_as_e(make_binop($1, $3))); }
     | expr GREATER expr { $$ = make_expr(E_GREATER, binop_as_e(make_binop($1, $3))); }
     | expr LESS expr { $$ = make_expr(E_LESS, binop_as_e(make_binop($1, $3))); }
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

typedef enum {
    SHOW_AST = 1, SHOW_REPR = 2, SHOW_DOT = 4,
    EXEC_RAND = 8, EXEC_ALL = 16,
    SHOW_TRACE = 32,
    NO_COLOR = 64,
    HELP = 128,
} Option;

typedef struct {
    char* fname_src;
    uint flags;
} Args;

typedef struct {
    char* long_name;
    char short_name;
    Option option;
    char* help_message;
} Flag;

Flag opt_flags [] = {
    { "ast", 'a', SHOW_AST, "Pretty-print the syntax tree" },
    { "repr", 'r', SHOW_REPR, "Pretty-print the internal graph representation" },
    { "dot", 'd', SHOW_DOT, "Dump graphviz file and render as png" },
    { "rand", 'R', EXEC_RAND, "Perform Monte-Carlo execution" },
    { "all", 'A', EXEC_ALL, "Perform exhaustive execution" },
    { "trace", 't', SHOW_TRACE, "Show sequence of steps to satisfy checks" },
    { "no-color", 'c', NO_COLOR, "Do not use ANSI color codes in pretty-prints" },
    { "help", 'h', HELP, "Show help message and exit" },
    { NULL, 0, 0, NULL },
};

void show_help () {
    printf("lang\n");
    printf("  Parser, pretty-printer and simulator\n");
    printf("\n");
    printf("  Usage: lang [FILE] [FLAGS]\n");
    printf("  Flags:\n");
    for (uint j = 0; opt_flags[j].long_name; j++) {
        printf("    -%c, --%-10s   %50s\n",
            opt_flags[j].short_name,
            opt_flags[j].long_name,
            opt_flags[j].help_message);
    }
    printf("  Examples:\n");
    printf("      lang -ar input.prog --no-color\n");
    printf("      lang input.prog --rand --all -c -t\n");
    printf("      lang -h");
}

uint multiflags (char* c) {
    uint acc = 0;
    for (; *c; c++) {
        uint j;
        for (j = 0; opt_flags[j].long_name; j++) {
            if (*c == opt_flags[j].short_name) {
                if (acc & opt_flags[j].option) {
                    fprintf(stderr, "Warning: duplicate flag '%c' is ignored\n", *c);
                }
                acc |= opt_flags[j].option;
                break;
            }
        }
        if (!opt_flags[j].long_name) {
            fprintf(stderr, "Unknown flag '%c'\n", *c);
            exit(1);
        }
    }
    return acc;
}

uint find_option (char* arg) {
    if (arg[1] == '-') { // caller must check arg[0] == '-'
        for (uint j = 0; opt_flags[j].long_name; j++) {
            if (0 == strcmp(arg+2, opt_flags[j].long_name)) {
                return opt_flags[j].option;
            }
        }
        return 0;
    }
    return multiflags(arg+1);
}

Args* parse_args (int argc, char** argv) {
    Args* args = malloc(sizeof(Args));
    args->fname_src = NULL;
    args->flags = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            uint opt = find_option(argv[i]);
            if (opt) {
                if (args->flags & opt) {
                    fprintf(stderr,
                        "Warning: duplicate flag '%s' is ignored\n",
                        argv[i]);
                }
                args->flags |= opt;
            } else {
                fprintf(stderr, "No such option '%s'\n", argv[i]);
                show_help();
                exit(1);
            }
        } else {
            if (args->fname_src) {
                fprintf(stderr,
                    "Warning: duplicate filename '%s' is ignored\n",
                    argv[i]);
            } else {
                args->fname_src = argv[i];
            }
        }
    }
    if ((args->flags&SHOW_TRACE)
        && !(args->flags&EXEC_RAND)
        && !(args->flags&EXEC_ALL)) {
            fprintf(stderr,
                "Warning: --trace is useless without either --rand or --all\n");
    }
    if (args->flags&HELP) {
        show_help();
        exit(0);
    }
    if (!args->fname_src) {
        fprintf(stderr, "No file specified\n");
        show_help();
        exit(1);
    }
    return args;
}


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
