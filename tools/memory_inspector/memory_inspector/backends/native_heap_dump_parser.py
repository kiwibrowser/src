# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This parser turns the heap_dump output into a |NativeHeap| object."""

import json

from memory_inspector.core import native_heap
from memory_inspector.core import stacktrace


# These are defined in heap_profiler/heap_profiler.h
FLAGS_MALLOC = 1
FLAGS_MMAP = 2
FLAGS_MMAP_FILE = 4
FLAGS_IN_ZYGOTE = 8


def Parse(content):
  """Parses the output of the heap_dump binary (part of libheap_profiler).

  heap_dump provides a conveniente JSON output.
  See the header of tools/android/heap_profiler/heap_dump.c for more details.

  Args:
      content: string containing the command output.

  Returns:
      An instance of |native_heap.NativeHeap|.
  """
  data = json.loads(content)
  assert('allocs' in data), 'Need to run heap_dump with the -x (extended) arg.'
  nativeheap = native_heap.NativeHeap()
  strace_by_index = {}   # index (str) -> |stacktrace.Stacktrace|

  for index, entry in data['stacks'].iteritems():
    strace = stacktrace.Stacktrace()
    for absolute_addr in entry['f']:
      strace.Add(nativeheap.GetStackFrame(absolute_addr))
    strace_by_index[index] = strace

  for start_addr, entry in data['allocs'].iteritems():
    flags = int(entry['f'])
    # TODO(primiano): For the moment we just skip completely the allocations
    # made in the Zygote (pre-fork) because this is usually reasonable. In the
    # near future we will expose them with some UI to selectively filter them.
    if flags & FLAGS_IN_ZYGOTE:
      continue
    nativeheap.Add(native_heap.Allocation(
      size=entry['l'],
      stack_trace=strace_by_index[entry['s']],
      start=int(start_addr, 16),
      flags=flags))

  return nativeheap
