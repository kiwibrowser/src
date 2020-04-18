# Memory Usage in CC

This document gives an overview of memory usage in the CC component, as well as
information on how to analyze that memory.

[TOC]

## Types of Memory in Use

CC uses a number of types of memory:

1.  Malloc Memory - Standard system memory used for all manner of objects in CC.
2.  Discardable Memory - Memory allocated by the discardable memory system.
    Designed to be freeable by the system at any time (under memory pressure).
    In most cases, only pinned discardable memory should be considered to
    have a cost; however, the implementation of discardable memory is platform
    dependent, and on certain platforms unpinned memory can contribute to
    memory pressure to some degree.
3.  Shared Memory - Memory which is allocated by the Browser and can safely
    be transferred between processes. This memory is allocated by the browser
    but may count against a renderer process depending on who logically "owns"
    the memory.
4.  GPU Memory - Memory which is allocated on the GPU and typically does not
    count against system memory. This mainly includes OpenGL objects.

## Categories Of Memory

Memory-infra tracing will grab dumps of CC memory in several categories.

### CC Category

The CC category contains resource allocations made by ResourceProvider. All
resource allocations are enumerated under cc/resource_memory. A subset of
resources are used as tile memory, and are also enumerated under cc/tile_memory.
For resources that appear in both cc/tile_memory and cc/resource_memory, the
size will be attributed to cc/tile_memory (effective_size of cc/resource_memory
will not include these resources).

If the one-copy tile update path is in use, the cc category will also enumerate
staging resources used as intermediates when drawing tiles. These resources are
like tile_memory, in that they are shared with cc/resource_memory.

Note that depending on the path being used, CC memory may be either shared
memory or GPU memory:

```
Path         | Tile Memory Type     | Staging Memory Type
-------------|-------------------------------------------
Bitmap       | Shared Memory        | N/A
One Copy     | GPU Memory           | Shared Memory
Zero Copy    | GPU or Shared Memory | N/A
GPU          | GPU Memory           | N/A
```

Note that these values can be determined from a memory-infra dump. For a given
resource, hover over the small green arrow next to it's "size". This will show
the other allocations that this resource is aliased with. If you see an
allocation in the GPU process, the memory is generally GPU memory. Otherwise,
the resource is typically Shared Memory.

Tile and Staging memory managers are set up to evict any resource not used
within 1s.

### GPU Category

This category lists the memory allocations needed to support CC's GPU path.
Despite the name, the data in this category (within a Renderer process) is not
GPU memory but Shared Memory.

Allocations tracked here include GL command buffer support allocations such as:

1.  Command Buffer Memory - memory used to send commands across the GL command
    buffer. This is backed by Shared Memory.
2.  Mapped Memory - memory used in certain image upload paths to share data
    with the GPU process. This is backed by Shared Memory.
3.  Transfer Buffer Memory - memory used to transfer data to the GPU - used in
    different paths than mapped memory. Also backed by Shared Memory.

### Discardable Category

Cached images make use of Discardable memory. These allocations are managed by
Skia and a better summary of these allocations can likely be found in the Skia
category.

### Malloc Category

The malloc category shows a summary of all memory allocated through malloc.

Currently the information here is not granular enough to be useful, and a
good project would be to track down and instrument any large pools of memory
using malloc.

Some Skia caches also make use of malloc memory. For these allocations, a better
summary can be seen in the Skia category.

### Skia Category

The Skia category shows all resources used by the Skia rendering system. These
can be divided into a few subcategories. skia/gpu_resources/* includes all
resources using GPU memory. All other categories draw from either Shared or
malloc memory. To determine which type of memory a resource is using, hover
over the green arrow next to its size. This will show the other allocations
which the resource is aliased with.

## Other Areas of Interest

Many of the allocations under CC are aliased with memory in the Browser or GPU
process. When investigating CC memory it may be worth looking at the following
external categories:

1.  GPU Process / GPU Category - All GPU resources allocated by CC have a
    counterpart in the GPU/GPU category. This includes GL Textures, buffers, and
    other GPU backed objects such as Native GPU Memory Buffers.
2.  Browser Process / GpuMemoryBuffer Category - Resources backed by Shared
    Memory GpuMemoryBuffers are allocated by the browser and will be tracked
    in this category.
3.  Browser Process / SharedMemory Category - Resources backed by Bitmap and
    Shared Memory GpuMemoryBuffer objects are allocated by the browser and will
    also tracked in this category.

## Memory TODOs

The following areas have insufficient memory instrumentation.

1.  DisplayLists - DisplayLists can be quite large and are currently
    un-instrumented. These use malloc memory and currently contribute to
    malloc/allocated_objects/<unspecified>. [BUG](https://crbug.com/567465)
