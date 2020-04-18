# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class PrintSymbolTest(gdb_test.GdbTest):

  def test_print_symbol(self):
    self.gdb.Command('break set_global_var')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.assertEquals(self.gdb.Eval('global_var'), '2')
    self.assertEquals(self.gdb.Eval('arg'), '1')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    self.assertEquals(self.gdb.Eval('global_var'), '1')
    self.assertEquals(self.gdb.Eval('local_var'), '3')


if __name__ == '__main__':
  gdb_test.Main()
