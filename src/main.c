#include<Heap.h>
int main()
{
    struct heap_t heap;
    hinit(8192, &heap);
    testGarbageCollection(&heap);
    testFragmentation(&heap);
    testHeapSprayingDetection(&heap);

    return 0;
}