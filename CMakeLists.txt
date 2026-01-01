cmake_minimum_required(VERSION 3.10)

project(CustomHeapAllocator LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(heap
    src/Heap.c
    src/Pool.c
    src/main.c
)
target_include_directories(heap PUBLIC include)

target_link_libraries(heap m)

