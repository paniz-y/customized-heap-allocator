#include "Chunk.h"
#include "Heap.h"

int hinit(size_t heapSize, struct heap_t *heap)
{
    if (!heapSize)
    {
        errno = EINVAL;
        return -1;
    }
    size_t alignedPageSize = allignPage(heapSize);
    struct chunk_t *firstChunk = mmap(NULL, alignedPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    size_t remainedAvail = alignedPageSize - sizeof(struct chunk_t);
    firstChunk->size = remainedAvail;
    firstChunk->inuse = 0;
    firstChunk->next = NULL;

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
    return ((size + 8 - 1) / 8) * size;
}
void *halloc(const size_t size)
{
    if (!size)
    {
        errno = EINVAL;
        return -1;
    }
    size_t alignedSize = align8(size);
}
struct chunk_t *firstFit(const size_t size, struct heap_t *heap)
{
    struct chunk_t *currChuck = heap->start;
    while (currChuck)
    {
        if (!currChuck->inuse && size <= currChuck->size)
        {
            return currChuck;
        }
        currChuck = currChuck->next;
        return NULL;
    }
}
int main()
{
    struct heap_t heap;
    hinit(4096, &heap);
    printf("%p %d", (void *)heap.start, heap.avail);
    return 0;
}