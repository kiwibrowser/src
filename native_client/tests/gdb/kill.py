# -*- python -*-
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class KillTest(gdb_test.GdbTest):

  def test_kill(self):
    # Test that you can stop on a breakpoint, then kill the program being
    # debugged.
    self.gdb.Command('break test_kill')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.Kill()

  def tearDown(self):
    # Test program should end first with the kill return code.
    # Intentionally bypass superclass's tearDown as it assumes gdb exits first.
    self.AssertSelLdrExits()
    self.gdb.Quit()
    self.gdb.Wait()


if __name__ == '__main__':
  gdb_test.Main()
