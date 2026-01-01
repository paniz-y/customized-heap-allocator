#include "Pool.h"
#include "Heap.h"
#include <sys/mman.h>
#include <stdint.h>

void poolInit(struct heap_t *heap)
{
    heap->pools = NULL;

    for (size_t i = 0; i < heap->numPools; i++)
    {
        struct pool_t *firstPool = mmap(NULL, sizeof(struct pool_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (firstPool == MAP_FAILED)
        {
            errno = ENOMEM;
            return;
        }

        firstPool->blockSize = heap->poolSizes[i];
        firstPool->freeList = NULL;
        firstPool->next = heap->pools;

        heap->pools = firstPool;
    }
}

void *poolAlloc(size_t size, struct heap_t *heap)
{
    struct pool_t *newPool = heap->pools;

    while (newPool && newPool->blockSize < size) // find the smallest pool fiting the size
        newPool = newPool->next;

    if (!newPool)
        return NULL;

    void *poolPage = mmap(NULL, POOL_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (poolPage == MAP_FAILED)
    {
        errno = ENOMEM;
        return NULL;
    }
    struct pool_region_t *newReg = mmap(NULL, sizeof(struct pool_region_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newReg == MAP_FAILED)
    {
        errno = ENOMEM;
        return NULL;
    }

    newReg->poolRegionStart = poolPage;
    newReg->poolRegionEnd = (char *)poolPage + POOL_PAGE_SIZE;
    newReg->nextPoolRegion = heap->poolRegions;
    heap->poolRegions = newReg;

    size_t numberOfBlocks = POOL_PAGE_SIZE / newPool->blockSize;

    char *pagePtr = (char *)poolPage;

    for (size_t i = 0; i < numberOfBlocks; i++)
    {
        *(void **)pagePtr = newPool->freeList;
        newPool->freeList = pagePtr;
        pagePtr += newPool->blockSize;
    }

    void *newBlock = newPool->freeList;
    newPool->freeList = *(void **)newBlock;

    return newBlock;
}

int poolFree(void *ptr, struct heap_t *heap)
{
    if (!ptrInPool(ptr, heap))
    {
        errno = EINVAL;
        return 0;
    }

    struct pool_t *pool = heap->pools;

    while (pool)
    {
        if (((uintptr_t)ptr % pool->blockSize) == 0)
        {
            *(void **)ptr = pool->freeList;
            pool->freeList = ptr;
            return 1;
        }
        pool = pool->next;
    }
    return 0;
}

int ptrInPool(void *ptr, struct heap_t *heap)
{
    uintptr_t newPtr = (uintptr_t)ptr;
    struct pool_region_t *reg = heap->poolRegions;

    while (reg)
    {
        uintptr_t start = (uintptr_t)reg->poolRegionStart;
        uintptr_t end = (uintptr_t)reg->poolRegionEnd;

        if (newPtr >= start && newPtr < end)
            return 1;

        reg = reg->nextPoolRegion;
    }
    return 0;
}
