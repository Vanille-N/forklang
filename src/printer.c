#include "printer.h"

void pp_stmt (int, stmt_t*);

void pp_indent (int num) {
    for (int i = 0; i < num; i++) {
        printf("  ");
    }
}

void pp_var (int indent, var_t* var) {
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
        case E_ELSE: return "Else";
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
            pp_expr(expr->val.expr);
            printf(")");
            break;
        case E_ELSE:
            printf("(%s)", str_of_expr_e(expr->type));
            break;
        default:
            UNREACHABLE();
    }
}

void pp_branch (int indent, branch_t* branch) {
    if (branch) {
        pp_indent(indent);
        printf("WHEN ");
        pp_expr(branch->cond);
        printf("\n");
        pp_stmt(indent+1, branch->stmt);
        pp_branch(indent, branch->next);
    }
}

void pp_assign (int indent, assign_t* assign) {
    printf("SET [%s] <- ", assign->target);
    pp_expr(assign->expr);
    printf("\n");
}

void pp_stmt (int indent, stmt_t* stmt) {
    if (stmt) {
        switch (stmt->type) {
            case S_IF:
                pp_indent(indent);
                printf("CHOICE {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_DO:
                pp_indent(indent);
                printf("LOOP {\n");
                pp_branch(indent+1, stmt->val.branch);
                pp_indent(indent);
                printf("}\n");
                break;
            case S_ASSIGN:
                pp_indent(indent);
                pp_assign(indent, stmt->val.assign);
                break;
            case S_BREAK:
                pp_indent(indent);
                printf("BREAK\n");
                break;
            case S_SKIP:
                pp_indent(indent);
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
    pp_var(0, prog->vars);
    pp_proc(prog->procs);
    pp_check(prog->checks);
}

