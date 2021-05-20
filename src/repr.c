#include "repr.h"

#include "printer.h"

// Translate from ast to repr
// Ast is fine for parsing and for display, but it is poorly suited
// for execution.
// Linked lists everywhere are unefficient, branches are not connected
// to their continuation
// The internal representation solves this issue by making each statement
// a node in a directed graph.
// Each node has an optional assignment, zero or more guarded continuations,
// zero or one unguarded continuations. The continuations are simply
// pointers to the corresponding nodes.
// The main difficulty is to link the endings of branches in if and do
// to their continuation. This is done with a skipto/breakto mechanism,
// explained in more detail below.

// Variables
uint nbglob;
uint nbloc;
var_t* globs;
var_t* locs;

rprog_t* tr_prog (prog_t* in) {
    rprog_t* out = malloc(sizeof(rprog_t));
    out->nbstep = in->nbstmt;
    out->nbvar = in->nbvar;
    // Write global variables
    out->nbglob = tr_var_list(&out->globs, in->globs);
    nbglob = out->nbglob;
    globs = out->globs;
    // Write processes
    out->nbproc = tr_proc_list(&out->procs, in->procs);
    // Write checks
    out->nbcheck = tr_check_list(&out->checks, in->checks);
    return out;
}

// List to array conversion
uint tr_var_list (var_t** loc, var_t* in) {
    {
        uint len = 0;
        var_t* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(var_t));
    }
    uint n = 0;
    var_t* cur = in;
    while (cur) {
        (*loc)[n].name = cur->name;
        (*loc)[n].id = cur->id;
        n++;
        cur = cur->next;
    }
    return n;
}

uint tr_check_list (rcheck_t** loc, check_t* in) {
    {
        uint len = 0;
        check_t* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(check_t));
    }
    // no local variables during checks
    nbloc = 0;
    locs = NULL;

    uint n = 0;
    check_t* cur = in;
    while (cur) {
        (*loc)[n].cond = tr_expr(cur->cond);
        n++;
        cur = cur->next;
    }
    return n;
}

rexpr_t* tr_expr (expr_t* in) {
    rexpr_t* out = malloc(sizeof(rexpr_t));
    out->type = in->type;
    switch (in->type) {
        case E_VAR: {
            var_t* find;
            if ((find = locate_var(in->val.ident, nbloc, locs))) {
                out->val.var = find;
            } else if ((find = locate_var(in->val.ident, nbglob, globs))) {
                out->val.var = find;
            } else {
                printf("Variable %s is not declared\n", in->val.ident);
                exit(1);
            }
            break;
        }
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

var_t* locate_var (char* ident, uint nb, var_t* vars) {
    for (uint i = 0; i < nb; i++) {
        if (0 == strcmp(ident, vars[i].name)) {
            return vars + i;
        }
    }
    return NULL;
}

uint tr_proc_list (rproc_t** loc, proc_t* in) {
    {
        uint len = 0;
        proc_t* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(rproc_t));
    }
    uint n = 0;
    proc_t* cur = in;
    while (cur) {
        rproc_t* out = (*loc) + n;
        out->name = cur->name;
        out->nbloc = tr_var_list(&out->locs, cur->locs);
        // setup local variables just for this translation
        nbloc = out->nbloc;
        locs = out->locs;
        tr_stmt(
            &out->entrypoint, cur->stmts,
            true, // advances to the end
            NULL, // nothing to skip to
            NULL); // nothing to break to
        n++;
        cur = cur->next;
    }
    return n;
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
            if ((find = locate_var(in->val.assign->target, nbloc, locs))) {
                (*out)->assign->target = find;
            } else if ((find = locate_var(in->val.assign->target, nbglob, globs))) {
                (*out)->assign->target = find;
            } else {
                printf("Variable %s is not declared\n", in->val.assign->target);
                exit(1);
            }
            (*out)->assign->expr = tr_expr(in->val.assign->value);
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

