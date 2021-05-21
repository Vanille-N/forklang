#ifndef MEMREG_H
#define MEMREG_H

#include <stdlib.h>

typedef unsigned uint;

typedef struct memblock {
    uint len;
    void** block;
    struct memblock* next;
} memblock_t;

void register_alloc (memblock_t** registry, void* ptr);
void register_free (memblock_t** registry);

#endif // MEMREG_H
