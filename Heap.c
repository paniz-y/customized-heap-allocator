#include "Chunk.h"
#include "Heap.h"

int hinit(size_t heapSize, struct heap_t *heap)
{
    if (!heapSize)
    {
        errno = EINVAL;
        return -1;
    }
    size_t alignedPageSize = alignPage(heapSize);
    struct chunk_t *firstChunk = mmap(NULL, alignedPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    size_t remainedAvail = alignedPageSize - sizeof(struct chunk_t);
    firstChunk->size = remainedAvail;
    firstChunk->inuse = 0;
    firstChunk->next = NULL;
    firstChunk->magic = CHUNK_MAGIC;

    heap->heap_start = firstChunk;
    heap->heap_end = (char *)firstChunk + alignedPageSize; // enable all future security checks

    heap->start = firstChunk;
    heap->avail = firstChunk->size;

    return 0;
}
size_t alignPage(size_t heapSize)
{
    long pageSize = sysconf(_SC_PAGESIZE);
    return ((heapSize + pageSize - 1) / pageSize) * pageSize;
}
size_t align8(size_t size)
{
    return ((size + 8 - 1) / 8) * 8; //every allocated block size would be a multiple of 8
}

void split(struct chunk_t *chunk, const size_t size)
{
    if (chunk->size > size + sizeof(struct chunk_t))
    {
        struct chunk_t *newChunk = (struct chunk_t *)((char *)chunk + sizeof(struct chunk_t) + size);
        newChunk->size = chunk->size - size - sizeof(struct chunk_t);
        newChunk->next = chunk->next;
        newChunk->inuse = 0;
        chunk->next = newChunk;
        chunk->magic = CHUNK_MAGIC;
    }
}
void *halloc(const size_t size, struct heap_t *heap)
{
    if (!size || size > heap->avail) 
    {
        errno = EINVAL;
        return NULL;
    }
    size_t alignedSize = align8(size);
    struct chunk_t *prevChunk = NULL;
    struct chunk_t *foundChunk = firstFit(alignedSize, heap, &prevChunk);
    foundChunk->magic = CHUNK_MAGIC;
    if (foundChunk)
    {
        split(foundChunk, alignedSize);
        foundChunk->inuse = 1;
        heap->avail -= alignedSize;
        return (char *)foundChunk + sizeof(struct chunk_t);
    }
    else
    {
        struct chunk_t *newChunk = requestMemory(size, heap);
        prevChunk->next = newChunk;
    }
}
struct chunk_t *firstFit(const size_t size, struct heap_t *heap, struct chunk_t **prevChunk)
{
    struct chunk_t *currChuck = heap->start;
    while (currChuck)
    {
        if (!currChuck->inuse && size <= currChuck->size)
        {
            return currChuck;
        }
        *prevChunk = currChuck;
        currChuck = currChuck->next;
    }
    return NULL;
}
struct chunk_t *requestMemory(size_t size, struct heap_t *heap)
{
    size_t alignedPageSize = alignPage(size + sizeof(struct chunk_t));
    struct chunk_t *newChunk = mmap(NULL, alignedPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    newChunk->size = alignedPageSize - sizeof(struct chunk_t);
    newChunk->next = NULL;
    newChunk->inuse = 1;
    newChunk->magic = CHUNK_MAGIC;

    return newChunk;
}
void hfree(void *ptr, struct heap_t *heap)
{
    if (!ptr)
    {
        errno = EINVAL;
        return;
    }
    if(ptr < heap->heap_start || ptr >= heap->heap_end) //bounbry check
    {
        errno = EINVAL;
        return;
    }
    struct chunk_t *chunk = (struct chunk_t *)((char *)ptr - sizeof(struct chunk_t));

    if(chunk->magic != CHUNK_MAGIC) // metadata inegrity check
    {
        errno = EINVAL;
        return;
    }
    if(!chunk->inuse) // double free check
    {
        errno = EINVAL;
        return;
    }

    chunk->inuse = 0;
    heap->avail += chunk->size;
}

int main()
{
    struct heap_t heap;
    hinit(4096, &heap);
    printf("%p %d", (void *)heap.start, heap.avail);
    return 0;
}