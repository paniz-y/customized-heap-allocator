#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <bits/confname.h>
struct heap_t 
{
struct chunk_t *start;
uint32_t avail; // available memory
};
int hinit(const size_t heapSize);
size_t allignPage(const size_t heapSize);
#endif