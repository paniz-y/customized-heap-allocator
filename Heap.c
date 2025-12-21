#include "Chunk.h"
#include "Heap.h"
#include "Pool.h"

static size_t allocationCounts[1024] = {0};

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
    if (firstChunk == MAP_FAILED)
    {
        errno = ENOMEM;
        return -1;
    }
    size_t remainedAvail = alignedPageSize - sizeof(struct chunk_t);
    firstChunk->size = remainedAvail;
    firstChunk->inuse = 0;
    firstChunk->next = NULL;
    firstChunk->magic = CHUNK_MAGIC;

    // initialize the first region
    struct region_t *firstReg = mmap(NULL, sizeof(struct region_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (firstReg == MAP_FAILED)
    {
        errno = ENOMEM;
        return -1;
    }
    firstReg->startOfRegion = firstChunk;
    firstReg->endOfRegion = (char *)firstChunk + alignedPageSize;
    firstReg->next = NULL;

    heap->start = firstChunk;
    heap->avail = firstChunk->size;
    heap->regions = firstReg;

    // memory pool
    poolInitialize(heap);
    poolInit(heap);

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
uint8_t detectHeapSpraying(size_t alignedSize)
{
    size_t bucket = alignedSize % 1024;
    allocationCounts[bucket]++;
    if (allocationCounts[bucket] > 1000)
    {
        errno = EPERM;
        return 1;
    }
    return 0;
}

void *halloc(const size_t size, struct heap_t *heap)
{
    if (!size)
    {
        errno = EINVAL;
        return NULL;
    }
    if (size <= POOL_MAX_SIZE)
    {
        void *pool = poolAlloc(align8(size), heap);
        if (pool)
            return pool;
    }
    size_t alignedSize = align8(size);

    if(detectHeapSpraying(alignedSize))
    {
        return NULL;
    }

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
        struct chunk_t *newChunk = requestMemory(alignedSize, heap);
        foundChunk = newChunk;
        if (prevChunk)
        {
            prevChunk->next = foundChunk;
        }
        else
        {
            heap->start = foundChunk;
        }
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
    if (newChunk == MAP_FAILED)
    {
        errno = ENOMEM;
        return NULL;
    }
    newChunk->size = alignedPageSize - sizeof(struct chunk_t);
    newChunk->next = NULL;
    newChunk->inuse = 1;
    newChunk->magic = CHUNK_MAGIC;

    struct region_t *newReg = mmap(NULL, sizeof(struct region_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newReg == MAP_FAILED)
    {
        errno = ENOMEM;
        return NULL;
    }
    newReg->startOfRegion = newChunk;
    newReg->endOfRegion = (char *)newChunk + alignedPageSize;
    newReg->next = heap->regions;
    heap->regions = newReg;

    heap->avail += newChunk->size;

    return newChunk;
}
int existingPtrInHeap(void *ptr, struct heap_t *heap)
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

    if (poolFree(ptr, heap))
    {
        return;
    }

    if (!existingPtrInHeap(ptr, heap))
    {
        errno = EINVAL;
        return;
    }

    struct chunk_t *chunk = (struct chunk_t *)((char *)ptr - sizeof(struct chunk_t));

    if (!existingPtrInHeap(chunk, heap))
    {
        errno = EINVAL;
        return;
    }

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
void mark(struct chunk_t *chunk, struct heap_t *heap)
{
    if (!chunk)
    {
        return;
    }
    if (chunk->marked)
    {
        return;
    }
    chunk->marked = 1;
    char *userDataStart = (char *)chunk + sizeof(struct chunk_t);
    char *userDataEnd = chunk->size + userDataStart;
    printf("219\n");
    for (char *ptr = userDataStart; ptr < userDataEnd; ptr += sizeof(void *))
    {
        printf("222\n");
        void **possibleUserPtr = (void **)ptr;
        if (*possibleUserPtr)
        {
            printf("226\n");
            if (!existingPtrInHeap(*possibleUserPtr, heap))
            {
                continue;
            }
            struct chunk_t *childChunk = (struct chunk_t *)((char *)*possibleUserPtr) - sizeof(struct chunk_t);
            if (childChunk->inuse)
            {
                printf("230\n");
                mark(childChunk, heap);
            }
        }
    }
}
void sweep(struct heap_t *heap)
{
    struct chunk_t *currChunk = heap->start;
    while (currChunk)
    {
        if (currChunk->inuse && !currChunk->marked)
        {
            currChunk->inuse = 0;
            heap->avail += currChunk->size;
        }
        currChunk = currChunk->next;
    }
}
void unmarkAllChunks(struct heap_t *heap)
{
    struct chunk_t *currChunk = heap->start;
    while (currChunk)
    {
        if (currChunk->marked)
        {
            currChunk->marked = 0;
        }
        currChunk = currChunk->next;
    }
}
void poolInitialize(struct heap_t *heap)
{
    size_t size = 8;
    for (size_t i = 1; i <= 5; i++)
    {
        heap->poolSizes[i] = (size_t)pow(size, i);
    }
    heap->numPools = 5;
}
void markAndSweep(struct heap_t *heap, void **roots, size_t numOfRoots)
{
    unmarkAllChunks(heap);

    for (size_t i = 0; i < numOfRoots; i++)
    {
        if (roots[i])
        {
            struct chunk_t *currChunk = (struct chunk_t *)((char *)roots[i] - sizeof(struct chunk_t));
            if (currChunk->inuse)
            {
                mark(currChunk, heap);
            }
        }
    }
    sweep(heap);
}

int main()
{
    struct heap_t heap;
    hinit(8192, &heap);
    //printf("295\n");
    void *p1 = halloc(100, &heap);
    //printf("297\n");
    void *p2 = halloc(500, &heap);
    //printf("299\n");
    void *p3 = halloc(10000, &heap);
    //printf("301\n");
    void *p4 = halloc(2000, &heap);
    //printf("303\n");

    p4 = NULL;
    void *roots[] = {p1, p2, p3};
    markAndSweep(&heap, roots, 3);

    return 0;
}
