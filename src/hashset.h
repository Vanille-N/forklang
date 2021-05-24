#ifndef HASHSET_H
#define HASHSET_H

#include "exec.h"
#include "prelude.h"

// Print information about hash collisions
// and hashset histogram
#define HASHSET_SHOW_STATS 0

typedef unsigned long long ull;

typedef struct HashSet HashSet;
typedef struct WorkList WorkList;

ull hash (Compute* item);
bool equals (Compute* lhs, Compute* rhs);

HashSet* create_hashset (uint size);
void free_hashset (HashSet* set);
void insert (HashSet* set, Compute* item, ull hashed);
bool query (HashSet* set, Compute* item, ull hashed);
bool try_insert (HashSet* set, Compute* item);

WorkList* create_worklist ();
Compute* dequeue (WorkList* todo);
void enqueue (WorkList* todo, Compute* item);

#endif // HASHSET_H
