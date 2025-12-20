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

    // initialize the first chunk
    struct chunk_t *firstChunk = mmap(NULL, alignedPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    size_t remainedAvail = alignedPageSize - sizeof(struct chunk_t);
    firstChunk->size = remainedAvail;
    firstChunk->inuse = 0;
    firstChunk->next = NULL;
    firstChunk->magic = CHUNK_MAGIC;

    // initialize the first region
    struct region_t *firstReg = mmap(NULL, sizeof(struct region_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    firstReg->startOfRegion = firstChunk;
    firstReg->endOfRegion = (char *)firstChunk + alignedPageSize;
    firstReg->next = NULL;

    heap->start = firstChunk;
    heap->avail = firstChunk->size;
    heap->regions = firstReg;

    return 0;
}
size_t alignPage(size_t heapSize)
{
    long pageSize = sysconf(_SC_PAGESIZE);
    return ((heapSize + pageSize - 1) / pageSize) * pageSize;
}
size_t align8(size_t size)
{
    return ((size + 8 - 1) / 8) * 8; // every allocated block size would be a multiple of 8
}

void split(struct chunk_t *chunk, const size_t size)
{
    if (chunk->size > size + sizeof(struct chunk_t))
    {
        struct chunk_t *newChunk = (struct chunk_t *)((char *)chunk + sizeof(struct chunk_t) + size);
        newChunk->size = chunk->size - size - sizeof(struct chunk_t);
        newChunk->magic = CHUNK_MAGIC;
        newChunk->next = chunk->next;
        newChunk->inuse = 0;

        chunk->next = newChunk;
        chunk->size = size;
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
    if (foundChunk)
    {
        foundChunk->magic = CHUNK_MAGIC;
        split(foundChunk, alignedSize);
        foundChunk->inuse = 1;
    }
    else
    {
        struct chunk_t *newChunk = requestMemory(size, heap);
        prevChunk->next = newChunk;
    }
    heap->avail -= alignedSize;
    return (char *)foundChunk + sizeof(struct chunk_t);
}
struct chunk_t *firstFit(const size_t size, struct heap_t *heap, struct chunk_t **prevChunk)
{
    struct chunk_t *currChuck = heap->start;
    *prevChunk = NULL;

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

    struct region_t *newReg = mmap(NULL, sizeof(struct region_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    newReg->startOfRegion = newChunk;
    newReg->endOfRegion = (char *)newChunk + alignedPageSize;
    newReg->next = heap->regions;
    heap->regions = newReg;

    heap->avail += newChunk->size;

    return newChunk;
}
int exictingPtrInHeap(void *ptr, struct heap_t *heap)
{
    struct region_t *newTmpReg = heap->regions;
    while (newTmpReg)
    {
        if (ptr >= newTmpReg->startOfRegion && ptr < newTmpReg->endOfRegion)
            return 1;
        newTmpReg = newTmpReg->next;
    }

    return 0;
}
void coalescing(struct heap_t *heap)
{
    struct chunk_t *currChunk = heap->start;
    while (currChunk && currChunk->next)
    {
        if (!currChunk->inuse && !currChunk->next->inuse && (char *)currChunk + sizeof(struct chunk_t) + currChunk->size == (char *)currChunk->next)
        {
            currChunk->size += currChunk->next->size + sizeof(struct chunk_t);
            currChunk->next = currChunk->next->next;
        }
        else
        {
            currChunk = currChunk->next;
        }
    }
}

void hfree(void *ptr, struct heap_t *heap)
{
    if (!ptr)
    {
        errno = EINVAL;
        return;
    }

    if (!exictingPtrInHeap(ptr, heap))
    {
        errno = EINVAL;
        return;
    }

    struct chunk_t *chunk = (struct chunk_t *)((char *)ptr - sizeof(struct chunk_t));

    if (chunk->magic != CHUNK_MAGIC) // metadata inegrity check
    {
        errno = EINVAL;
        return;
    }
    if (!chunk->inuse) // double free check
    {
        errno = EINVAL;
        return;
    }

    chunk->inuse = 0;
    heap->avail += chunk->size;
    coalescing(heap);
}
void mark(struct chunk_t *chunk)
{
    if(!chunk)
    {
        return;
    }
    if(chunk->marked)
    {
        return;
    }
    char *userDataStart = (char *)chunk + sizeof(struct chunk_t);
    char *userDataEnd = chunk->size + userDataStart;

    for(char *ptr = userDataStart; ptr < userDataEnd; ptr += sizeof(void*))
    {
        void **possibleUserPtr = (void**)ptr;
        if(*possibleUserPtr)
        {
            struct chunk_t *childChunk = (struct chunk_t*)((char *)*possibleUserPtr) - sizeof(struct chunk_t);
            if(childChunk->inuse)
            {
                mark(childChunk);
            }
        }
    }

}
void sweep(struct heap_t *heap)
{
    struct chunk_t *currChunk = heap->start;
    while(currChunk)
    {
        if(currChunk->inuse && !currChunk->marked)
        {
            currChunk->inuse = 0;
            heap->avail += currChunk->size;

        }
        currChunk = currChunk->next;
    }
}
void markAndSweep(struct heap_t *heap, void **roots, size_t numOfRoots)
{

    
}



int main()
{
    struct heap_t heap;
    hinit(4096, &heap);
    printf("%p %d", (void *)heap.start, heap.avail);
    return 0;
}
