#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
struct heap_t
{
    struct chunk_t *start; // the chunk list head
    uint32_t avail;        // available memory
    struct region_t *regions;
};
struct region_t
{
    void *startOfRegion; // start of the region
    void *endOfRegion;   // end of the reaion for boundry check
    struct region_t *next;
};

int hinit(const size_t heapSize, struct heap_t *heap);
size_t alignPage(const size_t heapSize);
size_t align8(size_t size);
void *halloc(const size_t size, struct heap_t *heap);
struct chunk_t *firstFit(const size_t size, struct heap_t *heap, struct chunk_t **prevChunk);
struct chunk_t *requestMemory(size_t size, struct heap_t *heap);
int exictingPtrInHeap(void *ptr, struct heap_t *heap);
void hfree(void *ptr, struct heap_t *heap);
void coalescing(struct heap_t *heap);
void markAndSweep(struct heap_t *heap, void **roots, size_t numOfRoots);
void mark(struct chunk_t *chunk);
void sweep(struct heap_t *heap);

#endif
