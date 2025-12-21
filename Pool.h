#ifndef POOL_H
#define POOL_H
#include <stddef.h>

#define POOL_MAX_SIZE 128
#define POOL_PAGE_SIZE 4096

struct pool_t
{
    size_t blockSize;
    void *freeList;
    struct pool_t *next;
};

struct heap_t;
void poolInit(struct heap_t *heap);
void *poolAlloc(size_t size, struct heap_t *heap);
int poolFree(void *ptr, struct heap_t *heap);
#endif
