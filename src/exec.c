#include "exec.h"

#include <stdlib.h>
#include <stdio.h>

#include "hashset.h"
#include "memreg.h"

MemBlock* sat_alloc_registry = NULL;
void register_sat (void* ptr) { register_alloc(&sat_alloc_registry, ptr); }
void free_sat () { register_free(&sat_alloc_registry); }

Env blank_env (RProg* prog) {
    Env env = malloc(prog->nbvar * sizeof(int));
    for (uint i = 0; i < prog->nbvar; i++) {
        env[i] = 0;
    }
    return env;
}

State init_state (RProg* prog) {
    State state = malloc(prog->nbproc * sizeof(RStep*));
    for (uint i = 0; i < prog->nbproc; i++) {
        state[i] = prog->procs[i].entrypoint;
    }
    return state;
}

Diff* make_diff (Diff* parent) {
    Diff* diff = malloc(sizeof(Diff));
    register_sat(diff);
    diff->parent = parent;
    diff->pid_advance = (uint)(-1);
    diff->new_step = NULL;
    diff->var_assign = NULL;
    diff->val_assign = 0;
    return diff;
}

Diff* dup_diff (Diff* src) {
    Diff* cpy = malloc(sizeof(Diff));
    register_sat(cpy);
    cpy->parent = src->parent;
    cpy->pid_advance = src->pid_advance;
    cpy->new_step = src->new_step;
    cpy->var_assign = src->var_assign;
    cpy->val_assign = src->val_assign;
    return cpy;
}

// duplicate a computation to avoid side effects
Compute* dup_compute (Compute* comp) {
    Compute* cpy = malloc(sizeof(Compute));
    // copy by reference so that sat is updated
    cpy->sat = comp->sat;
    cpy->prog = comp->prog;
    cpy->diff = comp->diff;
    // copy by value so that environment is not modified
    cpy->env = malloc(comp->prog->nbvar * sizeof(int));
    for (uint i = 0; i < comp->prog->nbvar; i++) { cpy->env[i] = comp->env[i]; }
    cpy->state = malloc(comp->prog->nbproc * sizeof(RStep));
    for (uint i = 0; i < comp->prog->nbproc; i++) { cpy->state[i] = comp->state[i]; }
    return cpy;
}

void free_compute (Compute* comp) {
    free(comp->env);
    free(comp->state);
    free(comp);
}

Sat* blank_sat (RProg* prog) {
    Sat* sat = malloc(prog->nbcheck * sizeof(Compute*));
    register_sat(sat);
    for (uint i = 0; i < prog->nbcheck; i++) {
        sat[i] = NULL;
    }
    return sat;
}

// black magic for operator expansion
#define BINOP_E_LESS <
#define BINOP_E_GREATER >
#define BINOP_E_EQUAL ==
#define BINOP_E_AND &&
#define BINOP_E_OR ||
#define BINOP_E_ADD +
#define BINOP_E_SUB -
#define MONOP_E_NOT !
#define MONOP_E_NEG -
#define BINOP(o) case o: return lhs BINOP_##o rhs
#define MONOP(o) case o: return MONOP_##o val

// straightforward expression evaluation
int eval_expr (RExpr* expr, Env env) {
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
                BINOP(E_LESS);
                BINOP(E_GREATER);
                BINOP(E_EQUAL);
                BINOP(E_AND);
                BINOP(E_OR);
                BINOP(E_ADD);
                BINOP(E_SUB);
                default: UNREACHABLE();
            }
        }
        case E_NOT:
        case E_NEG: {
            int val = eval_expr(expr->val.subexpr, env);
            switch (expr->type) {
                MONOP(E_NOT);
                MONOP(E_NEG);
                default: UNREACHABLE();
            }
        }
        default: UNREACHABLE();
    }
}

void exec_assign (RAssign* assign, Env env, Diff* diff) {
    env[assign->target->id] = eval_expr(assign->expr, env);
    diff->var_assign = assign->target;
    diff->val_assign = env[assign->target->id];
}

// Randomly choose a successor of a determined computation step
// and update the environment
// Returns the new state
RStep* exec_step_random (RStep* step, Env env, Diff* diff) {
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
Sat* exec_prog_random (RProg* prog) {
    Compute comp;
    comp.sat = blank_sat(prog);
    comp.prog = prog;
    for (uint j = 0; j < 100; j++) {
        comp.env = blank_env(prog); 
        comp.state = init_state(prog);
        comp.diff = make_diff(NULL);
        for (uint i = 0; i < 100; i++) {
            // update reachability
            // (do this _before_ simulating a step so that if a check
            // is initially valid it is counted)
            for (uint k = 0; k < prog->nbcheck; k++) {
                if (!comp.sat[k] && 0 != eval_expr(prog->checks[k].cond, comp.env)) {
                    comp.sat[k] = comp.diff;
                }
            }
            // duplicate zero check, not a big deal
            // compared to duplicating code.
            // Probably optimized anyway.
            if (!prog->nbproc) break;

            // choose the process that will advance
            uint choose_proc = (uint)rand() % prog->nbproc;
            // calculate next step of the computation
            Diff* old_diff = comp.diff;
            RStep* old_step = comp.state[choose_proc];
            comp.diff = make_diff(old_diff);
            comp.diff->pid_advance = choose_proc;
            comp.state[choose_proc] = exec_step_random(
                old_step,
                comp.env,
                comp.diff);
            if (comp.diff->new_step == old_step) {
                // process is blocked, do not record empty diff
                comp.diff = old_diff;
            }
        }
        free(comp.env);
        free(comp.state);
    }
    return comp.sat;
}

// Explore (i.e. add to the worklist with their updated environment) all successors of a state
void exec_step_all_proc (HashSet* seen, WorkList* todo, uint pid, Compute* comp) {
    RStep* step = comp->state[pid];
    if (!step) return; // NULL, blocked
    Diff* diff = make_diff(comp->diff);
    diff->pid_advance = pid;
    if (step->assign) {
        exec_assign(step->assign, comp->env, diff);
    }
    // find all satisfied guards
    RStep* satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        if (0 != eval_expr(step->guarded[i].cond, comp->env)) {
            satisfied[nbsat++] = step->guarded[i].next;
        }
    }
    RStep** successors;
    uint nbsucc;
    if (step->nbguarded == 0) {
        // unconditional advancement
        successors = &step->unguarded;
        nbsucc = 1;
    } else if (nbsat == 0) {
        if (step->unguarded) {
            // else clause
            successors = &step->unguarded;
            nbsucc = 1;
        } else {
            // blocked
            nbsucc = 0;
        }
    } else {
        // satisfied guards
        successors = satisfied;
        nbsucc = nbsat;
    }
    // enqueue all successors
    for (uint i = 0; i < nbsucc; i++) {
        comp->state[pid] = successors[i];
        // record only if not already seen
        if (try_insert(seen, comp)) {
            comp->diff = dup_diff(diff);
            comp->diff->new_step = comp->state[pid];
            enqueue(todo, comp);
        }
    }
}

Sat* exec_prog_all (RProg* prog) {
    Sat* sat = blank_sat(prog);
    // setup computation state
    Compute* comp = malloc(sizeof(Compute));
    comp->sat = sat;
    comp->prog = prog;
    comp->env = blank_env(prog);
    comp->state = init_state(prog);
    comp->diff = make_diff(NULL);
    // explored records
    HashSet* seen = create_hashset(200);
    WorkList* todo = create_worklist();
    insert(seen, comp, hash(comp));
    enqueue(todo, comp);
    free_compute(comp);
    while ((comp = dequeue(todo))) {
        // loop as long as some configurations are unexplored
        for (uint k = 0; k < prog->nbcheck; k++) {
            if (!comp->sat[k] && 0 != eval_expr(prog->checks[k].cond, comp->env)) {
                comp->sat[k] = comp->diff;
            }
        }
        // advance all processes in parallel
        for (uint k = 0; k < prog->nbproc; k++) {
            Compute* tmp = dup_compute(comp);
            exec_step_all_proc(seen, todo, k, tmp);
            free_compute(tmp);
        }
        free_compute(comp);
    }
    free_hashset(seen);
    free(todo);
    return sat;
}
