#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "lib.h"
#include "mmu.h"

#define MEM_MAGIC 0xA5

typedef struct buddy_block {
    uint32_t size;
    bool free;
} buddy_block_t;

struct {
    buddy_block_t* start;
    buddy_block_t* end;
    uint32_t alignment;
} buddy_allocator;

void* kmalloc(void);
void kfree(void*);
void buddy_init(void* mem, uint32_t size, uint32_t alignment);
buddy_block_t* buddy_search(uint32_t size);
buddy_block_t* buddy_split(buddy_block_t* b, uint32_t size);
void buddy_coalesce(void);
void* buddy_malloc(uint32_t size);
int32_t buddy_free(void* mem);

// DEBUG
void buddy_traverse(void);
void buddy_print_block(const buddy_block_t* b);

#endif
