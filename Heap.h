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
size_t allignPage(const size_t heapSize);
#endif
