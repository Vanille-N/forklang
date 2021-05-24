#include "memreg.h"
#include "prelude.h"

const uint MBLOCK_SIZE = 100;

typedef struct MemBlock {
    uint len; // capacity is fixed, len is how many are filled
    void** block; // array of pointers to free
    struct MemBlock* next; // when block is full add a new record
} MemBlock;

void register_alloc (MemBlock** registry, void* ptr) {
    if (!(*registry) || ((*registry) && (*registry)->len == MBLOCK_SIZE)) {
        // block is full, allocate a new one
        MemBlock* newblock = malloc(sizeof(MemBlock));
        newblock->next = *registry;
        newblock->len = 0;
        newblock->block = malloc(MBLOCK_SIZE * sizeof(void*));
        *registry = newblock;
    }
    (*registry)->block[(*registry)->len++] = ptr;
}

void register_free (MemBlock** registry) {
#if MEMREG_SHOW_STATS
    uint nbblocks = 0;
    uint nbcells = 0;
#endif // MEMREG_SHOW_STATS
    while (*registry) {
        MemBlock* tmp = *registry;
        *registry = tmp->next;
        for (uint i = 0; i < tmp->len; i++) free(tmp->block[i]);
#if MEMREG_SHOW_STATS
        nbblocks++;
        nbcells += tmp->len;
#endif // MEMREG_SHOW_STATS
        free(tmp->block);
        free(tmp);
    }
#if MEMREG_SHOW_STATS
    printf("-> %d memory blocks deallocated from %p\n", nbblocks, (void*)registry);
    printf("   total %d * %d + %d = %d cells\n",
        nbcells / MBLOCK_SIZE, MBLOCK_SIZE, nbcells % MBLOCK_SIZE, nbcells);
#endif // MEMREG_SHOW_STATS
}

