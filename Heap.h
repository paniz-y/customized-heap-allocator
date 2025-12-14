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
struct chunk_t *start;
uint32_t avail; // available memory
};
int hinit(const size_t heapSize ,struct heap_t* heap);
size_t alignPage(const size_t heapSize);
size_t align8(size_t size);
void *halloc(const size_t size, struct heap_t *heap);
struct chunk_t *firstFit(const size_t size, struct heap_t *heap, struct chunk_t *prevChunk);
struct chunk_t *requestMemory(size_t size, struct heap_t *heap);



#endif
