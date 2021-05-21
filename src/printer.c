#include "printer.h"

#include <stdlib.h>
#include <stdio.h>

#define PP_COLOR

bool use_color;
enum color {
    RED, GREEN, BLUE, PURPLE, CYAN,
    YELLOW, BLACK, RESET
};
const char* _ (enum color c) {
    if (use_color) {
        switch (c) {
            case RED: return "\x1b[1;31m";
            case GREEN: return "\x1b[0;32m";
            case BLUE: return "\x1b[1;34m";
            case PURPLE: return "\x1b[1;35m";
            case CYAN: return "\x1b[1;36m";
            case YELLOW: return "\x1b[1;33m";
            case BLACK: return "\x1b[0;90m";
            case RESET: return "\x1b[0m";
            default: UNREACHABLE();
        }
    } else {
        return "";
    }
}

void pp_ast (bool color, prog_t* prog) {
    use_color = color;
    pp_prog(prog);
}

void pp_stmt (uint, stmt_t*);

void pp_indent (uint num) {
    printf("%s|%s", _(BLUE), _(RESET));
    for (uint i = 0; i <= num; i++) {
        printf("  ");
    }
}

void pp_var (uint indent, var_t* var) {
    if (var) {
        pp_indent(indent);
        printf("%sVAR %s[%s]%s\n", _(PURPLE), _(RED), var->name, _(RESET));
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
            printf("%s(%s%s%s)%s", _(GREEN), _(RED), expr->val.ident, _(GREEN), _(RESET));
            break;
        case E_VAL:
            printf("%s(%d)%s", _(GREEN), expr->val.digit, _(RESET));
            break;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            printf("%s(%s%s ", _(GREEN), _(YELLOW), str_of_expr_e(expr->type));
            pp_expr(expr->val.binop->lhs);
            printf(" ");
            pp_expr(expr->val.binop->rhs);
            printf("%s)%s", _(GREEN), _(RESET));
            break;
        case E_NOT:
        case E_NEG:
            printf("%s(%s%s ", _(GREEN), _(YELLOW), str_of_expr_e(expr->type));
            pp_expr(expr->val.subexpr);
            printf("%s)%s", _(GREEN), _(RESET));
            break;
        default:
            UNREACHABLE();
    }
}

void pp_branch (uint indent, branch_t* branch) {
    if (branch) {
        pp_indent(indent);
        if (branch->cond) {
            printf("%sWHEN ", _(CYAN));
            pp_expr(branch->cond);
        } else {
            printf("%sELSE", _(CYAN));
        }
        printf("\n%s", _(RESET));
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_assign (assign_t* assign) {
    printf("SET %s[%s]%s <- ", _(RED), assign->target, _(RESET));
    pp_expr(assign->value);
    printf("\n");
}

void pp_stmt (uint indent, stmt_t* stmt) {
    if (stmt) {
        pp_indent(indent);
        printf("%s<%d> %s", _(BLACK), stmt->id, _(RESET));
        switch (stmt->type) {
            case S_IF:
                printf("%sCHOICE %s{\n", _(CYAN), _(RESET));
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_DO:
                printf("%sLOOP %s{\n", _(CYAN), _(RESET));
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_ASSIGN:
                pp_assign(stmt->val.assign);
                break;
            case S_BREAK:
                printf("%sBREAK%s\n", _(CYAN), _(RESET));
                break;
            case S_SKIP:
                printf("%sSKIP%s\n", _(CYAN), _(RESET));
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
        printf("%sPROC {%s} %s{\n", _(PURPLE), proc->name, _(RESET));
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
        printf("%sREACH? ", _(PURPLE));
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

// In order to avoid duplicates, record seen positions
// (loop) indicates that a statement continuation precedes it
//      it is detected by the translator that sets the `advance` field to false
// (merge) indicates that a statement has already been displayed
//      because it is reachable from more than one point
//      it is detected thanks to the below variable
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
    printf("%sref %s{%d as '%s'}%s", _(PURPLE), _(CYAN), var->id, var->name, _(RESET));
}

void pp_rcheck (rcheck_t* check) {
    pp_indent(0);
    printf("%sreach? %s", _(PURPLE), _(GREEN));
    pp_rexpr(check->cond);
    printf("%s\n", _(RESET));
}

void pp_rexpr (rexpr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("%s{%d as '%s'}%s", _(CYAN), expr->val.var->id, expr->val.var->name, _(GREEN));
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
    printf("%sthread '%s' %sentrypoint [%d]%s", _(PURPLE), proc->name, _(RED), proc->entrypoint->id, _(RESET));
    for (uint i = 0; i < proc->nbloc; i++) {
        printf("\n");
        pp_rvar(1, proc->locs+i);
    }
    pp_rstep(1, proc->entrypoint);
    printf("\n");
    pp_indent(0);
    printf("%send%s\n", _(PURPLE), _(RESET));
}

void pp_rstep (uint indent, rstep_t* step) {
    if (explored_steps[step->id]) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s(merge)%s", _(YELLOW), step->id, _(BLACK), _(RESET));
        return;
    }
    explored_steps[step->id] = true;
    if (step->assign) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s", _(YELLOW), step->id, _(RESET));
        pp_rassign(step->assign);
        if (step->unguarded) {
            printf(" %sthen [%d]%s", _(RED), step->unguarded->id, _(RESET));
        } else {
            printf(" %s<END>%s", _(BLACK), _(RESET));
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                printf("\n");
                pp_indent(indent);
                printf("%s<END>%s", _(BLACK), _(RESET));
            }
        } else {
            printf(" %s(loop)%s", _(BLACK), _(RESET));
        }
    } else if (step->nbguarded > 0) {
        printf("\n");
        pp_indent(indent);
        printf("%s<%d> %s%d guarded", _(YELLOW), step->id, _(BLACK), step->nbguarded);
        if (step->unguarded) { printf(", default"); }
        printf("%s", _(RESET));
        for (uint i = 0; i < step->nbguarded; i++) {
            pp_rguard(indent+1, step->guarded+i);
            if (step->advance) {
                pp_rstep(indent+2, step->guarded[i].next);
            } else {
                printf(" %s(loop)%s", _(BLACK), _(RESET));
            }
        }
        if (step->unguarded) {
            printf("\n");
            pp_indent(indent+1);
            printf("%selse %sjump [%d]%s", _(PURPLE), _(RED), step->unguarded->id, _(RESET));
            pp_rstep(indent+2, step->unguarded);
        }
        printf("\n");
        pp_indent(indent);
        printf("%s</>%s", _(BLACK), _(RESET));
    } else {
        if (step->unguarded) {
            printf("\n");
            pp_indent(indent);
            printf("%s<%d> %sskip [%d]%s", _(YELLOW), step->id, _(RED), step->unguarded->id, _(RESET));
        } else {
            printf("\n");
            pp_indent(indent);
            printf("%s<%d> %s<END>%s", _(YELLOW), step->id, _(BLACK), _(RESET));
            return;
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                printf(" %s<END>%s", _(BLACK), _(RESET));
            }
        } else {
            printf(" %s(loop)%s", _(BLACK), _(RESET));
        }
    }
}

void pp_rassign (rassign_t* assign) {
    printf("%s{%d as '%s'}%s <- %s", _(CYAN), assign->target->id, assign->target->name, _(RESET), _(GREEN));
    pp_rexpr(assign->expr);
    printf("%s", _(RESET));
}

void pp_rguard (uint indent, rguard_t* guard) {
    printf("\n");
    pp_indent(indent);
    printf("%swhen %s", _(PURPLE), _(GREEN));
    pp_rexpr(guard->cond);
    printf(" %sjump [%d]%s", _(RED), guard->next->id, _(RESET));
}




// Dot-readable

void dot_indent (uint num) {
    for (uint i = 0; i <= num; i++) {
        printf("  ");
    }
}

void dot_rprog (rprog_t* prog) {
    explored_steps = malloc(prog->nbstep * sizeof(bool));
    for (uint i = 0; i < prog->nbstep; i++) explored_steps[i] = false;
    printf("digraph {\n");
    printf("node [fontname=\"Mono\"]\n");
    printf("graph [fontname=\"Mono\"]\n");
    printf("edge [fontname=\"Mono\"]\n");
    for (uint i = 0; i < prog->nbglob; i++) {
        dot_rvar(prog->globs+i);
    }
    for (uint i = 1; i < prog->nbglob; i++) {
        printf("{ var_%d -> var_%d [style=invis] }\n",
            prog->globs[i-1].id, prog->globs[i].id);
    }
    for (uint i = 0; i < prog->nbproc; i++) {
        dot_rproc(prog->procs+i);
    }
    for (uint i = 0; i < prog->nbcheck; i++) {
        dot_rcheck(prog->checks+i, i);
    }
    for (uint i = 1; i < prog->nbcheck; i++) {
        printf("{ check_%d -> check_%d [style=invis] }\n", i-1, i);
    }
    printf("}\n");
    free(explored_steps);
}

void dot_rvar (var_t* var) {
    printf("{ var_%d [label=\"%s\" shape=box style=\"filled\" fillcolor=orange] }\n",
        var->id, var->name);
}

void dot_rcheck (rcheck_t* check, uint id) {
    printf("{ check_%d [label=\"", id);
    dot_rexpr(check->cond);
    printf("\" shape=diamond style=\"filled\" fillcolor=yellow] }\n");
}

void dot_rexpr (rexpr_t* expr) {
    switch (expr->type) {
        case E_VAR:
            printf("%s", expr->val.var->name);
            break;
        case E_VAL:
            printf("%d", expr->val.digit);
            break;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            printf("(");
            dot_rexpr(expr->val.binop->lhs);
            switch (expr->type) {
                case E_LESS: printf("<"); break;
                case E_GREATER: printf(">"); break;
                case E_EQUAL: printf("="); break;
                case E_AND: printf(" & "); break;
                case E_OR: printf(" | "); break;
                case E_ADD: printf("+"); break;
                case E_SUB: printf("-"); break;
                default: UNREACHABLE();
            }
            dot_rexpr(expr->val.binop->rhs);
            printf(")");
            break;
        case E_NOT:
        case E_NEG:
            if (expr->type == E_NOT) {
                printf("!");
            } else {
                printf("-");
            }
            dot_rexpr(expr->val.subexpr);
            break;
        default:
            UNREACHABLE();
    }
}

void dot_rproc (rproc_t* proc) {
    printf("{ th_%s [label=\"proc %s\" style=\"filled\" fillcolor=blue] }\n", proc->name, proc->name);
    for (uint i = 1; i < proc->nbloc; i++) {
        printf("{ var_%d -> var_%d [style=invis] }\n",
            proc->locs[i-1].id, proc->locs[i].id);
    }
    for (uint i = 0; i < proc->nbloc; i++) {
        printf("{ th_%s -> ", proc->name);
        dot_rvar(proc->locs+i);
        printf(" [constraint=false] }\n");
    }
    printf("th_%s -> ", proc->name);
    dot_rstep(proc->entrypoint);
}

void dot_rstep (rstep_t* step) {
    if (explored_steps[step->id]) return;
    explored_steps[step->id] = true;
    if (step->assign) {
        printf("{ st_%d [label=\"", step->id);
        dot_rassign(step->assign);
        printf("\" style=\"filled\" fillcolor=lightblue] }\n");
        if (step->unguarded) {
            printf("st_%d -> st_%d\n", step->id, step->unguarded->id);
        }
        if (step->advance && step->unguarded) dot_rstep(step->unguarded);
    } else if (step->nbguarded > 0) {
        printf("{ st_%d [label=\"branch\" style=\"filled\" fillcolor=purple] }\n",
            step->id);
        for (uint i = 0; i < step->nbguarded; i++) {
            printf("st_%d -> g_%d_%d\n", step->id, step->id, i);
            dot_rguard(step->id, i, step->guarded+i);
            if (step->advance) {
                dot_rstep(step->guarded[i].next);
            }
        }
        if (step->unguarded) {
            printf("st_%d -> st_%d\n", step->id, step->unguarded->id);
            dot_rstep(step->unguarded);
        }
    } else {
        printf("{ st_%d [label=\"skip\"] }\n", step->id);
        if (step->unguarded) {
            printf("st_%d -> st_%d\n", step->id, step->unguarded->id);
        } else {
            return;
        }
        if (step->advance && step->unguarded) dot_rstep(step->unguarded);
    }
}

void dot_rassign (rassign_t* assign) {
    printf("%s := ", assign->target->name);
    dot_rexpr(assign->expr);
}

void dot_rguard (uint parent_id, uint idx, rguard_t* guard) {
    printf("{ g_%d_%d [label=\"", parent_id, idx);
    dot_rexpr(guard->cond);
    printf("\" shape=diamond style=\"filled\" fillcolor=yellow] }\n");
    printf("g_%d_%d -> st_%d\n", parent_id, idx, guard->next->id);
}

