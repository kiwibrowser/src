The heap profiler records heap allocations, and exports heap dumps via Chrome
tracing.

The code is divided into three directories:

* components/services/heap_profiling/ contains the core logic that records
  allocations.
* components/heap_profiling/ [this directory] contains
  * Logic that hooks up processes to the heap_profiling service [depends on
    content/].
  * End to end tests [depends on content/].
* chrome/browser/profiling_host/ contains:
  * Logic that hooks up UI surfaces to the heap_profiling service.
  * Logic to automatically upload heap dumps in the wild.
