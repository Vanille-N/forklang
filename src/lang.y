%{

#include "prelude.h"
#include "ast.h"

int yylex ();

char* fname_src;
extern int yylineno;
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

// Uses the 'range operator' feature
bool use_range = false;

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
%type <check> checks
%type <stmt> stmts stmt
%type <expr> expr
%type <branch> branches branch else

%token DECL SEQ BRANCH THEN IF FI DO OD
%token ELSE BREAK NOT ASSIGN PROC END REACH SKIP OPEN CLOSE
%token OR AND EQ ADD SUB GT LT GEQ LEQ DIV MOD MUL RANGE
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

// Neat trick to chain together all variables:
//
//     var x,y;
//     var a;
//
// is _not_ seen as
//
//     (
//       ('var' START) x (',' SEPARATOR) y
//     )
//     (';' SEPARATOR)
//     (
//       ('var' START) a
//     )
//     (';' TERMINATOR)
//
// but rather as
//     ('var' START)
//     (x (',' SEPARATOR) y ('; var' SEPARATOR) a)
//     (';' TERMINATOR)
//
// this results in a list instead of a list of lists, and is
// much easier to handle
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

stmt : IDENT ASSIGN expr { ($$ = make_stmt(S_ASSIGN, unique_stmt_id++))->val.assign = make_assign($1, $3); }
     | DO BRANCH branches OD { ($$ = make_stmt(S_DO, unique_stmt_id++))->val.branch = $3; }
     | IF BRANCH branches FI { ($$ = make_stmt(S_IF, unique_stmt_id++))->val.branch = $3; }
     | BREAK { ($$ = make_stmt(S_BREAK, unique_stmt_id++))->val._ = 0; }
     | SKIP { ($$ = make_stmt(S_SKIP, unique_stmt_id++))->val._ = 0; }
     ;

// Prevent duplicate `else` or other branch after `else`
// Note that `::` aka BRANCH is considered as a separator rather than
// a start marker
branches : branch { $$ = $1; }
         | else { $$ = $1; }
         | branch BRANCH branches { ($$ = $1)->next = $3; }
         ;

branch : expr THEN stmts { $$ = make_branch($1, $3); }
       ;

else : ELSE THEN stmts { $$ = make_branch(NULL, $3); }
     ;

// Some duplication here, but attempting to factor all into an
// `expr binop expr` yields shift/reduce conflicts
expr : INT { ($$ = make_expr(E_VAL))->val.digit = $1; }
     | IDENT { ($$ = make_expr(E_VAR))->val.ident = $1; }
     | expr ADD expr { ($$ = make_expr(E_ADD))->val.binop = make_binop($1, $3); }
     | expr SUB expr { ($$ = make_expr(E_SUB))->val.binop = make_binop($1, $3); }
     | expr OR expr { ($$ = make_expr(E_OR))->val.binop = make_binop($1, $3); }
     | expr AND expr { ($$ = make_expr(E_AND))->val.binop = make_binop($1, $3); }
     | expr EQ expr { ($$ = make_expr(E_EQ))->val.binop = make_binop($1, $3); }
     | expr GT expr { ($$ = make_expr(E_GT))->val.binop = make_binop($1, $3); }
     | expr GEQ expr { ($$ = make_expr(E_GEQ))->val.binop = make_binop($1, $3); }
     | expr LT expr { ($$ = make_expr(E_LT))->val.binop = make_binop($1, $3); }
     | expr LEQ expr { ($$ = make_expr(E_LEQ))->val.binop = make_binop($1, $3); }
     | expr MUL expr { ($$ = make_expr(E_MUL))->val.binop = make_binop($1, $3); }
     | expr MOD expr { ($$ = make_expr(E_MOD))->val.binop = make_binop($1, $3); }
     | expr DIV expr { ($$ = make_expr(E_DIV))->val.binop = make_binop($1, $3); }
     | '{' expr RANGE expr '}' {
        use_range = true;
        ($$ = make_expr(E_RANGE))->val.binop = make_binop($2, $4); }
     | OPEN expr CLOSE { $$ = $2; }
     | NOT expr { ($$ = make_expr(E_NOT))->val.subexpr = $2; }
     | SUB expr { ($$ = make_expr(E_NEG))->val.subexpr = $2; }
     ;

checks : REACH expr checks { ($$ = make_check($2))->next = $3; }
       | { $$ = NULL; }
       ;

%%

#include "lex.yy.c"
#include "argparse.h"
#include "printer.h"
#include "exec.h"
#include "repr.h"

int main (int argc, char **argv) {
    {
        srand((unsigned)(unsigned long long)getpid());
    };
    Args* args = parse_args(argc, argv);
	if (!(yyin = fopen(args->fname_src, "r"))) {
        fprintf(stderr, "File not found '%s'\n", args->fname_src);
        show_help(false);
        free(args);
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
        // last use of `program
        if (args->flags&SHOW_REPR) pp_repr(stdout, !(args->flags&NO_COLOR), repr);
        if (args->flags&SHOW_DOT) make_dot(args->fname_src, repr);
        if (args->flags&EXEC_RAND) {
            Sat* sat = exec_prog_random(repr);
            pp_sat(repr, sat, !(args->flags&NO_COLOR), args->flags&SHOW_TRACE, false);
            free_sat();
            // `sat` does not exit this scope
        }
        if (args->flags&EXEC_ALL) {
            if (use_range) {
                fprintf(stderr, "The 'range operator' feature is not available with --all. Use --rand instead.\n");
            } else {
                Sat* sat = exec_prog_all(repr);
                pp_sat(repr, sat, !(args->flags&NO_COLOR), args->flags&SHOW_TRACE, true);
                free_sat();
                // `sat` does not exit this scope
            }
        }
        free_var();
        free_repr();
        // last use of `repr`
    } else {
        fclose(yyin);
        yylex_destroy();
        // parsing failed, cleanup ast anyway
    }
    free(args);
    free_ident();
    // final cleanup
}
