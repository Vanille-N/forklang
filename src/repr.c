#include "repr.h"

#include "printer.h"


#define UNREACHABLE() { \
    fflush(stdout); \
    fprintf(stderr, "\n\nFatal: Entered unreachable code in file %s at %s:%d", \
        __FILE__, __func__, __LINE__); \
    exit(1); \
}

// Translate from ast to repr

rprog_t* tr_prog (prog_t* in) {
    rprog_t* out = malloc(sizeof(rprog_t));
    out->nbvar = 0;
    tr_var_list(&out->nbvar, &out->vars, in->vars);
    out->nbproc = 0;
    tr_proc_list(&out->nbproc, &out->procs, in->procs, out->nbvar, out->vars);
    out->nbcheck = 0;
    tr_check_list(&out->nbcheck, &out->checks, in->checks, out->nbvar, out->vars);
    return out;
}

void tr_var_list (uint* nb, rvar_t** loc, var_t* in) {
    if (in) {
        uint n = (*nb)++;
        tr_var_list(nb, loc, in->next);
        (*loc)[n].name = in->name;
        (*loc)[n].value = 0;
    } else {
        *loc = malloc(*nb * sizeof(rvar_t));
    }
}

void tr_check_list (uint* nb, rcheck_t** loc, check_t* in, uint nbvar, rvar_t* vars) {
    if (in) {
        uint n = (*nb)++;
        tr_check_list(nb, loc, in->next, nbvar, vars);
        (*loc)[n].cond = tr_expr(in->cond, nbvar, vars, 0, NULL);
    } else {
        *loc = malloc(*nb * sizeof(rcheck_t));
    }
}

rexpr_t* tr_expr (expr_t* in, uint nbglob, rvar_t* globs, uint nbloc, rvar_t* locs) {
    rexpr_t* out = malloc(sizeof(rexpr_t));
    out->type = in->type;
    switch (in->type) {
        case E_VAR:
            rvar_t* find;
            if (find = locate_var(in->val.ident, nbloc, locs)) {
                out->val.var = find;
            } else if (find = locate_var(in->val.ident, nbglob, globs)) {
                out->val.var = find;
            } else {
                printf("Variable %s is not declared\n", in->val.ident);
                exit(1);
            }
            break;
        case E_VAL:
            out->val.digit = in->val.digit;
            break;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            out->val.binop = malloc(sizeof(rbinop_t));
            out->val.binop->lhs = tr_expr(in->val.binop->lhs, nbglob, globs, nbloc, locs);
            out->val.binop->rhs = tr_expr(in->val.binop->rhs, nbglob, globs, nbloc, locs);
            break;
        case E_NOT:
        case E_NEG:
            out->val.subexpr = tr_expr(in->val.subexpr, nbglob, globs, nbloc, locs);
            break;
        default: UNREACHABLE();
    }
    return out;
}

rvar_t* locate_var (char* ident, uint nb, rvar_t* locs) {
    for (uint i = 0; i < nb; i++) {
        if (0 == strcmp(ident, locs[i].name)) {
            return locs+i;
        }
    }
    return NULL;
}

void tr_proc_list (uint* nb, rproc_t** loc, proc_t* in, uint nbglob, rvar_t* globs) {
    if (in) {
        uint n = (*nb)++;
        tr_proc_list(nb, loc, in->next, nbglob, globs);
        rproc_t* curr = &(*loc)[n];
        curr->name = in->name;
        curr->nbvar = 0;
        tr_var_list(&(curr->nbvar), &(curr->vars), in->vars);
        tr_stmt(
            &curr->entrypoint, in->stmts,
            true, NULL, NULL,
            nbglob, globs, curr->nbvar, curr->vars);
    } else {
        *loc = malloc(*nb * sizeof(rproc_t));
    }
}

void tr_stmt (
    rstep_t** out, stmt_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto,
    uint nbglob, rvar_t* globs, uint nbloc, rvar_t* locs
) {
    *out = malloc(sizeof(rstep_t));
    (*out)->assign = NULL;
    switch (in->type) {
        case S_ASSIGN:
            (*out)->assign = malloc(sizeof(rassign_t));
            rvar_t* find;
            if (find = locate_var(in->val.assign->target, nbloc, locs)) {
                (*out)->assign->target = find;
            } else if (find = locate_var(in->val.assign->target, nbglob, globs)) {
                (*out)->assign->target = find;
            } else {
                printf("Variable %s is not declared\n", in->val.assign->target);
                exit(1);
            }
            (*out)->assign->expr = tr_expr(in->val.assign->expr, nbloc, locs, nbglob, globs);
            // fallthrough
        case S_SKIP:
        case S_BREAK:
            (*out)->nbguarded = 0;
            (*out)->guarded = NULL;
            if (in->next) {
                (*out)->unguarded = malloc(sizeof(rstep_t));
                tr_stmt(
                    &((*out)->unguarded), in->next,
                    advance, skipto, breakto,
                    nbglob, globs, nbloc, locs);
                (*out)->advance = true;
            } else if (in->type == S_BREAK) {
                (*out)->unguarded = breakto;
                (*out)->advance = true;
            } else {
                (*out)->unguarded = skipto;
                (*out)->advance = advance;
            }
            break;
        case S_DO:
            (*out)->nbguarded = 0;
            (*out)->advance = true;
            if (in->next) {
                rstep_t* next = malloc(sizeof(rstep_t));
                tr_stmt(
                    &next, in->next,
                    advance, skipto, breakto,
                    nbglob, globs, nbloc, locs);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, *out, next,
                    nbglob, globs, nbloc, locs);
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, *out, skipto,
                    nbglob, globs, nbloc, locs);
            }
            break;
        case S_IF:
            (*out)->nbguarded = 0;
            (*out)->advance = true;
            if (in->next) {
                rstep_t* next = malloc(sizeof(rstep_t));
                tr_stmt(
                    &next, in->next,
                    advance, skipto, breakto,
                    nbglob, globs, nbloc, locs);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    true, next, breakto,
                    nbglob, globs, nbloc, locs);
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    advance, skipto, breakto,
                    nbglob, globs, nbloc, locs);
            }   
            break;
        default: UNREACHABLE();
    }
}

rstep_t* tr_branch_list (
    uint* nb, rguard_t** loc, branch_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto,
    uint nbglob, rvar_t* globs, uint nbloc, rvar_t* locs
) {
    if (in) {
        if (in->cond) {
            uint n = (*nb)++;
            rstep_t* end = tr_branch_list(
                nb, loc, in->next,
                advance, skipto, breakto,
                nbglob, globs, nbloc, locs);
            (*loc)[n].cond = tr_expr(in->cond, nbglob, globs, nbloc, locs);
            tr_stmt(
                &((*loc)[n].next), in->stmt,
                advance, skipto, breakto,
                nbglob, globs, nbloc, locs);
            return end;
        } else {
            uint n = *nb;
            *loc = malloc(*nb * sizeof(rguard_t));
            rstep_t* end = malloc(sizeof(rstep_t));
            tr_stmt(
                &end, in->stmt,
                advance, skipto, breakto,
                nbglob, globs, nbloc, locs);
            return end;
        }
    } else {
        *loc = malloc(*nb * sizeof(rguard_t));
        return NULL;
    }
}

