#include "k_mem.h"
#include "k_task.h"

#include <stdint.h>

#define MAGIC_ALLOC  0xC0FFEEAAU
#define MAGIC_FREE   0xDEADBEEFU
#define OWNER_KERNEL ((task_t)0xFFFFFFFFU)
#define MEM_ALIGN    8U

extern uint32_t _img_end;
extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;

typedef struct mem_block {
    U32 size;                  /* total block size including header */
    U32 magic;
    task_t owner;
    struct mem_block *next;
    struct mem_block *prev;
    U32 reserved;              /* keeps payload 8-byte aligned */
} mem_block_t;

static mem_block_t *free_head = NULL;
static uintptr_t heap_start = 0U;
static uintptr_t heap_end = 0U;
static int mem_initialized = 0;



static uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
    return (value + alignment - 1U) & ~(alignment - 1U);
}

static uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    return value & ~(alignment - 1U);
}

static U32 aligned_payload_size(size_t size)
{
    uintptr_t aligned;

    if (size == 0U) {
        return 0U;
    }

    aligned = align_up((uintptr_t)size, MEM_ALIGN);

    if (aligned > UINT32_MAX) {
        return 0U;
    }

    return (U32)aligned;
}



static void remove_free(mem_block_t *block)
{
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        free_head = block->next;
    }

    if (block->next != NULL) {
        block->next->prev = block->prev;
    }

    block->next = NULL;
    block->prev = NULL;
}

static void insert_free_sorted(mem_block_t *block)
{
    mem_block_t *cursor = free_head;
    mem_block_t *previous = NULL;

    block->magic = MAGIC_FREE;
    block->owner = OWNER_KERNEL;
    block->next = NULL;
    block->prev = NULL;
    block->reserved = 0U;

    /*
     * Keep the free list ordered by increasing memory address. This allows
     * immediate adjacent-block coalescing and naturally supports first fit.
     */
    while (cursor != NULL &&
           (uintptr_t)cursor < (uintptr_t)block) {
        previous = cursor;
        cursor = cursor->next;
    }

    block->prev = previous;
    block->next = cursor;

    if (previous != NULL) {
        previous->next = block;
    } else {
        free_head = block;
    }

    if (cursor != NULL) {
        cursor->prev = block;
    }
}

static mem_block_t *coalesce(mem_block_t *block)
{
    /*
     * Merge with the next block.
     */
    if (block->next != NULL &&
        (uintptr_t)block + block->size ==
            (uintptr_t)block->next) {
        mem_block_t *next = block->next;

        block->size += next->size;
        block->next = next->next;

        if (block->next != NULL) {
            block->next->prev = block;
        }
    }

    /*
     * Merge with the previous block.
     */
    if (block->prev != NULL &&
        (uintptr_t)block->prev + block->prev->size ==
            (uintptr_t)block) {
        mem_block_t *previous = block->prev;

        previous->size += block->size;
        previous->next = block->next;

        if (block->next != NULL) {
            block->next->prev = previous;
        }

        block = previous;
    }

    return block;
}



static int block_header_is_valid(const mem_block_t *block)
{
    uintptr_t address = (uintptr_t)block;

    if (address < heap_start ||
        address + sizeof(mem_block_t) > heap_end) {
        return 0;
    }

    if (block->size < sizeof(mem_block_t)) {
        return 0;
    }

    if ((block->size & (MEM_ALIGN - 1U)) != 0U) {
        return 0;
    }

    if (address + block->size > heap_end) {
        return 0;
    }

    if (block->magic != MAGIC_ALLOC &&
        block->magic != MAGIC_FREE) {
        return 0;
    }

    return 1;
}

static mem_block_t *find_allocated_block(void *ptr)
{
    uintptr_t cursor;

    if (ptr == NULL || !mem_initialized) {
        return NULL;
    }

    /*
     * Walk every physical block rather than trusting metadata directly before
     * the supplied pointer. This rejects pointers into the middle of blocks.
     */
    cursor = heap_start;

    while (cursor < heap_end) {
        mem_block_t *block = (mem_block_t *)cursor;

        if (!block_header_is_valid(block)) {
            return NULL;
        }

        if (block->magic == MAGIC_ALLOC &&
            (void *)(cursor + sizeof(mem_block_t)) == ptr) {
            return block;
        }

        cursor += block->size;
    }

    return NULL;
}

static void free_block_unchecked(mem_block_t *block)
{
    insert_free_sorted(block);
    (void)coalesce(block);
}



static void *allocate_with_owner(size_t size, task_t owner)
{
    U32 payload_size;
    U32 required_size;
    mem_block_t *cursor;

    if (!mem_initialized) {
        return NULL;
    }

    payload_size = aligned_payload_size(size);

    if (payload_size == 0U ||
        payload_size >
            UINT32_MAX - (U32)sizeof(mem_block_t)) {
        return NULL;
    }

    required_size =
        payload_size + (U32)sizeof(mem_block_t);

    /*
     * The free list is sorted by address, so the first block large enough is
     * the required first-fit allocation.
     */
    cursor = free_head;

    while (cursor != NULL) {
        if (cursor->size >= required_size) {
            U32 remaining =
                cursor->size - required_size;

            /*
             * Split only when the remainder is large enough to contain a
             * metadata header and at least one aligned payload unit.
             */
            if (remaining >=
                (U32)sizeof(mem_block_t) + MEM_ALIGN) {
                mem_block_t *replacement =
                    (mem_block_t *)(
                        (uintptr_t)cursor +
                        required_size
                    );

                replacement->size = remaining;
                replacement->magic = MAGIC_FREE;
                replacement->owner = OWNER_KERNEL;
                replacement->prev = cursor->prev;
                replacement->next = cursor->next;
                replacement->reserved = 0U;

                if (replacement->prev != NULL) {
                    replacement->prev->next = replacement;
                } else {
                    free_head = replacement;
                }

                if (replacement->next != NULL) {
                    replacement->next->prev = replacement;
                }

                cursor->size = required_size;
                cursor->next = NULL;
                cursor->prev = NULL;
            } else {
                /*
                 * Give the caller the entire block when the remaining region
                 * is too small to be useful.
                 */
                remove_free(cursor);
            }

            cursor->magic = MAGIC_ALLOC;
            cursor->owner = owner;
            cursor->reserved = 0U;

            return (void *)(
                (uintptr_t)cursor +
                sizeof(mem_block_t)
            );
        }

        cursor = cursor->next;
    }

    return NULL;
}



int k_mem_init(void)
{
    uintptr_t start;
    uintptr_t end;

    /*
     * Memory initialization requires a previously initialized kernel and can
     * occur only once.
     */
    if (mem_initialized ||
        !k_kernel_is_initialized()) {
        return RTX_ERR;
    }

    start = align_up(
        (uintptr_t)&_img_end,
        MEM_ALIGN
    );

    end = align_down(
        (uintptr_t)&_estack -
            (uintptr_t)&_Min_Stack_Size,
        MEM_ALIGN
    );

    if (end <= start ||
        end - start <
            sizeof(mem_block_t) + MEM_ALIGN) {
        return RTX_ERR;
    }

    heap_start = start;
    heap_end = end;

    free_head = (mem_block_t *)heap_start;
    free_head->size = (U32)(heap_end - heap_start);
    free_head->magic = MAGIC_FREE;
    free_head->owner = OWNER_KERNEL;
    free_head->next = NULL;
    free_head->prev = NULL;
    free_head->reserved = 0U;

    mem_initialized = 1;

    return RTX_OK;
}

int k_mem_is_initialized(void)
{
    return mem_initialized;
}

void *k_mem_alloc_owner(size_t size, task_t owner)
{
    return allocate_with_owner(size, owner);
}

void *k_mem_alloc(size_t size)
{
    task_t owner;

    /*
     * Memory allocated while no task is active belongs to the kernel.
     */
    if (current_tcb != NULL) {
        owner = current_task;
    } else {
        owner = OWNER_KERNEL;
    }

    return allocate_with_owner(size, owner);
}

int k_mem_dealloc(void *ptr)
{
    mem_block_t *block;
    task_t caller;

    if (!mem_initialized) {
        return RTX_ERR;
    }

    /*
     * The lab specification treats deallocating NULL as successful.
     */
    if (ptr == NULL) {
        return RTX_OK;
    }

    block = find_allocated_block(ptr);

    if (block == NULL) {
        return RTX_ERR;
    }

    if (current_tcb != NULL) {
        caller = current_task;
    } else {
        caller = OWNER_KERNEL;
    }

    /*
     * User tasks may deallocate only memory they own. The kernel may release
     * any block through its internal management path.
     */
    if (caller != OWNER_KERNEL &&
        block->owner != caller) {
        return RTX_ERR;
    }

    free_block_unchecked(block);

    return RTX_OK;
}

int k_mem_release_task(task_t owner)
{
    int released_any;

    if (!mem_initialized ||
        owner == OWNER_KERNEL) {
        return RTX_ERR;
    }

    /*
     * Restart the physical heap scan after every release because coalescing
     * changes block locations and boundaries.
     */
    do {
        uintptr_t cursor = heap_start;
        released_any = 0;

        while (cursor < heap_end) {
            mem_block_t *block =
                (mem_block_t *)cursor;

            if (!block_header_is_valid(block)) {
                return RTX_ERR;
            }

            if (block->magic == MAGIC_ALLOC &&
                block->owner == owner) {
                free_block_unchecked(block);
                released_any = 1;
                break;
            }

            cursor += block->size;
        }
    } while (released_any);

    return RTX_OK;
}

int k_mem_count_extfrag(size_t size)
{
    int count = 0;
    mem_block_t *cursor;

    if (!mem_initialized) {
        return 0;
    }

    cursor = free_head;

    while (cursor != NULL) {
        /*
         * The free-block metadata itself is considered free space for the
         * external-fragmentation calculation.
         */
        if ((size_t)cursor->size < size) {
            ++count;
        }

        cursor = cursor->next;
    }

    return count;
}