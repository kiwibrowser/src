# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
The test scenario is as follows:
VA space: |0     |4k    |8k    |12k   |16k   |20k  ... |64k      |65k      |66k
Mmaps:    [    anon 1  ][anon 2]      [anon 3]     ... [  exe1  ][  exe2  ]
Resident: *******-------*******-------*******      (*:resident, -:not resident)
Allocs:     <1> <2>   <         3       >
             |   |              |
S.Traces:    |   |              +-----------> st1[exe1 + 0, exe1 + 4]
             |   +--------------------------> st1[exe1 + 0, exe1 + 4]
             +------------------------------> st2[exe1 + 0, exe2 + 4, post-exe2]

Furthermore, the exe2 is a file mapping with non-zero (8k) offset.
"""

import unittest

from memory_inspector.core import memory_map
from memory_inspector.core import native_heap
from memory_inspector.core import stacktrace
from memory_inspector.core import symbol

from memory_inspector.core.memory_map import PAGE_SIZE


class NativeHeapTest(unittest.TestCase):
  def runTest(self):
    nheap = native_heap.NativeHeap()

    EXE_1_MM_BASE = 64 * PAGE_SIZE
    EXE_2_MM_BASE = 65 * PAGE_SIZE
    EXE_2_FILE_OFF = 8192
    st1 = stacktrace.Stacktrace()
    st1.Add(nheap.GetStackFrame(EXE_1_MM_BASE))
    st1.Add(nheap.GetStackFrame(EXE_1_MM_BASE + 4))

    st2 = stacktrace.Stacktrace()
    st2.Add(nheap.GetStackFrame(EXE_1_MM_BASE))
    st2.Add(nheap.GetStackFrame(EXE_2_MM_BASE + 4))
    st2.Add(nheap.GetStackFrame(EXE_2_MM_BASE + PAGE_SIZE + 4))

    # Check that GetStackFrames keeps one unique object instance per address.
    # This is to guarantee that the symbolization logic (SymbolizeUsingSymbolDB)
    # can cheaply iterate on distinct stack frames rather than re-processing
    # every stack frame for each allocation (and save memory as well).
    self.assertIs(st1[0], st2[0])
    self.assertIsNot(st1[0], st1[1])
    self.assertIsNot(st2[0], st2[1])

    alloc1 = native_heap.Allocation(start=4, size=4, stack_trace=st1)
    alloc2 = native_heap.Allocation(start=4090, size=8, stack_trace=st1)
    alloc3 = native_heap.Allocation(start=8190, size=10000, stack_trace=st2)
    nheap.Add(alloc1)
    nheap.Add(alloc2)
    nheap.Add(alloc3)

    self.assertEqual(len(nheap.allocations), 3)
    self.assertIn(alloc1, nheap.allocations)
    self.assertIn(alloc2, nheap.allocations)
    self.assertIn(alloc3, nheap.allocations)

    ############################################################################
    # Test the relativization (absolute address -> mmap + offset) logic.
    ############################################################################
    mmap = memory_map
    mmap = memory_map.Map()
    mmap.Add(memory_map.MapEntry(EXE_1_MM_BASE, EXE_1_MM_BASE + PAGE_SIZE - 1,
        'rw--', '/d/exe1', 0))
    mmap.Add(memory_map.MapEntry(EXE_2_MM_BASE, EXE_2_MM_BASE + PAGE_SIZE - 1,
        'rw--', 'exe2',EXE_2_FILE_OFF))
    # Entry for EXE_3 is deliberately missing to check the fallback behavior.

    nheap.RelativizeStackFrames(mmap)

    self.assertEqual(st1[0].exec_file_rel_path, '/d/exe1')
    self.assertEqual(st1[0].exec_file_name, 'exe1')
    self.assertEqual(st1[0].offset, 0)

    self.assertEqual(st1[1].exec_file_rel_path, '/d/exe1')
    self.assertEqual(st1[1].exec_file_name, 'exe1')
    self.assertEqual(st1[1].offset, 4)

    self.assertEqual(st2[0].exec_file_rel_path, '/d/exe1')
    self.assertEqual(st2[0].exec_file_name, 'exe1')
    self.assertEqual(st2[0].offset, 0)

    self.assertEqual(st2[1].exec_file_rel_path, 'exe2')
    self.assertEqual(st2[1].exec_file_name, 'exe2')
    self.assertEqual(st2[1].offset, 4 + EXE_2_FILE_OFF)

    self.assertIsNone(st2[2].exec_file_rel_path)
    self.assertIsNone(st2[2].exec_file_name)
    self.assertIsNone(st2[2].offset)

    ############################################################################
    # Test the symbolization logic.
    ############################################################################
    syms = symbol.Symbols()
    syms.Add('/d/exe1', 0, symbol.Symbol('sym1', 'src1.c', 1))  # st1[0]
    syms.Add('exe2', 4 + EXE_2_FILE_OFF, symbol.Symbol('sym3'))  # st2[1]

    nheap.SymbolizeUsingSymbolDB(syms)
    self.assertEqual(st1[0].symbol.name, 'sym1')
    self.assertEqual(st1[0].symbol.source_info[0].source_file_path, 'src1.c')
    self.assertEqual(st1[0].symbol.source_info[0].line_number, 1)

    # st1[1] should have no symbol info, because we didn't provide any above.
    self.assertIsNone(st1[1].symbol)

    # st2[0] and st1[0] were the same Frame. Expect identical symbols instances.
    self.assertIs(st2[0].symbol, st1[0].symbol)

    # st2[1] should have a symbols name, but no source line info.
    self.assertEqual(st2[1].symbol.name, 'sym3')
    self.assertEqual(len(st2[1].symbol.source_info), 0)

    # st2[2] should have no sym because we didn't even provide a mmap for exe3.
    self.assertIsNone(st2[2].symbol)

    ############################################################################
    # Test the resident size calculation logic (intersects mmaps and allocs).
    ############################################################################
    mmap.Add(
        memory_map.MapEntry(0, 8191, 'rw--', '', 0, resident_pages=[1]))
    mmap.Add(
        memory_map.MapEntry(8192, 12287, 'rw--', '', 0, resident_pages=[1]))
    # [12k, 16k] is deliberately missing to check the fallback behavior.
    mmap.Add(
        memory_map.MapEntry(16384, 20479, 'rw--', '', 0, resident_pages=[1]))
    nheap.CalculateResidentSize(mmap)

    # alloc1 [4, 8] is fully resident because it lays in the first resident 4k.
    self.assertEqual(alloc1.resident_size, 4)

    # alloc2 [4090, 4098] should have only 6 resident bytes ([4090,4096]), but
    # not the last two, which lay on the second page which is noijt resident.
    self.assertEqual(alloc2.resident_size, 6)

    # alloc3 [8190, 18190] is split as follows (* = resident):
    #  [8190, 8192]: these 2 bytes are NOT resident, they lay in the 2nd page.
    # *[8192, 12288]: the 3rd page is resident and is fully covered by alloc3.
    #  [12288, 16384]: the 4th page is fully covered as well, but not resident.
    # *[16384, 18190]: the 5th page is partially covered and resident.
    self.assertEqual(alloc3.resident_size, (12288 - 8192) + (18190 - 16384))
