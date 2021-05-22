#include "hashset.h"

#include <stdlib.h>
#include <stdio.h>

const ull MOD = 0x10000000;
const ull MUL = 1103515245;
const ull ADD = 12345;
const ull INIT = 42;

// Hashes on more that the capacity for fewer
// structural comparisons
ull hash (compute_t* item) {
    ull h = INIT;
    for (uint i = 0; i < item->prog->nbvar; i++) {
        h = ((h + (ull)item->env[i]) * MUL + ADD) % MOD;
    }
    for (uint i = 0; i < item->prog->nbproc; i++) {
        if (item->state[i]) {
            h = ((h + item->state[i]->id) * MUL + ADD) % MOD;
        }
    }
    return h;
}

// In the rare event that two computations have the same hash
bool equals (compute_t* lhs, compute_t* rhs) {
    for (uint i = 0; i < lhs->prog->nbvar; i++) {
        if (lhs->env[i] != rhs->env[i]) return false;
    }
    for (uint i = 0; i < lhs->prog->nbproc; i++) {
        if (lhs->state[i] != rhs->state[i]) return false;
    }
    return true;
}

// Allocate set buffer and fill with NULL
hashset_t* create_hashset (uint size) {
    hashset_t* set = malloc(sizeof(hashset_t));
    set->size = size;
    set->records = malloc(size * sizeof(record_t*));
    for (uint i = 0; i < size; i++) {
        set->records[i] = NULL;
    }
#if HASHSET_SHOW_STATS
    set->collisions = 0;
    set->nb_elem = 0;
#endif // HASHSET_SHOW_STATS
    return set;
}

void free_record (record_t* rec) {
    while (rec) {
        record_t* tmp = rec;
        rec = rec->next;
        free_compute(tmp->data);
        free(tmp);
    }
}

void free_hashset (hashset_t* set) {
#if HASHSET_SHOW_STATS
    printf("Hashset deallocated\n");
    printf(" | %d elements\n", set->nb_elem);
    printf(" | %d hash collisions\n", set->collisions);
    uint maxhist = (set->nb_elem > set->size) ? set->size : set->nb_elem;
    int histogram [maxhist + 1];
    for (uint i = 0; i <= maxhist; i++) histogram[i] = 0;
    for (uint i = 0; i < set->size; i++) {
        record_t* cur = set->records[i];
        uint nb = 0;
        while (cur) { nb++; cur = cur->next; }
        histogram[nb]++;
    }
    printf(" | Histogram [size -> count]\n");
    for (uint i = 0; i <= maxhist; i++) {
        if (histogram[i]) {
            printf(" |- %d -> %d\n", i, histogram[i]);
        }
    }
#endif // HASHSET_SHOW_STATS
    for (uint i = 0; i < set->size; i++) {
        free_record(set->records[i]);
    }
    free(set->records);
    free(set);
}

// Insert regardless of presence
void insert (hashset_t* set, compute_t* item, ull hashed) {
    uint idx = (uint)(hashed % set->size);
    record_t* rec = malloc(sizeof(record_t));
    rec->data = dup_compute(item);
    rec->hash = hashed;
    rec->next = set->records[idx];
    set->records[idx] = rec;
#if HASHSET_SHOW_STATS
    set->nb_elem++;
#endif // HASHSET_SHOW_STATS
}

// Check for presence in set
bool query (hashset_t* set, compute_t* item, ull hashed) {
    uint idx = (uint)(hashed % set->size);
    record_t* rec = set->records[idx];
    while (rec) {
        if (rec->hash == hashed) {
            if (equals(rec->data, item)) {
                return true;
            } else {
#if HASHSET_SHOW_STATS
                set->collisions++;
#endif // HASHSET_SHOW_STATS
            }
        }
        rec = rec->next;
    }
    return false;
}

// Insert and return true iff absent
bool try_insert (hashset_t* set, compute_t* item) {
    ull hashed = hash(item);
    if (!query(set, item, hashed)) {
        insert(set, item, hashed);
        return true;
    } else {
        return false;
    }
}

worklist_t* create_worklist () {
    worklist_t* queue = malloc(sizeof(worklist_t));
    queue->head = NULL;
    return queue;
}

void enqueue (worklist_t* todo, compute_t* item) {
    record_t* rec = malloc(sizeof(record_t));
    rec->data = dup_compute(item);
    rec->hash = 0;
    rec->next = todo->head;
    todo->head = rec;
}

compute_t* dequeue (worklist_t* todo) {
    record_t* tmp = todo->head;
    if (tmp) {
        compute_t* res = tmp->data;
        todo->head = tmp->next;
        free(tmp);
        return res;
    }
    return NULL;
}
