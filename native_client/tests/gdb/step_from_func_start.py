# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class StepFromFuncStartTest(gdb_test.GdbTest):

  def test_step_from_func_start(self):
    self.gdb.Command('break test_step_from_function_start')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeCommand('step')
    self.gdb.ResumeCommand('step')
    self.gdb.ResumeCommand('step')
    self.assertEquals(self.gdb.Eval('global_var'), '1')


if __name__ == '__main__':
  gdb_test.Main()
