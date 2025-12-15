#ifndef CHUNK_H
#define CHUNK_H
#include <stdint.h>
#define CHUNK_MAGIC 0xDEADBEEF
struct chunk_t
{
    uint32_t size;
    uint8_t inuse; // boolean
    struct chunk_t *next;
    uint32_t magic;
};
#endif