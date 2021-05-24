#include "hashset.h"
#include "prelude.h"

const ull MOD = 0x10000000;
const ull MUL = 1103515245;
const ull ADD = 12345;
const ull INIT = 42;

typedef struct Record {
    Compute* data;
    ull hash;
    struct Record* next;
} Record;

struct HashSet {
    uint size;
    Record** records;
#if HASHSET_SHOW_STATS
    uint collisions;
    uint nb_elem;
#endif // HASHSET_SHOW_STATS
};

// Queue : new elements at the end
// -> guarantees shortest path is found
struct WorkList {
    Record* head;
    Record* tail;
};

// Hashes on more that the capacity for fewer
// structural comparisons
ull hash (Compute* item) {
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
bool equals (Compute* lhs, Compute* rhs) {
    return memcmp(lhs->env, rhs->env, lhs->prog->nbvar) == 0
        && memcmp(lhs->state, rhs->state, lhs->prog->nbproc) == 0;
}

// Allocate set buffer and fill with NULL
HashSet* create_hashset (uint size) {
    HashSet* set = malloc(sizeof(HashSet));
    set->size = size;
    set->records = malloc(size * sizeof(Record*));
    for (uint i = 0; i < size; i++) {
        set->records[i] = NULL;
    }
#if HASHSET_SHOW_STATS
    set->collisions = 0;
    set->nb_elem = 0;
#endif // HASHSET_SHOW_STATS
    return set;
}

void free_record (Record* rec) {
    while (rec) {
        Record* tmp = rec;
        rec = rec->next;
        free_compute(tmp->data);
        free(tmp);
    }
}

void free_hashset (HashSet* set) {
#if HASHSET_SHOW_STATS
    printf("Hashset deallocated\n");
    printf(" | %d elements\n", set->nb_elem);
    printf(" | %d hash collisions\n", set->collisions);
    uint maxhist = (set->nb_elem > set->size) ? set->size : set->nb_elem;
    int histogram [maxhist + 1];
    memset(histogram, 0, sizeof(histogram));
    for (uint i = 0; i < set->size; i++) {
        Record* cur = set->records[i];
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
void insert (HashSet* set, Compute* item, ull hashed) {
    uint idx = (uint)(hashed % set->size);
    Record* rec = malloc(sizeof(Record));
    rec->data = dup_compute(item);
    rec->hash = hashed;
    rec->next = set->records[idx];
    set->records[idx] = rec;
#if HASHSET_SHOW_STATS
    set->nb_elem++;
#endif // HASHSET_SHOW_STATS
}

// Check for presence in set
bool query (HashSet* set, Compute* item, ull hashed) {
    uint idx = (uint)(hashed % set->size);
    Record* rec = set->records[idx];
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
bool try_insert (HashSet* set, Compute* item) {
    ull hashed = hash(item);
    if (!query(set, item, hashed)) {
        insert(set, item, hashed);
        return true;
    } else {
        return false;
    }
}

WorkList* create_worklist () {
    WorkList* queue = malloc(sizeof(WorkList));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void enqueue (WorkList* todo, Compute* item) {
    Record* rec = malloc(sizeof(Record));
    rec->data = dup_compute(item);
    rec->hash = 0;
    rec->next = NULL;
    if (todo->head) {
        todo->tail->next = rec;
        todo->tail = rec;
    } else {
        todo->head = rec;
        todo->tail = rec;
    }
}

Compute* dequeue (WorkList* todo) {
    Record* tmp = todo->head;
    if (tmp) {
        Compute* res = tmp->data;
        todo->head = tmp->next;
        free(tmp);
        return res;
    }
    return NULL;
}
