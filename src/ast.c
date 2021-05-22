#include "ast.h"

#include "memreg.h"
#include "stdlib.h"
#include "string.h"

memblock_t* ast_alloc_registry = NULL;
memblock_t* var_alloc_registry = NULL;

void register_ast (void* ptr) { register_alloc(&ast_alloc_registry, ptr); }
void register_var (void* ptr) { register_alloc(&var_alloc_registry, ptr); }
void free_ast () { register_free(&ast_alloc_registry); }
void free_var () { register_free(&var_alloc_registry); }

// Parse-time expression builders
// make_X : construct X from essential items
// X_as_Y : put X inside an union to make it a Y

var_t* make_ident (char* name, uint id) {
    var_t* var = malloc(sizeof(var_t));
    register_var(var);
    var->name = strdup(name);
    register_var(var->name);
    var->next = NULL;
    var->id = id;
    return var;
}

prog_t* make_prog (var_t* globs, proc_t* procs, check_t* checks) {
    prog_t* prog = malloc(sizeof(prog_t));
    register_ast(prog);
    prog->globs = globs;
    prog->procs = procs;
    prog->checks = checks;
    return prog;
}

assign_t* make_assign (char* target, expr_t* value) {
    assign_t* assign = malloc(sizeof(assign_t));
    register_ast(assign);
    assign->target = target;
    assign->value = value;
    return assign;
}

branch_t* make_branch (expr_t* cond, stmt_t* stmt) {
    branch_t* branch = malloc(sizeof(branch_t));
    register_ast(branch);
    branch->cond = cond;
    branch->stmt = stmt;
    branch->next = NULL;
    return branch;
}

stmt_u assign_as_s (assign_t* assign) { return (stmt_u){ .assign = assign }; }
stmt_u branch_as_s (branch_t* branch) { return (stmt_u){ .branch = branch }; }
stmt_u null_as_s () { return (stmt_u){ ._ = 0 }; }

stmt_t* make_stmt (stmt_e type, stmt_u val, uint id) {
    stmt_t* stmt = malloc(sizeof(stmt_t));
    register_ast(stmt);
    stmt->type = type;
    stmt->val = val;
    stmt->next = NULL;
    stmt->id = id;
    return stmt;
}

proc_t* make_proc (char* name, var_t* locs, stmt_t* stmts) {
    proc_t* proc = malloc(sizeof(proc_t));
    register_ast(proc);
    proc->name = name;
    proc->locs = locs;
    proc->stmts = stmts;
    proc->next = NULL;
    return proc;
}

expr_u uint_as_e (unsigned i) { return (expr_u){ .digit = i }; }
expr_u expr_as_e (expr_t* expr) { return (expr_u){ .subexpr = expr }; }

binop_t* make_binop (expr_t* lhs, expr_t* rhs) {
    binop_t* binop = malloc(sizeof(binop_t));
    register_ast(binop);
    binop->lhs = lhs;
    binop->rhs = rhs;
    return binop;
}

expr_u binop_as_e (binop_t* binop) { return (expr_u){ .binop = binop }; }
expr_u str_as_e (char* ident) { return (expr_u){ .ident = ident }; }

expr_t* make_expr (expr_e type, expr_u val) {
    expr_t* expr = malloc(sizeof(expr_t));
    register_ast(expr);
    expr->type = type;
    expr->val = val;
    return expr;
}

check_t* make_check (expr_t* cond) {
    check_t* check = malloc(sizeof(check_t));
    register_ast(check);
    check->cond = cond;
    check->next = NULL;
    return check;
}

