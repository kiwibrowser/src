# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class ChangeVariableTest(gdb_test.GdbTest):

  def test_change_variable(self):
    self.gdb.Command('break test_change_variable')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.Command('set var arg = 2')
    self.assertEquals(self.gdb.Eval('arg'), '2')
    self.gdb.ResumeCommand('step')
    self.assertEquals(self.gdb.Eval('local_var'), '4')
    self.gdb.Command('set var local_var = 5')
    self.assertEquals(self.gdb.Eval('local_var'), '5')
    self.gdb.Command('set var global_var = 1')
    self.assertEquals(self.gdb.Eval('global_var'), '1')
    self.gdb.ResumeCommand('step')
    self.assertEquals(self.gdb.Eval('global_var'), '8') # 1 + 5 + 2


if __name__ == '__main__':
  gdb_test.Main()
