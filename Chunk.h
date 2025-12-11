#ifndef CHUNK_H
#define CHUNK_H
#include <stdint.h>
struct chunk_t
{
    uint32_t size;
    uint8_t inuse; // boolean
    struct chunk_t *next;
};
#endif