# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class StepIAfterBreakTest(gdb_test.GdbTest):

  def test_stepi_after_break(self):
    self.gdb.Command('break test_stepi_after_break')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    # From GDB/MI documentation, 'stepi' statement should be
    #   gdb.ResumeAndExpectStop('stepi', 'end-stepping-range')
    # but in reality 'stepi' stop reason is simply omitted.
    self.gdb.ResumeCommand('stepi')


if __name__ == '__main__':
  gdb_test.Main()
