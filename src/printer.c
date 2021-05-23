#include "printer.h"

#include <stdlib.h>
#include <string.h>

bool use_color;
FILE* fout;

#define COLOR(c) use_color ? "\x1b[" #c "m" : ""
#define RED COLOR(1;31)
#define GREEN COLOR(0;32)
#define BLUE COLOR(1;34)
#define PURPLE COLOR(1;95)
#define CYAN COLOR(1;36)
#define YELLOW COLOR(1;33)
#define BLACK COLOR(0;97)
#define RESET COLOR(0)

void pp_indent (uint num) {
    fprintf(fout, "%s||%s", BLUE, RESET);
    for (uint i = 0; i <= num; i++) {
        fprintf(fout, "  ");
    }
}

const char* str_of_expr_e (ExprKind e) {
    switch (e) {
        case E_VAR: return "Var";
        case E_VAL: return "Val";
        case E_LT: return "Lt";
        case E_LEQ: return "Leq";
        case E_GEQ: return "Geq";
        case E_GT: return "Gt";
        case E_EQ: return "Eq";
        case E_AND: return "And";
        case E_OR: return "Or";
        case E_ADD: return "Add";
        case E_SUB: return "Sub";
        case E_NOT: return "Not";
        case E_NEG: return "Neg";
        case E_MUL: return "Mul";
        case E_MOD: return "Mod";
        case E_DIV: return "Div";
        case E_RANGE: return "Rng";
        default: UNREACHABLE();
    }
}

void pp_prog (Prog* prog);
void pp_proc (Proc* proc);
void pp_branch (uint indent, Branch* branch);
void pp_stmt (uint indent, Stmt* stmt);
void pp_assign (Assign* assign);
void pp_expr (Expr* expr);
void pp_var (uint indent, Var* var, bool isglob);
void pp_check (Check* check);

void pp_ast (FILE* f, bool color, Prog* prog) {
    fout = f;
    use_color = color;
    pp_prog(prog);
}

void pp_prog (Prog* prog) {
    fprintf(fout, "%s================= AST ==================%s\n", BLUE, RESET);
    pp_var(0, prog->globs, true);
    pp_proc(prog->procs);
    pp_check(prog->checks);
    fprintf(fout, "%s========================================%s\n\n", BLUE, RESET);
}

void pp_proc (Proc* proc) {
    if (proc) {
        pp_indent(0);
        fprintf(fout, "%sPROC {%s} %s{\n", PURPLE, proc->name, RESET);
        pp_var(1, proc->locs, false);
        pp_stmt(1, proc->stmts);
        pp_indent(0);
        fprintf(fout, "}\n");
        pp_proc(proc->next); 
    }
}

void pp_branch (uint indent, Branch* branch) {
    if (branch) {
        pp_indent(indent);
        if (branch->cond) {
            fprintf(fout, "%sWHEN ", CYAN);
            pp_expr(branch->cond);
        } else {
            fprintf(fout, "%sELSE", CYAN);
        }
        fprintf(fout, "\n%s", RESET);
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_stmt (uint indent, Stmt* stmt) {
    if (stmt) {
        pp_indent(indent);
        fprintf(fout, "%s<%d> %s", BLACK, stmt->id, RESET);
        switch (stmt->type) {
            case S_IF:
                fprintf(fout, "%sCHOICE %s{\n", CYAN, RESET);
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                fprintf(fout, "}\n");
                break;
            case S_DO:
                fprintf(fout, "%sLOOP %s{\n", CYAN, RESET);
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                fprintf(fout, "}\n");
                break;
            case S_ASSIGN:
                pp_assign(stmt->val.assign);
                break;
            case S_BREAK:
                fprintf(fout, "%sBREAK%s\n", CYAN, RESET);
                break;
            case S_SKIP:
                fprintf(fout, "%sSKIP%s\n", CYAN, RESET);
                break;
            default:
                UNREACHABLE();
        }
        pp_stmt(indent, stmt->next);
    }
}

void pp_assign (Assign* assign) {
    fprintf(fout, "SET %s[%s]%s <- ", RED, assign->target, RESET);
    pp_expr(assign->value);
    fprintf(fout, "\n");
}

void pp_expr (Expr* expr) {
    switch (expr->type) {
        case E_VAR:
            fprintf(fout, "%s(%s%s%s)%s", GREEN, RED, expr->val.ident, GREEN, RESET);
            break;
        case E_VAL:
            fprintf(fout, "%s(%d)%s", GREEN, expr->val.digit, RESET);
            break;
        case MATCH_ANY_BINOP():
            fprintf(fout, "%s(%s%s ", GREEN, YELLOW, str_of_expr_e(expr->type));
            pp_expr(expr->val.binop->lhs);
            fprintf(fout, " ");
            pp_expr(expr->val.binop->rhs);
            fprintf(fout, "%s)%s", GREEN, RESET);
            break;
        case MATCH_ANY_MONOP():
            fprintf(fout, "%s(%s%s ", GREEN, YELLOW, str_of_expr_e(expr->type));
            pp_expr(expr->val.subexpr);
            fprintf(fout, "%s)%s", GREEN, RESET);
            break;
        default:
            UNREACHABLE();
    }
}

void pp_var (uint indent, Var* var, bool isglob) {
    if (var) {
        pp_indent(indent);
        fprintf(
            fout,
            "%s%s %s[%s]%s\n",
            PURPLE, isglob ? "GLOBAL" : "LOCAL",
            RED, var->name, RESET);
        pp_var(indent, var->next, isglob);
    }
}

void pp_check (Check* check) {
    if (check) {
        pp_indent(0);
        fprintf(fout, "%sREACH? ", PURPLE);
        pp_expr(check->cond);
        fprintf(fout, "\n");
        pp_check(check->next);
    }
}


// Internal representation
// In order to avoid duplicates, record seen positions
// (loop) indicates that a statement continuation precedes it
//      it is detected by the translator that sets the `advance` field to false
// (merge) indicates that a statement has already been displayed
//      because it is reachable from more than one point
//      it is detected thanks to the below variable
bool* explored_steps;

void pp_rprog (RProg* prog);
void pp_rproc (RProc* proc);
void pp_rstep (uint indent, RStep* step);
void pp_rguard (uint indent, RGuard* guard);
void pp_rassign (RAssign* assign);
void pp_rexpr (RExpr* expr);
void pp_rvar (uint indent, Var* var);
void pp_rcheck (RCheck* check);

void pp_repr (FILE* f, bool color, RProg* prog) {
    fout = f;
    use_color = color;
    pp_rprog(prog);
}

void pp_rprog (RProg* prog) {
    explored_steps = malloc(prog->nbstep * sizeof(bool));
    for (uint i = 0; i < prog->nbstep; i++) { explored_steps[i] = false; }
    fprintf(fout, "%s================= REPR =================%s\n", BLUE, RESET);
    for (uint i = 0; i < prog->nbglob; i++) {
        pp_rvar(0, prog->globs+i);
        fprintf(fout, "\n");
    }
    for (uint i = 0; i < prog->nbproc; i++) {
        pp_rproc(prog->procs+i);
    }
    for (uint i = 0; i < prog->nbcheck; i++) {
        pp_rcheck(prog->checks+i);
    }
    fprintf(fout, "%s========================================%s\n\n", BLUE, RESET);
    free(explored_steps);
}

void pp_rproc (RProc* proc) {
    pp_indent(0);
    fprintf(fout, "%sthread '%s' %sentrypoint [%d]%s", PURPLE, proc->name, RED, proc->entrypoint->id, RESET);
    for (uint i = 0; i < proc->nbloc; i++) {
        fprintf(fout, "\n");
        pp_rvar(1, proc->locs+i);
    }
    pp_rstep(1, proc->entrypoint);
    fprintf(fout, "\n");
    pp_indent(0);
    fprintf(fout, "%send%s\n", PURPLE, RESET);
}

void pp_rstep (uint indent, RStep* step) {
    if (explored_steps[step->id]) {
        fprintf(fout, "\n");
        pp_indent(indent);
        fprintf(fout, "%s<%d> %s(merge)%s", YELLOW, step->id, BLACK, RESET);
        return;
    }
    explored_steps[step->id] = true;
    if (step->assign) {
        fprintf(fout, "\n");
        pp_indent(indent);
        fprintf(fout, "%s<%d> %s", YELLOW, step->id, RESET);
        pp_rassign(step->assign);
        if (step->unguarded) {
            fprintf(fout, " %sthen [%d]%s", RED, step->unguarded->id, RESET);
        } else {
            fprintf(fout, " %s<END>%s", BLACK, RESET);
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                fprintf(fout, "\n");
                pp_indent(indent);
                fprintf(fout, "%s<END>%s", BLACK, RESET);
            }
        } else {
            fprintf(fout, " %s(loop)%s", BLACK, RESET);
        }
    } else if (step->nbguarded > 0) {
        fprintf(fout, "\n");
        pp_indent(indent);
        fprintf(fout, "%s<%d> %s%d guarded", YELLOW, step->id, BLACK, step->nbguarded);
        if (step->unguarded) { fprintf(fout, ", default"); }
        fprintf(fout, "%s", RESET);
        for (uint i = 0; i < step->nbguarded; i++) {
            pp_rguard(indent+1, step->guarded+i);
            if (step->advance) {
                pp_rstep(indent+2, step->guarded[i].next);
            } else {
                fprintf(fout, " %s(loop)%s", BLACK, RESET);
            }
        }
        if (step->unguarded) {
            fprintf(fout, "\n");
            pp_indent(indent+1);
            fprintf(fout, "%selse %sjump [%d]%s", PURPLE, RED, step->unguarded->id, RESET);
            pp_rstep(indent+2, step->unguarded);
        }
        fprintf(fout, "\n");
        pp_indent(indent);
        fprintf(fout, "%s</>%s", BLACK, RESET);
    } else {
        if (step->unguarded) {
            fprintf(fout, "\n");
            pp_indent(indent);
            fprintf(fout, "%s<%d> %sskip [%d]%s", YELLOW, step->id, RED, step->unguarded->id, RESET);
        } else {
            fprintf(fout, "\n");
            pp_indent(indent);
            fprintf(fout, "%s<%d> %s<END>%s", YELLOW, step->id, BLACK, RESET);
            return;
        }
        if (step->advance) {
            if (step->unguarded) {
                pp_rstep(indent, step->unguarded);
            } else {
                fprintf(fout, " %s<END>%s", BLACK, RESET);
            }
        } else {
            fprintf(fout, " %s(loop)%s", BLACK, RESET);
        }
    }
}

void pp_rguard (uint indent, RGuard* guard) {
    fprintf(fout, "\n");
    pp_indent(indent);
    fprintf(fout, "%swhen %s", PURPLE, GREEN);
    pp_rexpr(guard->cond);
    fprintf(fout, " %sjump [%d]%s", RED, guard->next->id, RESET);
}

void pp_rassign (RAssign* assign) {
    fprintf(fout, "%s{%d. %s}%s <- %s", CYAN, assign->target->id, assign->target->name, RESET, GREEN);
    pp_rexpr(assign->expr);
    fprintf(fout, "%s", RESET);
}

void pp_rexpr (RExpr* expr) {
    switch (expr->type) {
        case E_VAR:
            fprintf(fout, "%s{%d. %s}%s", CYAN, expr->val.var->id, expr->val.var->name, RESET);
            break;
        case E_VAL:
            fprintf(fout, "%s(%d)%s", GREEN, expr->val.digit, RESET);
            break;
        case MATCH_ANY_BINOP():
            fprintf(fout, "%s(%s ", GREEN, str_of_expr_e(expr->type));
            pp_rexpr(expr->val.binop->lhs);
            fprintf(fout, " ");
            pp_rexpr(expr->val.binop->rhs);
            fprintf(fout, "%s)%s", GREEN, RESET);
            break;
        case MATCH_ANY_MONOP():
            fprintf(fout, "%s(%s ", GREEN, str_of_expr_e(expr->type));
            pp_rexpr(expr->val.subexpr);
            fprintf(fout, "%s)%s", GREEN, RESET);
            break;
        default:
            UNREACHABLE();
    }
}

void pp_rvar (uint indent, Var* var) {
    pp_indent(indent);
    fprintf(fout, "%sref %s{%d. %s}%s", PURPLE, CYAN, var->id, var->name, RESET);
}

void pp_rcheck (RCheck* check) {
    pp_indent(0);
    fprintf(fout, "%sreach? %s", PURPLE, GREEN);
    pp_rexpr(check->cond);
    fprintf(fout, "%s\n", RESET);
}


// Dot-readable
// The internal representation is basically a graph,
// it seems appropriate to use Graphviz to render it.
// The text dump is not meant to be pretty, text formatting
// would distract from formatting directives for the actual
// dot output

void dot_rprog (RProg* prog);
void dot_rvar (Var* var);
void dot_rcheck (RCheck* check, uint id);
void dot_rexpr (RExpr* expr);
void dot_rproc (RProc* proc);
void dot_rassign (RAssign* assign);
void dot_rguard (uint parent_id, uint idx, RGuard* guard);
void dot_rstep (RStep* step);

void pp_dot (FILE* fdest, RProg* prog) {
    fout = fdest;
    use_color = false;
    dot_rprog(prog);
}

void make_dot (char* fname_src, RProg* prog) {
    // assemble filenames
    size_t len = strlen(fname_src);
    char* fname_dot = malloc((len+5) * sizeof(char));
    char* fname_png = malloc((len+5) * sizeof(char));
    strcpy(fname_dot, fname_src);
    strcpy(fname_png, fname_src);
    strcpy(fname_dot + len, ".dot");
    strcpy(fname_png + len, ".png");
    // dump data
    FILE* fdot = fopen(fname_dot, "w");
    pp_dot(fdot, prog);
    fclose(fdot);
    // execute dot
    printf("File saved as %s\n\n", fname_png);
    char* cmd = malloc((len*2 + 50) * sizeof(char));
    sprintf(cmd, "dot -Tpng %s -o %s", fname_dot, fname_png);
    system(cmd);
    // cleanup
    free(fname_dot);
    free(fname_png);
    free(cmd);
}

void dot_rprog (RProg* prog) {
    explored_steps = malloc(prog->nbstep * sizeof(bool));
    for (uint i = 0; i < prog->nbstep; i++) explored_steps[i] = false;
    fprintf(fout, "digraph {\n");
    fprintf(fout, "node [fontname=\"Mono\"]\n");
    fprintf(fout, "graph [fontname=\"Mono\"]\n");
    fprintf(fout, "edge [fontname=\"Mono\"]\n");
    if (prog->nbglob) { // GLOBAL VARIABLES
        for (uint i = 0; i < prog->nbglob; i++) {
            dot_rvar(prog->globs+i);
        }
        for (uint i = 1; i < prog->nbglob; i++) {
            fprintf(fout, "{ var_%d -> var_%d [style=invis] }\n",
                prog->globs[i-1].id, prog->globs[i].id);
        }
        { // phantom global variables for spacing
            fprintf(fout, "{ var_%d -> var_x%d [style=invis] }\n",
                prog->globs[prog->nbglob-1].id, prog->nbglob);
            fprintf(fout, "{ var_x%d [label=\"\" style=invis] }\n",
                prog->nbglob);
            for (uint i = prog->nbglob+1; i < 5; i++) {
                fprintf(fout, "{ var_x%d -> var_x%d [style=invis] }\n", i-1, i);
                fprintf(fout, "{ var_x%d [label=\"\" style=invis] }\n", i);
            }
        }
    }
    if (prog->nbproc) { // PROCS
        for (uint i = 0; i < prog->nbproc; i++) {
            dot_rproc(prog->procs+i);
        }
    }
    if (prog->nbcheck) { // CHECKS
        for (uint i = 0; i < prog->nbcheck; i++) {
            dot_rcheck(prog->checks+i, i);
        }
        for (uint i = 1; i < prog->nbcheck; i++) {
            fprintf(fout, "{ check_%d -> check_%d [style=invis] }\n", i-1, i);
        }
        for (uint i = prog->nbcheck; i < 5; i++) {
            // phantom checks for spacing
            fprintf(fout, "{ check_%d [style=invis label=\"\"] }\n", i);
            fprintf(fout, "{ check_%d -> check_%d [style=invis] }\n", i-1, i);
        }
    }
    fprintf(fout, "}\n");
    free(explored_steps);
}

void dot_rvar (Var* var) {
    fprintf(
        fout,
        "{ var_%d [label=\"%s\" shape=box style=\"filled\" fillcolor=orange] }\n",
        var->id, var->name);
}

void dot_rcheck (RCheck* check, uint id) {
    fprintf(fout, "{ check_%d [label=\"", id);
    dot_rexpr(check->cond);
    fprintf(fout, "\" shape=diamond style=\"filled\" fillcolor=yellow] }\n");
}

void dot_rexpr (RExpr* expr) {
    switch (expr->type) {
        case E_VAR:
            fprintf(fout, "%s", expr->val.var->name);
            break;
        case E_VAL:
            fprintf(fout, "%d", expr->val.digit);
            break;
        case MATCH_ANY_BINOP():
            fprintf(fout, "(");
            dot_rexpr(expr->val.binop->lhs);
            switch (expr->type) {
                case E_LT: fprintf(fout, "<"); break;
                case E_GT: fprintf(fout, ">"); break;
                case E_EQ: fprintf(fout, "="); break;
                case E_AND: fprintf(fout, " & "); break;
                case E_OR: fprintf(fout, " | "); break;
                case E_ADD: fprintf(fout, "+"); break;
                case E_SUB: fprintf(fout, "-"); break;
                case E_LEQ: fprintf(fout, "<="); break;
                case E_GEQ: fprintf(fout, ">="); break;
                case E_MUL: fprintf(fout, "*"); break;
                case E_MOD: fprintf(fout, "%%"); break;
                case E_DIV: fprintf(fout, "/"); break;
                case E_RANGE: fprintf(fout, ".."); break;
                default: UNREACHABLE();
            }
            dot_rexpr(expr->val.binop->rhs);
            fprintf(fout, ")");
            break;
        case MATCH_ANY_MONOP():
            switch (expr->type) {
                case E_NOT: fprintf(fout, "!"); break;
                case E_NEG: fprintf(fout, "-"); break;
                default: UNREACHABLE();
            }
            dot_rexpr(expr->val.subexpr);
            break;
        default:
            UNREACHABLE();
    }
}

void dot_rproc (RProc* proc) {
    fprintf(
        fout,
        "{ thread_%s [label=\"%s\" fontcolor=white shape=invhouse style=\"filled\" fillcolor=blue] }\n",
        proc->name, proc->name);
    for (uint i = 1; i < proc->nbloc; i++) {
        fprintf(fout, "{ var_%d -> var_%d [style=invis] }\n",
            proc->locs[i-1].id, proc->locs[i].id);
    }
    for (uint i = 0; i < proc->nbloc; i++) {
        fprintf(fout, "{ thread_%s -> ", proc->name);
        dot_rvar(proc->locs+i);
        fprintf(fout, " [constraint=false] }\n");
    }
    fprintf(fout, "thread_%s -> ", proc->name);
    dot_rstep(proc->entrypoint);
}

void dot_rstep (RStep* step) {
    if (explored_steps[step->id]) return;
    explored_steps[step->id] = true;
    if (step->assign) {
        fprintf(fout, "{ step_%d [label=\"", step->id);
        dot_rassign(step->assign);
        fprintf(fout, "\" style=\"filled\" fillcolor=lightblue] }\n");
        if (step->unguarded) {
            fprintf(fout, "step_%d -> step_%d\n", step->id, step->unguarded->id);
        }
        if (step->advance && step->unguarded) dot_rstep(step->unguarded);
    } else if (step->nbguarded > 0) {
        fprintf(
            fout,
            "{ step_%d [label=\"\" shape=triangle style=\"filled\" fillcolor=purple] }\n",
            step->id);
        for (uint i = 0; i < step->nbguarded; i++) {
            fprintf(fout, "step_%d -> guard_%d_%d\n", step->id, step->id, i);
            dot_rguard(step->id, i, step->guarded+i);
            if (step->advance) {
                dot_rstep(step->guarded[i].next);
            }
        }
        if (step->unguarded) {
            fprintf(fout, "step_%d -> step_%d\n", step->id, step->unguarded->id);
            dot_rstep(step->unguarded);
        }
    } else {
        fprintf(fout, "{ step_%d [label=\"\" shape=circle] }\n", step->id);
        if (step->unguarded) {
            fprintf(fout, "step_%d -> step_%d\n", step->id, step->unguarded->id);
        } else {
            return;
        }
        if (step->advance && step->unguarded) dot_rstep(step->unguarded);
    }
}

void dot_rassign (RAssign* assign) {
    fprintf(fout, "%s := ", assign->target->name);
    dot_rexpr(assign->expr);
}

void dot_rguard (uint parent_id, uint idx, RGuard* guard) {
    fprintf(fout, "{ guard_%d_%d [label=\"", parent_id, idx);
    dot_rexpr(guard->cond);
    fprintf(fout, "\" shape=diamond style=\"filled\" fillcolor=yellow] }\n");
    fprintf(fout, "guard_%d_%d -> step_%d\n", parent_id, idx, guard->next->id);
}

void pp_diff (RProg* prog, Diff* curr, Env env, bool isroot);

// Print reachability trace (i.e. walk back the chain of diffs)
void pp_sat (RProg* prog, Sat* sat, bool color, bool trace, bool exhaustive) {
    if (!prog->nbcheck) { printf("No checks declared\n"); return; }
    fout = stdout;
    use_color = color;
    for (uint i = 0; i < prog->nbcheck; i++) {
        printf(" %s{%d}%s ", BLUE, i+1, RESET);
        RCheck* check = prog->checks + i;
        pp_rexpr(check->cond);
        if (sat[i]) {
            printf(" is reachable\n");
            if (trace) {
                Env env = blank_env(prog);
                pp_diff(prog, sat[i], env, true);
                free(env);
            }
        } else if (exhaustive) {
            printf(" is not reachable\n");
        } else {
            printf(" has not been reached\n");
        }
    }
    printf("\n");
}

void pp_env (RProg* prog, Env env) {
    printf("  %s| %s* global %s", BLUE, BLACK, GREEN);
    for (uint i = 0; i < prog->nbglob; i++) {
        printf("[%s: %d] ", prog->globs[i].name, env[prog->globs[i].id]);
    }
    printf("%s\n", RESET);
    for (uint p = 0; p < prog->nbproc; p++) {
        RProc* proc = prog->procs + p;
        if (!proc->nbloc) continue;
        printf("  %s| %s* local %s'%s'%s: %s",
            BLUE, BLACK, PURPLE, proc->name, BLACK, GREEN);
        for (uint i = 0; i < proc->nbloc; i++) {
            printf("[%s: %d] ", proc->locs[i].name, env[proc->locs[i].id]);
        }
        printf("%s\n", RESET);
    }
}

void pp_diff (RProg* prog, Diff* curr, Env env, bool isroot) {
    if (curr->parent) {
        pp_diff(prog, curr->parent, env, false);
        if (curr->var_assign) {
            printf("%s  | ", BLUE);
            printf("%s{%d. %s}", CYAN, curr->var_assign->id, curr->var_assign->name);
            printf("%s <- %s%d%s\n", RESET, YELLOW, curr->val_assign, RESET);
            env[curr->var_assign->id] = curr->val_assign;
            pp_env(prog, env);
        } 
        if (!isroot) {
            printf("  %s| %s'%s'%s",
                BLUE, PURPLE, prog->procs[curr->pid_advance].name, BLACK);
            if (curr->new_step) {
                printf(" -> %s[%d]%s\n", RED, curr->new_step->id, RESET);
            } else {
                printf(" -> <END>\n");
            }
        } else {
            printf("\n");
        }
    } else {
        for (uint i = 0; i < prog->nbproc; i++) {
            printf("%s  | %s%s%s: %s[%d]%s\n",
                BLUE, PURPLE, prog->procs[i].name, BLACK,
                RED, prog->procs[i].entrypoint->id, RESET);    
        }
        pp_env(prog, env);
    }
}
