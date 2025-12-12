#include "Chunk.h"
#include "Heap.h"


int hinit(size_t heapSize, struct heap_t* heap)
{
    if(!heapSize)
    {
        errno = EINVAL;
        return -1;
    }
    size_t allignedPageSize = allignPage(heapSize);
    struct chunk_t* firstChunk = mmap(NULL, allignedPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    size_t remainedAvail = allignedPageSize - sizeof(struct chunk_t);
    firstChunk->size = remainedAvail;
    firstChunk->inuse = 0;
    firstChunk->next = NULL;

    heap->start = firstChunk;
    heap->avail = firstChunk->size;

    return 0;
}
size_t allignPage(size_t heapSize)
{
    long pageSize = sysconf(_SC_PAGESIZE);
    return ((heapSize + pageSize - 1) / pageSize) * pageSize;
    
}

int main()
{
    struct heap_t heap;
    hinit(4096, &heap);
    printf("%p %d", (void*)heap.start, heap.avail);
    return 0;
}