#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "lib.h"
#include "mmu.h"

#define MEM_MAGIC       0xA5
#define MIN_SIZE        0x8
#define BUDDY_MIN       (1U << 11)
#define BUDDY_START     (1U << 26)
#define BUDDY_SIZE      (1U << 25)

typedef struct buddy_block {
    uint32_t size;
    bool free;
} buddy_block_t;

struct {
    buddy_block_t* start;
    buddy_block_t* end;
    uint32_t align;
} buddy_allocator;

struct mem {
    struct mem* next;
};

typedef struct slab {
    struct mem* freelist;
    struct slab* next;
    uint32_t size;
} slab_t;

slab_t* slab_head;

uint32_t buddy_cnt;
uint32_t slab_cnt;

void* kmalloc(uint32_t size);
int32_t kfree(void* mem);
void buddy_init(void* mem, uint32_t size, uint32_t alignment);
buddy_block_t* buddy_search(uint32_t size);
buddy_block_t* buddy_split(buddy_block_t* b, uint32_t size);
void buddy_coalesce(void);
void* buddy_alloc(uint32_t size);
int32_t buddy_free(void* mem);
slab_t* slab_create(uint32_t size);
void* slab_alloc(uint32_t size);
int32_t slab_free(void* mem);
void slab_traverse(void);

// DEBUG
int32_t buddy_traverse(void);
void buddy_print_block(const buddy_block_t* b);

#endif
