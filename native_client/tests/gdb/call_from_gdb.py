# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class CallFromGdbTest(gdb_test.GdbTest):

  def test_call_from_gdb_test(self):
    self.gdb.Command('break main')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.assertEquals(self.gdb.Eval('test_call_from_gdb(1)'), '3')
    self.assertEquals(self.gdb.Eval('global_var'), '2')


if __name__ == '__main__':
  gdb_test.Main()
