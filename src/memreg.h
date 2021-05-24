#ifndef MEMREG_H
#define MEMREG_H

#define MEMREG_SHOW_STATS 0

// Any file/function/procedure that wishes to make many memory
// allocations without freeing them at the end of the procedure
// (i.e. parsing and the ast -> repr conversion) may use a
// MemBlock* variable to record all allocations and perform
// batch free at a later time.
typedef struct MemBlock MemBlock;

// Schedule pointer for future deletion
void register_alloc (MemBlock** registry, void* ptr);

// Perform all scheduled frees
void register_free (MemBlock** registry);

#endif // MEMREG_H
