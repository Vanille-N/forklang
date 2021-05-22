#include "repr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memreg.h"

MemBlock* repr_alloc_registry = NULL;
void register_repr (void* ptr) { register_alloc(&repr_alloc_registry, ptr); }
void free_repr () { register_free(&repr_alloc_registry); }

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

uint nbglob;
uint nbloc;
Var* globs;
Var* locs;
char* procname;

RProg* tr_prog (Prog* in) {
    RProg* out = malloc(sizeof(RProg));
    register_repr(out);
    out->nbstep = in->nbstmt;
    out->nbvar = in->nbvar;
    // Write global variables
    out->nbglob = tr_var_list(&out->globs, in->globs);
    nbglob = out->nbglob;
    globs = out->globs;
    out->nbproc = tr_proc_list(&out->procs, in->procs);
    out->nbcheck = tr_check_list(&out->checks, in->checks);
    return out;
}

// List to array conversion
uint tr_var_list (Var** loc, Var* in) {
    {
        uint len = 0;
        Var* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(Var));
        register_repr(*loc);
    }
    uint n = 0;
    Var* cur = in;
    while (cur) {
        (*loc)[n].name = cur->name;
        (*loc)[n].id = cur->id;
        n++;
        cur = cur->next;
    }
    return n;
}

uint tr_check_list (RCheck** loc, Check* in) {
    {
        uint len = 0;
        Check* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(Check));
        register_repr(*loc);
    }
    // no local variables during checks
    nbloc = 0;
    locs = NULL;
    procname = "reachability checks";

    uint n = 0;
    Check* cur = in;
    while (cur) {
        (*loc)[n].cond = tr_expr(cur->cond);
        n++;
        cur = cur->next;
    }
    return n;
}

RExpr* tr_expr (Expr* in) {
    RExpr* out = malloc(sizeof(RExpr));
    register_repr(out);
    out->type = in->type;
    switch (in->type) {
        case E_VAR:
            out->val.var = locate_var(in->val.ident);
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
            out->val.binop = malloc(sizeof(RBinop));
            register_repr(out->val.binop);
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

Var* locate_var (char* ident) {
    for (uint i = 0; i < nbloc; i++) {
        if (0 == strcmp(ident, locs[i].name)) {
            return locs + i;
        }
    }
    for (uint i = 0; i < nbglob; i++) {
        if (0 == strcmp(ident, globs[i].name)) {
            return globs + i;
        }
    }
    printf("In %s\n", procname);
    printf("Variable %s is not declared\n", ident);
    exit(1);
}

uint tr_proc_list (RProc** loc, Proc* in) {
    {
        uint len = 0;
        Proc* cur = in;
        while (cur) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(RProc));
        register_repr(*loc);
    }
    uint n = 0;
    Proc* cur = in;
    while (cur) {
        RProc* out = (*loc) + n;
        out->name = cur->name;
        out->nbloc = tr_var_list(&out->locs, cur->locs);
        // setup local variables just for this translation
        nbloc = out->nbloc;
        locs = out->locs;
        procname = out->name;
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

RAssign* tr_assign (Assign* in) {
    RAssign* out = malloc(sizeof(RAssign));
    register_repr(out);
    out->target = locate_var(in->target);
    out->expr = tr_expr(in->value);
    return out;
}

void tr_stmt (
    RStep** out, Stmt* in,
    bool advance, RStep* skipto, RStep* breakto
) {
    *out = malloc(sizeof(RStep));
    register_repr(*out);
    (*out)->assign = NULL;
    (*out)->id = in->id;
    switch (in->type) {
        case S_ASSIGN:
            (*out)->assign = tr_assign(in->val.assign);
            // fallthrough
        case S_SKIP:
        case S_BREAK:
            (*out)->nbguarded = 0;
            (*out)->guarded = NULL;
            if (in->next) {
                (*out)->unguarded = malloc(sizeof(RStep));
                register_repr((*out)->unguarded);
                tr_stmt(
                    &((*out)->unguarded), in->next,
                    advance, skipto, breakto); // normal transfer
                (*out)->advance = true; // advances to `next`
            } else if (in->type == S_BREAK) {
                // by definition the successor of a S_BREAK is `breakto`
                (*out)->unguarded = breakto;
                (*out)->advance = true; // break is always a progress
            } else {
                // similarly the successor of a S_SKIP is `skipto`
                (*out)->unguarded = skipto;
                (*out)->advance = advance;
            }
            break;
        case S_DO:
            (*out)->nbguarded = 0;
            (*out)->advance = true;
            if (in->next) {
                RStep* next = malloc(sizeof(RStep));
                register_repr(next);
                tr_stmt(
                    &next, in->next,
                    advance, skipto, breakto);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, // when inside a `do`, skipping does not necessarily advance
                    // the computation
                    *out, // skip to self
                    next); // break to successor
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    false, // same as above
                    *out, // skip to self
                    skipto); // break to parent's successor
            }
            break;
        case S_IF:
            (*out)->nbguarded = 0;
            (*out)->advance = true;
            if (in->next) {
                RStep* next = malloc(sizeof(RStep));
                register_repr(next);
                tr_stmt(
                    &next, in->next,
                    advance, skipto, breakto);
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    true, // skip is a progress since it can exit the `if`
                    next, // continue to successor
                    breakto); // still break out of last loop
            } else {
                (*out)->unguarded = tr_branch_list(
                    &((*out)->nbguarded), &((*out)->guarded), in->val.branch,
                    advance, // propagate, branches inside an `if`
                    // make the same progress as the `if` itself
                    skipto,
                    breakto);
            }   
            break;
        default: UNREACHABLE();
    }
}

// Returns the possible else clause so that
// the caller can set it as its unguarded branch
RStep* tr_branch_list (
    uint* nb, RGuard** loc, Branch* in,
    bool advance, RStep* skipto, RStep* breakto
) {
    {
        uint len = 0;
        Branch* cur = in;
        while (cur && cur->cond) { len++; cur = cur->next; }
        *loc = malloc(len * sizeof(RGuard));
        register_repr(*loc);
        *nb = len;
    }
    uint n = 0;
    Branch* cur = in;
    while (cur && cur->cond) {
        RGuard* out = *loc + n;
        out->cond = tr_expr(cur->cond);
        tr_stmt(
            &out->next, cur->stmt,
            advance, skipto, breakto);
        n++;
        cur = cur->next;
    }
    // else clause
    if (cur) {
        RStep* end = malloc(sizeof(RStep));
        register_repr(end);
        tr_stmt(
            &end, cur->stmt,
            advance, skipto, breakto);
        return end;
    } else {
        return NULL;
    }
}

