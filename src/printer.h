#ifndef PRINTER_H
#define PRINTER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

/***************************************************************************/
/* pretty-printer                                                          */

void pp_stmt (int, stmt_t*);
void pp_indent (int num);
void pp_var (int indent, var_t* var);
const char* str_of_expr_e (expr_e e);
void pp_expr (expr_t* expr);
void pp_branch (int indent, branch_t* branch);
void pp_assign (int indent, assign_t* assign);
void pp_stmt (int indent, stmt_t* stmt);
void pp_proc (proc_t* proc);
void pp_check (check_t* check);
void pp_prog (prog_t* prog);

#endif // PRINTER_H
