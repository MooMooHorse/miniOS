#include "kmalloc.h"

#define CHECK_ALIGNMENT(x)  \
    (!!(x) && (!((x) & ((x) - 1))))

#define HEADER_SIZE  (sizeof(buddy_block_t))
#define START        (buddy_allocator.start)
#define END          (buddy_allocator.end)
#define ALIGN        (buddy_allocator.align)

#define GET_NEXT(b)         \
    ((buddy_block_t*) ((uint8_t*) b + ((buddy_block_t*) b)->size))

#define ROUNDUP_SIZE(x)     \
    (((x) + ALIGN - 1) & ~(ALIGN - 1))

#define GET_BLOCK_SIZE(x)                   \
    ({                                      \
        uint32_t sz = ALIGN;                \
        uint32_t t;                         \
        t = ROUNDUP_SIZE(x + HEADER_SIZE);  \
        while (sz < t) { sz <<= 1; }        \
        sz;                                 \
    })

#define GET_SLAB_SIZE(x)                    \
    ({                                      \
        uint32_t sz = MIN_SIZE;             \
        while (sz < x) { sz <<= 1; }        \
        sz;                                 \
    })

buddy_block_t*
buddy_split(buddy_block_t* b, uint32_t size) {
    if (!b || !size) {
        return NULL;  // Invalid arguments!
    }

    while (size < b->size) {
        b->size >>= 1;
        GET_NEXT(b)->size = b->size;
        GET_NEXT(b)->free = true;
    }

    return b;
}

buddy_block_t*
buddy_search(uint32_t size) {
    buddy_block_t* res = NULL;
    buddy_block_t* curr = START;
    buddy_block_t* next = GET_NEXT(START);

    if (next >= END && curr->free) {
        return buddy_split(curr, size);  // Buddy out of bounds.
    }

    while (curr < END && next < END) {
        if (curr->free && next->free && curr->size == next->free) {
            curr->size <<= 1;  // Coalesce two blocks.

            if (size <= curr->size && (NULL == res || curr->size <= res->size)) {
                res = curr;  // Update result block.
            }

            // Increment curr & next pointer.
            if (END > (curr = GET_NEXT(next))) {
                next = GET_NEXT(curr);
            }
            continue;
        }

        if (curr->free && size <= curr->size && (NULL == res || curr->size <= res->size)) {
            res = curr;
        }

        // Implicitly prefer the left (curr) block.
        if (next->free && size <= next->size && (NULL == res || next->size < res->size)) {
            res = next;
        }

        if (curr->size <= next->size) {
            // Skip the next block -- it either has been or will not be a candidate.
            if (END > (curr = GET_NEXT(next))) {
                next = GET_NEXT(curr);
            }
            continue;
        }
        curr = next;
        next = GET_NEXT(curr);
    }

    return res ? buddy_split(res, size) : NULL;
}

void
buddy_coalesce(void) {
    buddy_block_t* curr;
    buddy_block_t* next;
    bool done;

    while (1) {
        curr = START;
        next = GET_NEXT(START);
        done = true;

        while (curr < END && next < END) {
            if (curr->free && next->free && curr->size == next->size) {
                curr->size <<= 1;  // Coalesce two blocks.

                // Skip the coalesced block.
                if (END > (curr = GET_NEXT(curr))) {
                    next = GET_NEXT(curr);
                    done = false;
                }
                continue;
            }

            curr = next;
            next = GET_NEXT(curr);
        }

        if (done) { return; }
    }
}

void
buddy_init(void* mem, uint32_t size, uint32_t align) {
    if (!mem || !CHECK_ALIGNMENT(size)) {
        panic("buddy allocator init, invalid pointer/size");
    }

    // The alignment must be large enough to hold metadata.
    align = align < sizeof(buddy_block_t) ? sizeof(buddy_block_t) : align;

    if (!CHECK_ALIGNMENT(align) || 0 != (uint32_t) mem % align) {
        panic("buddy allocator init, alignment");
    }

    // Initialize the first buddy block & buddy allocator.
    START = mem;
    START->size = size;
    START->free = true;
    END = GET_NEXT(START);
    ALIGN = align;

    // DEBUG
    printf("\nBuddy allocator initialized!\n");
    printf("sizeof buddy_block_t: %u\n", sizeof(buddy_block_t));
    printf("start: 0x%x\n", START);
    printf("start.size: 0x%x\n", START->size);
    printf("start.free: %s\n", START->free ? "true" : "false");
    printf("end: 0x%x\n", END);
    printf("alignment: 0x%x\n\n", ALIGN);
}

void
buddy_print_block(const buddy_block_t* b) {
    printf("block: 0x%x\n", b);
    printf("block.size: 0x%x\n", b->size);
    printf("block.free: %s\n", b->free ? "true" : "false");
}

void
buddy_traverse(void) {
    buddy_block_t* curr = START;
    int32_t i = 0;
    printf("\nBuddy Allocator Traverse\n");

    while (curr < END) {
        printf("\nBuddy block #%d\n", i++);
        buddy_print_block(curr);
        curr = GET_NEXT(curr);
    }
}

void*
buddy_alloc(uint32_t size) {
    buddy_block_t* res;
    uint32_t sz;
    if (0 == size) { return NULL; }

    sz = GET_BLOCK_SIZE(size);
    if (NULL == (res = buddy_search(sz))) {
        buddy_coalesce();
        res = buddy_search(sz);
    }

    if (NULL == res) {
        return NULL;
    }
    res->free = false;
    return (void*) ((uint8_t*) res + ALIGN);
}

int32_t
buddy_free(void* mem) {
    buddy_block_t* b;
    if (NULL == mem) {
        panic("buddy free, attempted to free a nullptr");
    }
    b = (buddy_block_t*) ((uint8_t*) mem - ALIGN);
    if (START > b || END <= b) {
        return 1;
    }
    b->free = true;
    memset((uint8_t*) b + HEADER_SIZE, MEM_MAGIC, b->size - HEADER_SIZE);
    return 0;
}

void
slab_traverse(void) {
    slab_t* s = slab_head;
    struct mem* m;
    int32_t i = 0;
    int32_t j;
    printf("\nSlab Allocator Traverse\n");

    while (s) {
        printf("Slab #%d\n", i++);
        printf("slab.size: %u\n", s->size);
        m = s->freelist;
        j = 0;
        while (m) {
//            printf("slab object at 0x%x\n", m);
            j++;
            m = m->next;
        }
        printf("%u slab objects found.\n", j);
        s = s->next;
    }
}

slab_t*
slab_create(uint32_t size) {
    int32_t i;
    slab_t* s;
    struct mem* mem;
    if (NULL == (s = buddy_alloc(PGSIZE))) {
        return NULL;  // No available memory in buddy allocator.
    }
    memset(s, 0, PGSIZE);

    // Add the new slab to the global slab list.
    s->next = slab_head;
    slab_head = s;

    // Initialize struct slab.
    s->size = size;
    mem = (struct mem*) ((uint8_t*) s + sizeof(*s));
    s->freelist = mem;

    for (i = 1; (uint8_t*) mem + size < (uint8_t*) s + PGSIZE; ++i) {
        mem->next = (struct mem*) ((uint8_t*) s->freelist + i * size);
        mem = mem->next;
    }

    return s;
}

static struct mem*
_slab_alloc(slab_t* s) {
    struct mem* mem;
    mem = s->freelist;
    s->freelist = s->freelist->next;
    return mem;
}

void*
slab_alloc(uint32_t size) {
    slab_t* s = slab_head;
    size = (size < MIN_SIZE) ? MIN_SIZE : GET_SLAB_SIZE(size);
    while (s) {
        if (s->freelist && size == s->size) {
            goto SLAB_ALLOC;
        }
        s = s->next;
    }
    if (NULL == (s = slab_create(size))) {
        return NULL;
    }

SLAB_ALLOC:
    return (void*) _slab_alloc(s);
}

int32_t
slab_free(void* mem) {
    struct mem* m = mem;
    slab_t* s = slab_head;

    for (; s; s = s->next) {
        if ((void*) s > mem || (uint8_t*) s + PGSIZE < (uint8_t*) mem) {
            continue;
        }
        m->next = s->freelist;
        s->freelist = m;
        return 0;
    }

    return -1;
}

void*
kmalloc(uint32_t size) {
    if (BUDDY_MIN < size) {
        return buddy_alloc(size);
    }
    return slab_alloc(size);
}

void
kfree_range(void* start, void* end) {
    uint8_t* p = (uint8_t*) PGROUNDUP((uint32_t) start);
    for (; p + PGSIZE <= (uint8_t*) end; p += PGSIZE) {
        kfree(p);
    }
}

int32_t
kfree(void* mem) {
    if (0 == slab_free(mem) || 0 == buddy_free(mem)) {
        return 0;
    }
    return 1;
}
