#include "ast.h"

// Parse-time expression builders

var_t* make_ident (char* s) {
    var_t* v = malloc(sizeof(var_t));
    v->name = s;
    v->next = NULL;
    return v;
}

prog_t* make_prog (var_t* v, proc_t* p, check_t* c) {
    prog_t* prog = malloc(sizeof(prog_t));
    prog->vars = v;
    prog->procs = p;
    prog->checks = c;
    return prog;
}

assign_t* make_assign (char* s, expr_t* e) {
    assign_t* assign = malloc(sizeof(assign_t));
    assign->target = s;
    assign->expr = e;
    return assign;
}

branch_t* make_branch (expr_t* cond, stmt_t* stmt) {
    branch_t* branch = malloc(sizeof(branch_t));
    branch->cond = cond;
    branch->stmt = stmt;
    branch->next = NULL;
    return branch;
}

stmt_u assign_as_s (assign_t* assign) {
    stmt_u val;
    val.assign = assign;
    return val;
}

stmt_u branch_as_s (branch_t* branch) {
    stmt_u val;
    val.branch = branch;
    return val;
}

stmt_u null_as_s () {
    stmt_u val;
    val._ = 0;
    return val;
}

stmt_t* make_stmt (stmt_e type, stmt_u val) {
    stmt_t* stmt = malloc(sizeof(stmt_t));
    stmt->type = type;
    stmt->val = val;
    stmt->next = NULL;
    return stmt;
}

proc_t* make_proc (char* name, var_t* vars, stmt_t* stmts) {
    proc_t* proc = malloc(sizeof(proc_t));
    proc->name = name;
    proc->vars = vars;
    proc->stmts = stmts;
    proc->next = NULL;
    return proc;
}

expr_u uint_as_e (unsigned i) {
    expr_u val;
    val.digit = i;
    return val;
}

expr_u expr_as_e (expr_t* expr) {
    expr_u val;
    val.subexpr = expr;
    return val;
}

binop_t* make_binop (expr_t* lhs, expr_t* rhs) {
    binop_t* binop = malloc(sizeof(binop_t));
    binop->lhs = lhs;
    binop->rhs = rhs;
    return binop;
}

expr_u binop_as_e (binop_t* binop) {
    expr_u val;
    val.binop = binop;
    return val;
}

expr_u str_as_e (char* ident) {
    expr_u val;
    val.ident = ident;
    return val;
}

expr_t* make_expr (expr_e type, expr_u val) {
    expr_t* expr = malloc(sizeof(expr_t));
    expr->type = type;
    expr->val = val;
    return expr;
}

check_t* make_check (expr_t* cond) {
    check_t* check = malloc(sizeof(check_t));
    check->cond = cond;
    check->next = NULL;
    return check;
}

