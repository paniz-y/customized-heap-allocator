#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
struct heap_t 
{
struct chunk *start;
uint32_t avail; // available memory
};
#endif