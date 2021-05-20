#include "printer.h"

void pp_stmt (uint, stmt_t*);

void pp_indent (uint num) {
    for (uint i = 0; i < num; i++) {
        printf("  ");
    }
}

void pp_var (uint indent, var_t* var) {
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
        case E_NEG: return "Neg";
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
            pp_expr(expr->val.subexpr);
            printf(")");
            break;
        default:
            UNREACHABLE();
    }
}

void pp_branch (uint indent, branch_t* branch) {
    if (branch) {
        pp_indent(indent);
        printf("WHEN ");
        if (branch->cond) {
            pp_expr(branch->cond);
        } else {
            printf("Else");
        }
        printf("\n");
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_assign (uint indent, assign_t* assign) {
    printf("SET [%s] <- ", assign->target);
    pp_expr(assign->expr);
    printf("\n");
}

void pp_stmt (uint indent, stmt_t* stmt) {
    if (stmt) {
        pp_indent(indent);
        printf("<%d> ", stmt->id);
        switch (stmt->type) {
            case S_IF:
                printf("CHOICE {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_DO:
                printf("LOOP {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_ASSIGN:
                pp_assign(indent, stmt->val.assign);
                break;
            case S_BREAK:
                printf("BREAK\n");
                break;
            case S_SKIP:
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
    printf("-- %d variables | %d statements --\n", prog->nbvar, prog->nbstmt);
    pp_var(0, prog->vars);
    pp_proc(prog->procs);
    pp_check(prog->checks);
}



// Internal representation

bool* explored_steps;

void pp_rprog (rprog_t* prog) {
    explored_steps = malloc(prog->nbstep * sizeof(bool));
    for (uint i = 0; i < prog->nbstep; i++) { explored_steps[i] = false; }
    for (uint i = 0; i < prog->nbvar; i++) {
        pp_rvar(0, prog->vars+i);
    }
    for (uint i = 0; i < prog->nbproc; i++) {
        pp_rproc(prog->procs+i);
    }
    for (uint i = 0; i < prog->nbcheck; i++) {
        pp_rcheck(prog->checks+i);
    }
    free(explored_steps);
}

void pp_rvar (uint indent, rvar_t* var) {
    pp_indent(indent);
    printf("ref [%d] as '%s'\n", var->id, var->name);
}

void pp_rcheck (rcheck_t* check) {
    printf("reach? ");
    pp_rexpr(check->cond);
    printf("\n");
}

void pp_rexpr (rexpr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("{%d as '%s'}", expr->val.var->id, expr->val.var->name);
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
     printf("thread '%s' entrypoint [%d]\n", proc->name, proc->entrypoint->id);
     for (uint i = 0; i < proc->nbvar; i++) {
         pp_rvar(1, proc->vars+i);
     }
     pp_rstep(1, proc->entrypoint);
     printf("end\n");
}

void pp_rstep (uint indent, rstep_t* step) {
    if (explored_steps[step->id]) {
        pp_indent(indent);
        printf("<%d> (merge)\n", step->id);
        return;
    }
    explored_steps[step->id] = true;
    pp_indent(indent);
    if (step->assign) {
        printf("<%d> ", step->id);
        pp_rassign(step->assign);
        if (step->unguarded) {
            printf(" then [%d]", step->unguarded->id);
        } else {
            printf(" <END>");
        }
        if (step->advance) {
            printf("\n");
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                pp_indent(indent);
                printf("<END>\n");
            }
        } else {
            printf(" (loop)\n");
        }
    } else if (step->nbguarded > 0) {
        printf("<%d> %d guarded", step->id, step->nbguarded);
        if (step->unguarded) { printf(", default"); }
        printf("\n");
        for (int i = 0; i < step->nbguarded; i++) {
            pp_rguard(indent+1, step->guarded+i);
            if (step->advance) {
                printf("\n");
                pp_rstep(indent+1, step->guarded[i].next);
            } else {
                printf(" (loop)\n");
            }
        }
        if (step->unguarded) {
            pp_indent(indent+1);
            printf("else jump to [%d]\n", step->unguarded->id);
            pp_rstep(indent+1, step->unguarded);
        }
        pp_indent(indent);
        printf("</>\n");
    } else {
        if (step->unguarded) {
            printf("<%d> skip to [%d]", step->id, step->unguarded->id);
        } else {
            printf("<%d> <END>\n", step->id);
            return;
        }
        if (step->advance) {
            printf("\n");
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                printf(" <END>");
            }
        } else {
            printf(" (loop)");
        }
    }
}

void pp_rassign (rassign_t* assign) {
    printf("{%d as '%s'} <- ", assign->target->id, assign->target->name);
    pp_rexpr(assign->expr);
}

void pp_rguard (uint indent, rguard_t* guard) {
    pp_indent(indent);
    printf("when ");
    pp_rexpr(guard->cond);
    printf(" jump to [%d]", guard->next->id);
}

