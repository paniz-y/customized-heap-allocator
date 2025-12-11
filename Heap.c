#include "Chunk.h"
#include "Heap.h"

int hinit(size_t heapSize)
{
    if(!heapSize)
    {
        errno = EINVAL;
        return -1;
    }
    allignPage(heapSize);
    return 0;
}
size_t allignPage(size_t heapSize)
{
    long pageSize = sysconf(_SC_PAGESIZE);
    return ((heapSize + pageSize - 1) / pageSize) * pageSize;
    
}