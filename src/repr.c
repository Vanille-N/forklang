#include "repr.h"

#include "printer.h"

// Translate from ast to repr

// variables
uint nbglob;
uint nbloc;
var_t* globs;
var_t* locs;

rprog_t* tr_prog (prog_t* in) {
    rprog_t* out = malloc(sizeof(rprog_t));
    out->nbvar = 0;
    tr_var_list(&out->nbvar, &out->vars, in->vars);
    nbglob = out->nbvar;
    globs = out->vars;
    out->nbproc = 0;
    tr_proc_list(&out->nbproc, &out->procs, in->procs);
    out->nbcheck = 0;
    tr_check_list(&out->nbcheck, &out->checks, in->checks);
    out->nbstep = in->nbstmt;
    return out;
}

void tr_var_list (uint* nb, var_t** loc, var_t* in) {
    if (in) {
        uint n = (*nb)++;
        tr_var_list(nb, loc, in->next);
        (*loc)[n].name = in->name;
        (*loc)[n].id = in->id;
    } else {
        *loc = malloc(*nb * sizeof(var_t));
    }
}

void tr_check_list (uint* nb, rcheck_t** loc, check_t* in) {
    if (in) {
        uint n = (*nb)++;
        tr_check_list(nb, loc, in->next);
        nbloc = 0;
        locs = NULL;
        (*loc)[n].cond = tr_expr(in->cond);
    } else {
        *loc = malloc(*nb * sizeof(rcheck_t));
    }
}

rexpr_t* tr_expr (expr_t* in) {
    rexpr_t* out = malloc(sizeof(rexpr_t));
    out->type = in->type;
    switch (in->type) {
        case E_VAR:
            var_t* find;
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
            out->val.binop->lhs = tr_expr(in->val.binop->lhs);
            out->val.binop->rhs = tr_expr(in->val.binop->rhs);
            break;
        case E_NOT:
        case E_NEG:
            out->val.subexpr = tr_expr(in->val.subexpr);
            break;
        default: UNREACHABLE();
    }
    return out;
}

var_t* locate_var (char* ident, uint nb, var_t* locs) {
    for (uint i = 0; i < nb; i++) {
        if (0 == strcmp(ident, locs[i].name)) {
            return locs+i;
        }
    }
    return NULL;
}

void tr_proc_list (uint* nb, rproc_t** loc, proc_t* in) {
    if (in) {
        uint n = (*nb)++;
        tr_proc_list(nb, loc, in->next);
        rproc_t* curr = &(*loc)[n];
        curr->name = in->name;
        curr->nbvar = 0;
        tr_var_list(&(curr->nbvar), &(curr->vars), in->vars);
        nbloc = curr->nbvar;
        locs = curr->vars;
        tr_stmt(
            &curr->entrypoint, in->stmts,
            true, NULL, NULL);
    } else {
        *loc = malloc(*nb * sizeof(rproc_t));
    }
}

void tr_stmt (
    rstep_t** out, stmt_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto
) {
    *out = malloc(sizeof(rstep_t));
    (*out)->assign = NULL;
    (*out)->id = in->id;
    switch (in->type) {
        case S_ASSIGN:
            (*out)->assign = malloc(sizeof(rassign_t));
            var_t* find;
            if (find = locate_var(in->val.assign->target, nbloc, locs)) {
                (*out)->assign->target = find;
            } else if (find = locate_var(in->val.assign->target, nbglob, globs)) {
                (*out)->assign->target = find;
            } else {
                printf("Variable %s is not declared\n", in->val.assign->target);
                exit(1);
            }
            (*out)->assign->expr = tr_expr(in->val.assign->expr);
            // fallthrough
        case S_SKIP:
        case S_BREAK:
            (*out)->nbguarded = 0;
            (*out)->guarded = NULL;
            if (in->next) {
                (*out)->unguarded = malloc(sizeof(rstep_t));
                tr_stmt(
                    &((*out)->unguarded), in->next,
                    advance, skipto, breakto);
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
                    advance, skipto, breakto);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, *out, next);
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, *out, skipto);
            }
            break;
        case S_IF:
            (*out)->nbguarded = 0;
            (*out)->advance = true;
            if (in->next) {
                rstep_t* next = malloc(sizeof(rstep_t));
                tr_stmt(
                    &next, in->next,
                    advance, skipto, breakto);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    true, next, breakto);
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    advance, skipto, breakto);
            }   
            break;
        default: UNREACHABLE();
    }
}

rstep_t* tr_branch_list (
    uint* nb, rguard_t** loc, branch_t* in,
    bool advance, rstep_t* skipto, rstep_t* breakto
) {
    if (in) {
        if (in->cond) {
            uint n = (*nb)++;
            rstep_t* end = tr_branch_list(
                nb, loc, in->next,
                advance, skipto, breakto);
            (*loc)[n].cond = tr_expr(in->cond);
            tr_stmt(
                &((*loc)[n].next), in->stmt,
                advance, skipto, breakto);
            return end;
        } else {
            uint n = *nb;
            *loc = malloc(*nb * sizeof(rguard_t));
            rstep_t* end = malloc(sizeof(rstep_t));
            tr_stmt(
                &end, in->stmt,
                advance, skipto, breakto);
            return end;
        }
    } else {
        *loc = malloc(*nb * sizeof(rguard_t));
        return NULL;
    }
}

