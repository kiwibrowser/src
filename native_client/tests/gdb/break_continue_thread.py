# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class BreakContinueThreadTest(gdb_test.GdbTest):

  def test_break_continue_thread(self):
    self.gdb.Command('break foo')
    self.gdb.Command('break bar')
    # Program runs 2 threads, each calls foo and bar - expect 4 breakpoint hits.
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')


if __name__ == '__main__':
  gdb_test.Main()
