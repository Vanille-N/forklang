#include "ast.h"

#include "memreg.h"
#include "stdlib.h"
#include "string.h"

MemBlock* ast_alloc_registry = NULL;
MemBlock* var_alloc_registry = NULL;

void register_ast (void* ptr) { register_alloc(&ast_alloc_registry, ptr); }
void register_var (void* ptr) { register_alloc(&var_alloc_registry, ptr); }
void free_ast () { register_free(&ast_alloc_registry); }
void free_var () { register_free(&var_alloc_registry); }

// Parse-time expression builders
// make_X : construct X from essential items
// X_as_Y : put X inside an union to make it a Y

Var* make_ident (char* name, uint id) {
    Var* var = malloc(sizeof(Var));
    register_var(var);
    var->name = strdup(name);
    register_var(var->name);
    var->next = NULL;
    var->id = id;
    return var;
}

Prog* make_prog (Var* globs, Proc* procs, Check* checks) {
    Prog* prog = malloc(sizeof(Prog));
    register_ast(prog);
    prog->globs = globs;
    prog->procs = procs;
    prog->checks = checks;
    return prog;
}

Assign* make_assign (char* target, Expr* value) {
    Assign* assign = malloc(sizeof(Assign));
    register_ast(assign);
    assign->target = target;
    assign->value = value;
    return assign;
}

Branch* make_branch (Expr* cond, Stmt* stmt) {
    Branch* branch = malloc(sizeof(Branch));
    register_ast(branch);
    branch->cond = cond;
    branch->stmt = stmt;
    branch->next = NULL;
    return branch;
}

stmt_u assign_as_s (Assign* assign) { return (stmt_u){ .assign = assign }; }
stmt_u branch_as_s (Branch* branch) { return (stmt_u){ .branch = branch }; }
stmt_u null_as_s () { return (stmt_u){ ._ = 0 }; }

Stmt* make_stmt (stmt_e type, stmt_u val, uint id) {
    Stmt* stmt = malloc(sizeof(Stmt));
    register_ast(stmt);
    stmt->type = type;
    stmt->val = val;
    stmt->next = NULL;
    stmt->id = id;
    return stmt;
}

Proc* make_proc (char* name, Var* locs, Stmt* stmts) {
    Proc* proc = malloc(sizeof(Proc));
    register_ast(proc);
    proc->name = name;
    proc->locs = locs;
    proc->stmts = stmts;
    proc->next = NULL;
    return proc;
}

expr_u uint_as_e (unsigned i) { return (expr_u){ .digit = i }; }
expr_u expr_as_e (Expr* expr) { return (expr_u){ .subexpr = expr }; }

Binop* make_binop (Expr* lhs, Expr* rhs) {
    Binop* binop = malloc(sizeof(Binop));
    register_ast(binop);
    binop->lhs = lhs;
    binop->rhs = rhs;
    return binop;
}

expr_u binop_as_e (Binop* binop) { return (expr_u){ .binop = binop }; }
expr_u str_as_e (char* ident) { return (expr_u){ .ident = ident }; }

Expr* make_expr (expr_e type, expr_u val) {
    Expr* expr = malloc(sizeof(Expr));
    register_ast(expr);
    expr->type = type;
    expr->val = val;
    return expr;
}

Check* make_check (Expr* cond) {
    Check* check = malloc(sizeof(Check));
    register_ast(check);
    check->cond = cond;
    check->next = NULL;
    return check;
}

