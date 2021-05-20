#include "printer.h"

const char* RED = "\x1b[1;31m";
const char* GREEN = "\x1b[0;32m";
const char* BLUE = "\x1b[1;34m";
const char* PURPLE = "\x1b[1;35m";
const char* CYAN = "\x1b[1;36m";
const char* YELLOW = "\x1b[1;33m";
const char* BLACK = "\x1b[0;90m";
const char* RESET = "\x1b[0m";

void pp_stmt (uint, stmt_t*);

void pp_indent (uint num) {
    printf("%s|%s", BLUE, RESET);
    for (uint i = 0; i <= num; i++) {
        printf("  ");
    }
}

void pp_var (uint indent, var_t* var) {
    if (var) {
        pp_indent(indent);
        printf("%sVAR %s[%s]%s\n", PURPLE, RED, var->name, RESET);
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
        case E_NEG: return "Neg";
        default: UNREACHABLE();
    }
}

void pp_expr (expr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("%s(%s%s%s)%s", GREEN, RED, expr->val.ident, GREEN, RESET);
            break;
        case E_VAL:
            printf("%s(%d)%s", GREEN, expr->val.digit, RESET);
            break;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            printf("%s(%s%s ", GREEN, YELLOW, str_of_expr_e(expr->type));
            pp_expr(expr->val.binop->lhs);
            printf(" ");
            pp_expr(expr->val.binop->rhs);
            printf("%s)%s", GREEN, RESET);
            break;
        case E_NOT:
        case E_NEG:
            printf("%s(%s%s ", GREEN, YELLOW, str_of_expr_e(expr->type));
            pp_expr(expr->val.subexpr);
            printf("%s)%s", GREEN, RESET);
            break;
        default:
            UNREACHABLE();
    }
}

void pp_branch (uint indent, branch_t* branch) {
    if (branch) {
        pp_indent(indent);
        if (branch->cond) {
            printf("%sWHEN ", CYAN);
            pp_expr(branch->cond);
        } else {
            printf("%sELSE", CYAN);
        }
        printf("\n%s", RESET);
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_assign (assign_t* assign) {
    printf("SET %s[%s]%s <- ", RED, assign->target, RESET);
    pp_expr(assign->expr);
    printf("\n");
}

void pp_stmt (uint indent, stmt_t* stmt) {
    if (stmt) {
        pp_indent(indent);
        printf("%s<%d> %s", BLACK, stmt->id, RESET);
        switch (stmt->type) {
            case S_IF:
                printf("%sCHOICE %s{\n", CYAN, RESET);
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_DO:
                printf("%sLOOP %s{\n", CYAN, RESET);
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_ASSIGN:
                pp_assign(stmt->val.assign);
                break;
            case S_BREAK:
                printf("%sBREAK%s\n", CYAN, RESET);
                break;
            case S_SKIP:
                printf("%sSKIP%s\n", CYAN, RESET);
                break;
            default:
                UNREACHABLE();
        }
        pp_stmt(indent, stmt->next);
    }
}

void pp_proc (proc_t* proc) {
    if (proc) {
        pp_indent(0);
        printf("%sPROC {%s} %s{\n", PURPLE, proc->name, RESET);
        pp_var(1, proc->locs);
        pp_stmt(1, proc->stmts);
        pp_indent(0);
        printf("}\n");
        pp_proc(proc->next); 
    }
}

void pp_check (check_t* check) {
    if (check) {
        pp_indent(0);
        printf("%sREACH? ", PURPLE);
        pp_expr(check->cond);
        printf("\n");
        pp_check(check->next);
    }
}

void pp_prog (prog_t* prog) {
    printf("-- %d variables | %d statements --\n", prog->nbvar, prog->nbstmt);
    pp_var(0, prog->globs);
    pp_proc(prog->procs);
    pp_check(prog->checks);
}



// Internal representation

bool* explored_steps;

void pp_rprog (rprog_t* prog) {
    explored_steps = malloc(prog->nbstep * sizeof(bool));
    for (uint i = 0; i < prog->nbstep; i++) { explored_steps[i] = false; }
    for (uint i = 0; i < prog->nbglob; i++) {
        pp_rvar(0, prog->globs+i);
        printf("\n");
    }
    for (uint i = 0; i < prog->nbproc; i++) {
        pp_rproc(prog->procs+i);
    }
    for (uint i = 0; i < prog->nbcheck; i++) {
        pp_rcheck(prog->checks+i);
    }
    free(explored_steps);
}

void pp_rvar (uint indent, var_t* var) {
    pp_indent(indent);
    printf("%sref %s{%d as '%s'}%s", PURPLE, CYAN, var->id, var->name, RESET);
}

void pp_rcheck (rcheck_t* check) {
    pp_indent(0);
    printf("%sreach? %s", PURPLE, GREEN);
    pp_rexpr(check->cond);
    printf("%s\n", RESET);
}

void pp_rexpr (rexpr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("%s{%d as '%s'}%s", CYAN, expr->val.var->id, expr->val.var->name, GREEN);
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
            pp_rexpr(expr->val.binop->lhs);
            printf(" ");
            pp_rexpr(expr->val.binop->rhs);
            printf(")");
            break;
        case E_NOT:
        case E_NEG:
            printf("(%s ", str_of_expr_e(expr->type));
            pp_rexpr(expr->val.subexpr);
            printf(")");
            break;
        default:
            UNREACHABLE();
    }
}

void pp_rproc (rproc_t* proc) {
    pp_indent(0);
    printf("%sthread '%s' %sentrypoint [%d]%s", PURPLE, proc->name, RED, proc->entrypoint->id, RESET);
    for (uint i = 0; i < proc->nbloc; i++) {
        printf("\n");
        pp_rvar(1, proc->locs+i);
    }
    pp_rstep(1, proc->entrypoint);
    printf("\n");
    pp_indent(0);
    printf("%send%s\n", PURPLE, RESET);
}

void pp_rstep (uint indent, rstep_t* step) {
    if (explored_steps[step->id]) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s(merge)%s", YELLOW, step->id, BLACK, RESET);
        return;
    }
    explored_steps[step->id] = true;
    if (step->assign) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s", YELLOW, step->id, RESET);
        pp_rassign(step->assign);
        if (step->unguarded) {
            printf(" %sthen [%d]%s", RED, step->unguarded->id, RESET);
        } else {
            printf(" %s<END>%s", BLACK, RESET);
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                printf("\n");
                pp_indent(indent);
                printf("%s<END>%s", BLACK, RESET);
            }
        } else {
            printf(" %s(loop)%s", BLACK, RESET);
        }
    } else if (step->nbguarded > 0) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s%d guarded", YELLOW, step->id, BLACK, step->nbguarded);
        if (step->unguarded) { printf(", default"); }
        printf("%s", RESET);
        for (uint i = 0; i < step->nbguarded; i++) {
            pp_rguard(indent+1, step->guarded+i);
            if (step->advance) {
                pp_rstep(indent+2, step->guarded[i].next);
            } else {
                printf(" %s(loop)%s", BLACK, RESET);
            }
        }
        if (step->unguarded) {
            printf("\n");
            pp_indent(indent+1);
            printf("%selse %sjump [%d]%s", PURPLE, RED, step->unguarded->id, RESET);
            pp_rstep(indent+2, step->unguarded);
        }
        printf("\n");
        pp_indent(indent);
        printf("%s</>%s", BLACK, RESET);
    } else {
        if (step->unguarded) {
            printf("\n");
            pp_indent(indent);
            printf("%s<%d> %sskip [%d]%s", YELLOW, step->id, RED, step->unguarded->id, RESET);
        } else {
            printf("\n");
            pp_indent(indent);
            printf("%s<%d> %s<END>%s", YELLOW, step->id, BLACK, RESET);
            return;
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                printf(" %s<END>%s", BLACK, RESET);
            }
        } else {
            printf(" %s(loop)%s", BLACK, RESET);
        }
    }
}

void pp_rassign (rassign_t* assign) {
    printf("%s{%d as '%s'}%s <- %s", CYAN, assign->target->id, assign->target->name, RESET, GREEN);
    pp_rexpr(assign->expr);
    printf("%s", RESET);
}

void pp_rguard (uint indent, rguard_t* guard) {
    printf("\n");
    pp_indent(indent);
    printf("%swhen %s", PURPLE, GREEN);
    pp_rexpr(guard->cond);
    printf(" %sjump [%d]%s", RED, guard->next->id, RESET);
}

