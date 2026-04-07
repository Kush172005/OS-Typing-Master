#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#define OS_VIRTUAL_RAM_SIZE 131072

/* Virtual memory manager API. */
void os_memory_init(void);
void *os_alloc(int size);
void os_dealloc(void *ptr);
int os_memory_used_bytes(void);
int os_memory_free_bytes(void);

#endif
