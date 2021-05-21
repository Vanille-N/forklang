#ifndef PRINTER_H
#define PRINTER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "repr.h"

typedef unsigned uint;

// Pretty-print parsed ast
// (i.e. "Niveau 1")

void pp_indent (uint num);

void pp_stmt (uint, stmt_t*);
void pp_var (uint indent, var_t* var);
const char* str_of_expr_e (expr_e e);
void pp_expr (expr_t* expr);
void pp_branch (uint indent, branch_t* branch);
void pp_assign (assign_t* assign);
void pp_stmt (uint indent, stmt_t* stmt);
void pp_proc (proc_t* proc);
void pp_check (check_t* check);
void pp_prog (prog_t* prog);


// Pretty-print internal representation

void pp_rprog (rprog_t* prog);
void pp_rvar (uint indent, var_t* var);
void pp_rcheck (rcheck_t* check);
void pp_rexpr (rexpr_t* expr);
void pp_rproc (rproc_t* proc);
void pp_rassign (rassign_t* assign);
void pp_rguard (uint indent, rguard_t* guard);
void pp_rstep (uint indent, rstep_t* step);

// Dump dot-readable format

void dot_rprog (rprog_t* prog);
void dot_rvar (uint indent, var_t* var);
void dot_rcheck (rcheck_t* check);
void dot_rexpr (rexpr_t* expr);
void dot_rproc (rproc_t* proc);
void dot_rassign (rassign_t* assign);
void dot_rguard (uint indent, uint parent_id, uint idx, rguard_t* guard);
void dot_rstep (uint indent, rstep_t* step);

#endif // PRINTER_H
