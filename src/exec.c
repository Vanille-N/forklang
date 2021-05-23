#include "exec.h"
#include "prelude.h"
#include "hashset.h"
#include "memreg.h"
#include <limits.h>


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
    diff->depth = parent ? parent->depth + 1 : 1;
    return diff;
}

Diff* dup_diff (Diff* src) {
    Diff* cpy = malloc(sizeof(Diff));
    register_sat(cpy);
    memcpy(cpy, src, sizeof(Diff));
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
    size_t envsize = comp->prog->nbvar * sizeof(int);
    size_t statesize = comp->prog->nbproc * sizeof(RStep);
    cpy->env = malloc(envsize);
    cpy->state = malloc(statesize);
    memcpy(cpy->env, comp->env, envsize);
    memcpy(cpy->state, comp->state, statesize);
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

// Macro concatenation for concise and extensible operator definition
#define APP_BIN_E_LT <
#define APP_BIN_E_GT >
#define APP_BIN_E_LEQ >=
#define APP_BIN_E_GEQ >=
#define APP_BIN_E_EQ ==
#define APP_BIN_E_AND &&
#define APP_BIN_E_OR ||
#define APP_BIN_E_ADD +
#define APP_BIN_E_SUB -
#define APP_BIN_E_MUL *
#define APP_BIN_E_DIV /
#define APP_BIN_E_MOD %

#define APP_MON_E_NOT !
#define APP_MON_E_NEG -

#define APPLY_BINOP(lhs, o, rhs) \
    o: return lhs APP_BIN_##o rhs

#define APPLY_MONOP(o, val) \
    o: return APP_MON_##o val

// Straightforward expression evaluation
int eval_expr (RExpr* expr, Env env) {
    fflush(stdout);
    switch (expr->type) {
        case E_VAR: return env[expr->val.var->id];
        case E_VAL: return (int)(expr->val.digit);
        case MATCH_ANY_BINOP(): {
            int lhs = eval_expr(expr->val.binop->lhs, env);
            if (lhs == INT_MIN) return INT_MIN; // divzero error bubbles up
            int rhs = eval_expr(expr->val.binop->rhs, env);
            if (rhs == INT_MIN) return INT_MIN;
            if (rhs == 0 && (expr->type == E_DIV || expr->type == E_MOD)) {
                return INT_MIN; // raise division error
            }
            switch (expr->type) {
                case APPLY_BINOP(lhs, E_LT, rhs);
                case APPLY_BINOP(lhs, E_GT, rhs);
                case APPLY_BINOP(lhs, E_EQ, rhs);
                case APPLY_BINOP(lhs, E_AND, rhs);
                case APPLY_BINOP(lhs, E_OR, rhs);
                case APPLY_BINOP(lhs, E_ADD, rhs);
                case APPLY_BINOP(lhs, E_SUB, rhs);
                case APPLY_BINOP(lhs, E_LEQ, rhs);
                case APPLY_BINOP(lhs, E_GEQ, rhs);
                case APPLY_BINOP(lhs, E_MUL, rhs);
                case APPLY_BINOP(lhs, E_DIV, rhs);
                case APPLY_BINOP(lhs, E_MOD, rhs);
                case E_RANGE:
                    if (lhs > rhs) return INT_MIN;
                    int pick = lhs + (rand() % (rhs - lhs + 1));
                    return pick;
                default: UNREACHABLE();
            }
        }
        case MATCH_ANY_MONOP(): {
            int val = eval_expr(expr->val.subexpr, env);
            switch (expr->type) {
                case APPLY_MONOP(E_NOT, val);
                case APPLY_MONOP(E_NEG, val);
                default: UNREACHABLE();
            }
        }
        default: UNREACHABLE();
    }
}

bool exec_assign (RAssign* assign, Env env, Diff* diff) {
    int val = eval_expr(assign->expr, env);
    if (val != INT_MIN) {
        env[assign->target->id] = eval_expr(assign->expr, env);
        diff->var_assign = assign->target;
        diff->val_assign = env[assign->target->id];
        return 1;
    } else {
        return 0;
    }
}

// Randomly choose a successor of a determined computation step
// and update the environment
// Returns the new state
RStep* exec_step_random (RStep* step, Env env, Diff* diff) {
    if (!step) return step; // NULL, blocked
    if (step->assign) {
        if (!exec_assign(step->assign, env, diff)) return step;
        // Blocked by null division
    }
    uint satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        int res = eval_expr(step->guarded[i].cond, env);
        if (res && res != INT_MIN) {
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
                int res = eval_expr(prog->checks[k].cond, comp.env);
                if (res == 0 || res == INT_MIN) continue;
                if (!comp.sat[k]) {
                    // found a solution
                    comp.sat[k] = comp.diff;
                } else if (comp.sat[k] && comp.diff->depth < comp.sat[k]->depth) {
                    // found a shorter solution
                    comp.sat[k] = comp.diff;
                }
            }
            // duplicate zero check, preferred to duplicating all the other code
            if (!prog->nbproc) break;

            // choose the process that will advance
            uint procid = (uint)rand() % prog->nbproc;
            // calculate next step of the computation
            Diff* old_diff = comp.diff;
            RStep* old_step = comp.state[procid];
            comp.diff = make_diff(old_diff);
            comp.diff->pid_advance = procid;
            comp.state[procid] = exec_step_random(old_step, comp.env, comp.diff);
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

// Explore (i.e. add to the worklist with their updated environment)
// all successors of a state
void exec_step_all_proc (HashSet* seen, WorkList* todo, uint pid, Compute* comp) {
    RStep* step = comp->state[pid];
    if (!step) return; // NULL, blocked
    Diff* diff = make_diff(comp->diff);
    diff->pid_advance = pid;
    if (step->assign) {
        if (!exec_assign(step->assign, comp->env, diff)) return;
        // Blocked by null division
    }
    // find all satisfied guards
    RStep* satisfied [step->nbguarded];
    uint nbsat = 0;
    for (uint i = 0; i < step->nbguarded; i++) {
        int res = eval_expr(step->guarded[i].cond, comp->env);
        if (res && res != INT_MIN) {
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
            int res = eval_expr(prog->checks[k].cond, comp->env);
            if (res == 0 || res == INT_MIN) continue;
            if (!comp->sat[k]) {
                // found a solution
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
