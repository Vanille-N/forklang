#include "exec.h"


typedef int* env_t;
typedef rstep_t** state_t;

env_t blank_env (uint nbvar) {
    env_t env = malloc(nbvar * sizeof(int));
    for (uint i = 0; i < nbvar; i++) {
        env[i] = 0;
    }
    return env;
}

state_t init_state (uint nbproc, rproc_t* procs) {
    state_t state = malloc(nbproc * sizeof(rproc_t*));
    for (uint i = 0; i < nbproc; i++) {
        state[i] = procs[i].entrypoint;
    }
    return state;
}

sat_t blank_sat (uint nbcheck) {
    sat_t sat = malloc(nbcheck * sizeof(bool));
    for (uint i = 0; i < nbcheck; i++) {
        sat[i] = false;
    }
    return sat;
}

void pp_env (uint nbvar, env_t env, var_t* vars) {
    for (uint i = 0; i < nbvar; i++) {
        printf("%s=%d ", vars[i].name, env[i]);
    }
    printf("\n");
}

int eval_expr (rexpr_t* expr, env_t env) {
    fflush(stdout);
    switch (expr->type) {
        case E_VAR: return env[expr->val.var->id];
        case E_VAL: return expr->val.digit;
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB:
            int lhs = eval_expr(expr->val.binop->lhs, env);
            int rhs = eval_expr(expr->val.binop->rhs, env);
            switch (expr->type) {
                case E_LESS: return lhs < rhs;
                case E_GREATER: return lhs > rhs;
                case E_EQUAL: return lhs == rhs;
                case E_AND: return lhs && rhs;
                case E_OR: return lhs || rhs;
                case E_ADD: return lhs + rhs;
                case E_SUB: return lhs - rhs;
                default: UNREACHABLE();
            }
        case E_NOT:
        case E_NEG:
            int val = eval_expr(expr->val.subexpr, env);
            if (expr->type == E_NOT) {
                return !val;
            } else {
                return -val;
            }
        default: UNREACHABLE();
    }
}

void exec_assign (rassign_t* assign, env_t env) {
    env[assign->target->id] = eval_expr(assign->expr, env);
}

rstep_t* exec_step (rstep_t* step, env_t env) {
    if (!step) return step; // NULL, blocked
    if (step->assign) {
        exec_assign(step->assign, env);
    }
    uint satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        if (0 != eval_expr(step->guarded[i].cond, env)) {
            satisfied[nbsat++] = i;
        }
    }
    if (step->nbguarded == 0) {
        return step->unguarded; // unconditional advancement
    } else if (nbsat == 0) {
        if (step->unguarded) {
            return step->unguarded; // else clause
        } else {
            return step; // blocked
        }
    } else {
        int choice = rand() % nbsat;
        return step->guarded[satisfied[choice]].next; // satisfied guard
    }
}

sat_t exec_prog (rprog_t* prog) {
    sat_t sat = blank_sat(prog->nbcheck);
    for (uint j = 0; j < 100; j++) {
        env_t env = blank_env(prog->nbvar); 
        state_t state = init_state(prog->nbproc, prog->procs);
        for (uint i = 0; i < 100; i++) {
            uint choose_proc = rand() % prog->nbproc;
            state[choose_proc] = exec_step(state[choose_proc], env);
            for (uint k = 0; k < prog->nbcheck; k++) {
                if (!sat[k] && 0 != eval_expr(prog->checks[k].cond, env)) {
                    sat[k] = true;
                    printf("%d has been reached\n", k);
                }
            }
        }
        free(env);
        free(state);
    }
    return sat;
}
