#include "kmalloc.h"

#define CHECK_ALIGNMENT(x)  \
    (!!(x) && (!((x) & ((x) - 1))))

#define HEADER_SIZE  (sizeof(buddy_block_t))
#define START        (buddy_allocator.start)
#define END          (buddy_allocator.end)
#define ALIGN        (buddy_allocator.alignment)

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

struct mem {
    struct mem* next;
};

struct mem* kmem;

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
buddy_init(void* mem, uint32_t size, uint32_t alignment) {
    if (!mem || !CHECK_ALIGNMENT(size)) {
        panic("buddy allocator init, invalid pointer/size");
    }

    // The alignment must be large enough to hold metadata.
    alignment = alignment < sizeof(buddy_block_t) ? sizeof(buddy_block_t) : alignment;

    if (!CHECK_ALIGNMENT(alignment) || 0 != (uint32_t) mem % alignment) {
        panic("buddy allocator init, alignment");
    }

    // Initialize the first buddy block & buddy allocator.
    START = mem;
    START->size = size;
    START->free = true;
    END = GET_NEXT(START);
    ALIGN = alignment;

    // DEBUG
    printf("\nBuddy allocator initialized!\n");
    printf("sizeof buddy_block_t: %u\n", sizeof(buddy_block_t));
    printf("start: %u\n", START);
    printf("start.size: %u\n", START->size);
    printf("start.free: %u\n", START->free);
    printf("end: %u\n", END);
    printf("alignment: %u\n", ALIGN);
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
buddy_malloc(uint32_t size) {
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
    memset((uint8_t*) res + ALIGN, MEM_MAGIC, size);
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
    return 0;
}

void*
kmalloc(uint32_t size) {
//    struct mem* m = kmem;
//
//    if (m) {
//        kmem = m->next;
//    }
    void* res;
    res = buddy_malloc(size);

    return res;
}

void
kfree_range(void* start, void* end) {
    uint8_t* p = (uint8_t*) PGROUNDUP((uint32_t) start);
    for (; p + PGSIZE <= (uint8_t*) end; p += PGSIZE) {
        kfree(p);
    }
}

void
kfree(void* v) {
//    struct mem* m = v;
//    if ((uint32_t) v << (PTESIZE - PDXOFF)) {
//        panic("kfree: va not aligned");
//    }

    buddy_free(v);

//    memset(v, MEM_MAGIC, PGSIZE);

//    m->next = kmem;
//    kmem = m;
}
