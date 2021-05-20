#ifndef PRINTER_H
#define PRINTER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "repr.h"

typedef unsigned uint;

#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

/***************************************************************************/
/* pretty-printer                                                          */

void pp_stmt (uint, stmt_t*);
void pp_indent (uint num);
void pp_var (uint indent, var_t* var);
const char* str_of_expr_e (expr_e e);
void pp_expr (expr_t* expr);
void pp_branch (uint indent, branch_t* branch);
void pp_assign (uint indent, assign_t* assign);
void pp_stmt (uint indent, stmt_t* stmt);
void pp_proc (proc_t* proc);
void pp_check (check_t* check);
void pp_prog (prog_t* prog);


/* internal representation */

void pp_rprog (rprog_t* prog);
void pp_rvar (uint indent, rvar_t* var);
void pp_rcheck (rcheck_t* check);
void pp_rexpr (rexpr_t* expr);
void pp_rproc (rproc_t* proc);
void pp_rassign (rassign_t* assign);
void pp_rguard (uint indent, rguard_t* guard);
void pp_rstep (uint indent, rstep_t* step);



#endif // PRINTER_H
