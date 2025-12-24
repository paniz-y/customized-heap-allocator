# Custom Heap Allocator

**Operating Systems Project**

## Project Overview
This project implements a custom heap allocator in C designed similar to standard 'malloc' and 'free'.
This allocator uses First-fit search with splitting as allocation strategy and uses coalescing to handle extarnal fragmentation.
Memory is acquired directly from the kernel via mmap.
Error handling is implemeted via 'errno'.

## Features
- **Memory Pool** — Fixed-size block pools for small allocations to handle internal fragmentation.
- **Garbage Collection** _ Mark-and-Sweep algorithm.
- **Heap Spraying Prevention** — Allocation counting per size bucket and threshold-based rejection (1000 same-size allocations trigger 'EPERM').
