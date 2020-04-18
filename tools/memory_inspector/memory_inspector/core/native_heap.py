# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from memory_inspector.core import memory_map
from memory_inspector.core import stacktrace
from memory_inspector.core import symbol

from memory_inspector.core.memory_map import PAGE_SIZE


class NativeHeap(object):
  """A snapshot of outstanding (i.e. not freed) native allocations.

  This is typically obtained by calling |backends.Process|.DumpNativeHeap()
  """

  def __init__(self):
    self.allocations = []  # list of individual |Allocation|s.
    self.stack_frames = {}  # absolute_address (int) -> |stacktrace.Frame|.

  def Add(self, allocation):
    assert(isinstance(allocation, Allocation))
    self.allocations += [allocation]

  def GetStackFrame(self, absolute_addr):
    """Guarantees that multiple calls with the same addr return the same obj."""
    assert(isinstance(absolute_addr, (long, int)))
    stack_frame = self.stack_frames.get(absolute_addr)
    if not stack_frame:
      stack_frame = stacktrace.Frame(absolute_addr)
      self.stack_frames[absolute_addr] = stack_frame
    return stack_frame

  def SymbolizeUsingSymbolDB(self, symbols):
    assert(isinstance(symbols, symbol.Symbols))
    for stack_frame in self.stack_frames.itervalues():
      if not stack_frame.exec_file_rel_path:
        continue
      sym = symbols.Lookup(stack_frame.exec_file_rel_path, stack_frame.offset)
      if sym:
        stack_frame.SetSymbolInfo(sym)

  def RelativizeStackFrames(self, mmap):
    """Turns stack frames' absolute addresses into mmap relative addresses.

    For each absolute address, the containing mmap is looked up and the frame
    is decorated with the mapped file + relative address in the file."""
    assert(isinstance(mmap, memory_map.Map))
    for abs_addr, stack_frame in self.stack_frames.iteritems():
      assert(abs_addr == stack_frame.address)
      map_entry = mmap.Lookup(abs_addr)
      if not map_entry:
        continue
      stack_frame.SetExecFileInfo(map_entry.mapped_file,
                                  map_entry.GetRelativeFileOffset(abs_addr))

  def CalculateResidentSize(self, mmap):
    """Updates the |Allocation|.|resident_size|s by looking at mmap stats.

    Not all the allocated memory is always used (read: resident). This function
    estimates the resident size of an allocation intersecting the mmaps dump.
    """
    assert(isinstance(mmap, memory_map.Map))
    for alloc in self.allocations:
      # This function loops over all the memory pages that intersect, partially
      # or fully, with each allocation. For each of them, the allocation  is
      # attributed a resident size equal to the size of intersecting range iff
      # the page is resident.
      # The tricky part is that, in the general case, an allocation can span
      # over multiple (contiguous) mmaps. See the chart below for a reference:
      #
      # VA space:  |0    |4k   |8k   |12k  |16k  |20k  |24k  |28k  |32k  |
      # Mmaps:     [   mm 1   ][ mm2 ]           [          map 3        ]
      # Allocs:      <a1>  <  a2  >                       <      a3      >
      #
      # Note: this accounting technique is not fully correct but is generally a
      # good tradeoff between accuracy and speed of profiling. The OS provides
      # resident information with the page granularity (typ. 4k). Finer values
      # would require more fancy techniques based, for instance, on run-time
      # instrumentation tools like Valgrind or *sanitizer.
      cur_start = alloc.start
      mm = None
      while cur_start < alloc.end:
        if not mm or not mm.Contains(cur_start):
          mm = mmap.Lookup(cur_start)
        if mm:
          page, page_off = mm.GetRelativeMMOffset(cur_start)
          if mm.IsPageResident(page):
            page_end = mm.start + page * PAGE_SIZE + PAGE_SIZE - 1
            alloc_memory_in_current_page = PAGE_SIZE - page_off
            if alloc.end < page_end:
              alloc_memory_in_current_page -= page_end - alloc.end
            alloc.resident_size += alloc_memory_in_current_page
        # Move to the next page boundary.
        cur_start = (cur_start + PAGE_SIZE) & ~(PAGE_SIZE - 1)


class Allocation(object):
  """Records profiling information about a native heap allocation.

  Args:
      size: size of the allocation, in bytes.
      stack_trace: the allocation call-site. See |stacktrace.Stacktrace|.
      start: (Optional) Absolute start address in the process VMA. It is
          required only for |CalculateResidentSize|.
      flags: (Optional) More details about the call site (e.g., mmap vs malloc).
      resident_size: this is normally obtained through |CalculateResidentSize|
          and is part of the initializer just for deserialization purposes.
  """

  def __init__(self, size, stack_trace, start=0, flags=0, resident_size=0):
    assert(size > 0)
    assert(isinstance(stack_trace, stacktrace.Stacktrace))
    self.size = size  # in bytes.
    self.stack_trace = stack_trace
    self.start = start  # Optional, for using the resident size logic.
    self.flags = flags
    self.resident_size = resident_size  #  see |CalculateResidentSize|.

  @property
  def end(self):
    return self.start + self.size - 1

  def __str__(self):
    return '%d : %s' % (self.size, self.stack_trace)
