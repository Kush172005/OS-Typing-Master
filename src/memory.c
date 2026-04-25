#include <stddef.h>

#include "memory.h"

#define OS_BLOCK_END (-1)

typedef struct {
    int size;
    int is_free;
    int next_offset;
} MemBlock;
// hidden MemBlock header

static union {
    long long alignment;
    unsigned char bytes[OS_VIRTUAL_RAM_SIZE];
} g_virtual_ram; // 128 KB array of system memory

static int g_initialized = 0;

static MemBlock *block_at_offset(int offset) {
    return (MemBlock *)(void *)(g_virtual_ram.bytes + offset);
}

void os_memory_init(void) {
    MemBlock *first;

    first = (MemBlock *)(void *)g_virtual_ram.bytes;
    first->size = OS_VIRTUAL_RAM_SIZE - (int)sizeof(MemBlock);
    first->is_free = 1;
    first->next_offset = OS_BLOCK_END;

    g_initialized = 1;
}

void *os_alloc(int size) {
    int current_offset;

    if (!g_initialized) {
        os_memory_init();
    }

    if (size <= 0) {
        return NULL;
    }

    current_offset = 0;
    while (current_offset != OS_BLOCK_END) {
        MemBlock *block = block_at_offset(current_offset);

        if (block->is_free && block->size >= size) {
            int remaining = block->size - size;

            if (remaining > (int)sizeof(MemBlock) + 4) {
                int next_offset = current_offset + (int)sizeof(MemBlock) + size;
                MemBlock *new_block = block_at_offset(next_offset);

                new_block->size = remaining - (int)sizeof(MemBlock);
                new_block->is_free = 1;
                new_block->next_offset = block->next_offset;

                block->next_offset = next_offset;
                block->size = size;
            }

            block->is_free = 0;
            return (void *)((unsigned char *)block + sizeof(MemBlock));
        }

        current_offset = block->next_offset;
    }

    return NULL;
}

void os_dealloc(void *ptr) {
    int current_offset;

    if (ptr == NULL) {
        return;
    }

    current_offset = 0;
    while (current_offset != OS_BLOCK_END) {
        MemBlock *block = block_at_offset(current_offset);
        void *data_start = (void *)((unsigned char *)block + sizeof(MemBlock));

        if (data_start == ptr) {
            block->is_free = 1;
            break;
        }

        current_offset = block->next_offset;
    }

    current_offset = 0;
    while (current_offset != OS_BLOCK_END) {
        MemBlock *block = block_at_offset(current_offset);

        if (block->next_offset != OS_BLOCK_END) {
            MemBlock *next = block_at_offset(block->next_offset);

            if (block->is_free && next->is_free) {
                block->size = block->size + (int)sizeof(MemBlock) + next->size;
                block->next_offset = next->next_offset;
                continue;
            }
        }

        current_offset = block->next_offset;
    }
}

int os_memory_used_bytes(void) {
    int used = 0;
    int current_offset = 0;

    while (current_offset != OS_BLOCK_END) {
        MemBlock *block = block_at_offset(current_offset);
        if (!block->is_free) {
            used += block->size;
        }
        current_offset = block->next_offset;
    }

    return used;
}

int os_memory_free_bytes(void) {
    int free_bytes = 0;
    int current_offset = 0;

    while (current_offset != OS_BLOCK_END) {
        MemBlock *block = block_at_offset(current_offset);
        if (block->is_free) {
            free_bytes += block->size;
        }
        current_offset = block->next_offset;
    }

    return free_bytes;
}
