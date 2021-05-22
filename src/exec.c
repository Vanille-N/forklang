#include "exec.h"

#include <stdlib.h>
#include <stdio.h>

#include "hashset.h"

env_t blank_env (rprog_t* prog) {
    env_t env = malloc(prog->nbvar * sizeof(int));
    for (uint i = 0; i < prog->nbvar; i++) {
        env[i] = 0;
    }
    return env;
}

state_t init_state (rprog_t* prog) {
    state_t state = malloc(prog->nbproc * sizeof(rstep_t*));
    for (uint i = 0; i < prog->nbproc; i++) {
        state[i] = prog->procs[i].entrypoint;
    }
    return state;
}

diff_t* make_diff (diff_t* parent) {
    diff_t* diff = malloc(sizeof(diff_t));
    diff->parent = parent;
    diff->pid_advance = (uint)(-1);
    diff->new_step = NULL;
    diff->var_assign = NULL;
    diff->val_assign = 0;
    return diff;
}

diff_t* dup_diff (diff_t* src) {
    diff_t* cpy = malloc(sizeof(diff_t));
    cpy->parent = src->parent;
    cpy->pid_advance = src->pid_advance;
    cpy->new_step = src->new_step;
    cpy->var_assign = src->var_assign;
    cpy->val_assign = src->val_assign;
    return cpy;
}

// duplicate a computation to avoid side effects
compute_t* dup_compute (compute_t* comp) {
    compute_t* cpy = malloc(sizeof(compute_t));
    // copy by reference so that sat is updated
    cpy->sat = comp->sat;
    cpy->prog = comp->prog;
    cpy->diff = comp->diff;
    // copy by value so that environment is not modified
    cpy->env = malloc(comp->prog->nbvar * sizeof(int));
    for (uint i = 0; i < comp->prog->nbvar; i++) { cpy->env[i] = comp->env[i]; }
    cpy->state = malloc(comp->prog->nbproc * sizeof(rstep_t));
    for (uint i = 0; i < comp->prog->nbproc; i++) { cpy->state[i] = comp->state[i]; }
    return cpy;
}

void free_compute (compute_t* comp) {
    free(comp->env);
    free(comp->state);
    free(comp);
}

sat_t* blank_sat (rprog_t* prog) {
    sat_t* sat = malloc(prog->nbcheck * sizeof(compute_t*));
    for (uint i = 0; i < prog->nbcheck; i++) {
        sat[i] = NULL;
    }
    return sat;
}

// straightforward expression evaluation
int eval_expr (rexpr_t* expr, env_t env) {
    fflush(stdout);
    switch (expr->type) {
        case E_VAR: return env[expr->val.var->id];
        case E_VAL: return (int)(expr->val.digit);
        case E_LESS:
        case E_GREATER:
        case E_EQUAL:
        case E_AND:
        case E_OR:
        case E_ADD:
        case E_SUB: {
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
        }
        case E_NOT:
        case E_NEG: {
            int val = eval_expr(expr->val.subexpr, env);
            if (expr->type == E_NOT) {
                return !val;
            } else {
                return -val;
            }
        }
        default: UNREACHABLE();
    }
}

void exec_assign (rassign_t* assign, env_t env, diff_t* diff) {
    env[assign->target->id] = eval_expr(assign->expr, env);
    diff->var_assign = assign->target;
    diff->val_assign = env[assign->target->id];
}

// Randomly choose a successor of a determined computation step
// and update the environment
// Returns the new state
rstep_t* exec_step_random (rstep_t* step, env_t env, diff_t* diff) {
    if (!step) return step; // NULL, blocked
    if (step->assign) {
        exec_assign(step->assign, env, diff);
    }
    uint satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        if (0 != eval_expr(step->guarded[i].cond, env)) {
            satisfied[nbsat++] = i;
        }
    }
    if (step->nbguarded == 0) {
        diff->new_step = step->unguarded; // unconditional advancement
    } else if (nbsat == 0) {
        if (step->unguarded) {
            diff->new_step = step->unguarded; // else clause
        } else {
            diff->new_step = step; // blocked
        }
    } else {
        int choice = rand() % (int)nbsat;
        diff->new_step = step->guarded[satisfied[choice]].next; // satisfied guard
    }
    return diff->new_step;
}

// Randomly execute a program (many times)
sat_t* exec_prog_random (rprog_t* prog) {
    compute_t comp;
    comp.sat = blank_sat(prog);
    comp.prog = prog;
    for (uint j = 0; j < 100; j++) {
        comp.env = blank_env(prog); 
        comp.state = init_state(prog);
        comp.diff = make_diff(NULL);
        //printf("RESTART\n");
        //pp_env(prog, comp.env);
        //printf("=======\n");
        for (uint i = 0; i < 100; i++) {
            // choose the process that will advance
            uint choose_proc = (uint)rand() % prog->nbproc;
            //printf("\n%d advances\n", choose_proc);
            // calculate next step of the computation
            //if (comp.state[choose_proc]) printf("  %d old state\n", comp.state[choose_proc]->id);
            //pp_env(prog, comp.env);
            comp.diff = make_diff(comp.diff);
            comp.diff->pid_advance = choose_proc;
            comp.state[choose_proc] = exec_step_random(
                comp.state[choose_proc],
                comp.env,
                comp.diff);
            //if (comp.state[choose_proc]) printf("  %d new state\n", comp.state[choose_proc]->id);
            // update reachability
            for (uint k = 0; k < prog->nbcheck; k++) {
                if (!comp.sat[k] && 0 != eval_expr(prog->checks[k].cond, comp.env)) {
                    comp.sat[k] = comp.diff;
                    printf("%d has been reached\n", k);
                }
            }
        }
        free(comp.env);
        free(comp.state);
    }
    return comp.sat;
}

// Explore (i.e. add to the worklist with their updated environment) all successors of a state
void exec_step_all_proc (hashset_t* seen, worklist_t* todo, uint pid, compute_t* comp) {
    //printf("Advance %d\n", pid);
    rstep_t* step = comp->state[pid];
    if (!step) return; // NULL, blocked
    diff_t* diff = make_diff(comp->diff);
    diff->pid_advance = pid;
    if (step->assign) {
        exec_assign(step->assign, comp->env, diff);
    }
    // find all satisfied guards
    uint satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        if (0 != eval_expr(step->guarded[i].cond, comp->env)) {
            satisfied[nbsat++] = i;
        }
    }
    if (step->nbguarded == 0) {
        // unconditional advancement
        // record if not already seen
        comp->state[pid] = step->unguarded;
        if (try_insert(seen, comp)) {
            comp->diff = dup_diff(diff);
            comp->diff->new_step = comp->state[pid];
            enqueue(todo, comp);
        }
    } else if (nbsat == 0) {
        if (step->unguarded) {
            // else clause
            comp->state[pid] = step->unguarded;
            if (try_insert(seen, comp)) {
                comp->diff = dup_diff(diff);
                comp->diff->new_step = comp->state[pid];
                enqueue(todo, comp);
            }
        } else {
            // blocked
            // state is obviously already recorded
        }
    } else {
        for (uint i = 0; i < nbsat; i++) {
            // satisfied guards
            comp->state[pid] = step->guarded[satisfied[i]].next;
            if (try_insert(seen, comp)) {
                comp->diff = dup_diff(diff);
                comp->diff->new_step = comp->state[pid];
                enqueue(todo, comp);
            }
        }
    }
}

sat_t* exec_prog_all (rprog_t* prog) {
    sat_t* sat = blank_sat(prog);
    // setup computation state
    compute_t* comp = malloc(sizeof(compute_t));
    comp->sat = sat;
    comp->prog = prog;
    comp->env = blank_env(prog);
    comp->state = init_state(prog);
    comp->diff = make_diff(NULL);
    // explored records
    hashset_t* seen = create_hashset(200);
    worklist_t* todo = create_worklist();
    insert(seen, comp, hash(comp));
    enqueue(todo, comp);
    free_compute(comp);
    //uint DBG = 0;
    while ((comp = dequeue(todo))) {
        //printf("<<%d>>\n", DBG++);
        //fflush(stdout);
        // loop as long as some configurations are unexplored
        //pp_env(prog, comp->env);
        for (uint k = 0; k < prog->nbcheck; k++) {
            if (!comp->sat[k] && 0 != eval_expr(prog->checks[k].cond, comp->env)) {
                comp->sat[k] = comp->diff;
                printf("%d has been reached\n", k);
            }
        }
        // advance all processes in parallel
        for (uint k = 0; k < prog->nbproc; k++) {
            compute_t* tmp = dup_compute(comp);
            exec_step_all_proc(seen, todo, k, tmp);
            free_compute(tmp);
        }
        free_compute(comp);
    }
    free_hashset(seen);
    free(todo);
    return sat;
}
