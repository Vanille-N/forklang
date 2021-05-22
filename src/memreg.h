#ifndef MEMREG_H
#define MEMREG_H

typedef unsigned uint;

// Any file/function/procedure that wishes to make many memory
// allocations without freeing them at the end of the procedure
// (i.e. parsing and the ast -> repr conversion) may use a
// memblock_t* variable to record all allocations and perform
// batch free at a later time.

typedef struct memblock {
    uint len; // capacity is fixed, len is how many are filled
    void** block; // array of pointers to free
    struct memblock* next; // when block is full add a new record
} memblock_t;

// Schedule pointer for future deletion
void register_alloc (memblock_t** registry, void* ptr);

// Perform all scheduled frees
void register_free (memblock_t** registry);

#endif // MEMREG_H
