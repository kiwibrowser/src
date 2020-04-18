# -*- python -*-
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import gdb_test


class MmapTest(gdb_test.GdbTest):

  def setUp(self):
    os.environ['NACL_FAULT_INJECTION'] = (
        'MMAP_BYPASS_DESCRIPTOR_SAFETY_CHECK=GF/@')
    super(MmapTest, self).setUp()

  def test_mmap(self):
    self.gdb.Command('break mmap_breakpoint')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    # Check that we can read from memory mapped files.
    self.assertEquals(gdb_test.ParseNumber(self.gdb.Eval('*file_mapping')), 123)
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    file_mapping_str = self.gdb.Eval('file_mapping')
    file_mapping = gdb_test.ParseNumber(file_mapping_str)
    self.gdb.Command('break *' + file_mapping_str)
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    # Check that breakpoint in memory mapped code is working.
    self.assertEquals(self.gdb.GetPC(), file_mapping)
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    file_mapping_str = self.gdb.Eval('file_mapping')
    file_mapping = gdb_test.ParseNumber(file_mapping_str)
    self.gdb.Command('break *' + file_mapping_str)
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    # Check that breakpoint in memory mapped code is working.
    self.assertEquals(self.gdb.GetPC(), file_mapping)


if __name__ == '__main__':
  gdb_test.Main()
