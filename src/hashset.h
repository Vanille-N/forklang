#ifndef HASHSET_H
#define HASHSET_H

#include <stdio.h>
#include <stdbool.h>
#include "exec.h"
#include "repr.h"

typedef unsigned long long ull;

typedef struct record {
    compute_t* data;
    ull hash;
    struct record* next;
} record_t;

typedef struct {
    uint size;
    record_t** records;
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
