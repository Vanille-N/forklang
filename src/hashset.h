#ifndef HASHSET_H
#define HASHSET_H

#include "exec.h"

#define HASHSET_SHOW_STATS 0

typedef unsigned long long ull;

typedef struct record {
    compute_t* data;
    ull hash;
    struct record* next;
} record_t;

typedef struct {
    uint size;
    record_t** records;
#if HASHSET_SHOW_STATS
    uint collisions;
    uint nb_elem;
#endif // HASHSET_SHOW_STATS
} hashset_t;

ull hash (compute_t* item);
bool equals (compute_t* lhs, compute_t* rhs);

hashset_t* create_hashset (uint size);
void free_hashset (hashset_t* set);
void insert (hashset_t* set, compute_t* item, ull hashed);
bool query (hashset_t* set, compute_t* item, ull hashed);
bool try_insert (hashset_t* set, compute_t* item);

typedef struct {
    record_t* head;
} worklist_t;

worklist_t* create_worklist ();
compute_t* dequeue (worklist_t* todo);
void enqueue (worklist_t* todo, compute_t* item);

#endif // HASHSET_H
