#include "memreg.h"

#include <stdlib.h>

const uint MBLOCK_SIZE = 100;

void register_alloc (memblock_t** registry, void* ptr) {
    if (!(*registry) || ((*registry) && (*registry)->len == MBLOCK_SIZE)) {
        memblock_t* newblock = malloc(sizeof(memblock_t));
        newblock->next = *registry;
        newblock->len = 0;
        newblock->block = malloc(MBLOCK_SIZE * sizeof(void*));
        *registry = newblock;
    }
    (*registry)->block[(*registry)->len++] = ptr;
}

void register_free (memblock_t** registry) {
    while (*registry) {
        memblock_t* tmp = *registry;
        *registry = tmp->next;
        for (uint i = 0; i < tmp->len; i++) free(tmp->block[i]);
        free(tmp->block);
        free(tmp);
    }
}

