# -*- python -*-
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class DetachTest(gdb_test.GdbTest):

  def test_detach(self):
    # Test that you can stop on a breakpoint, then detach and have the debugged
    # program run to completion.
    self.gdb.Command('break test_detach')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    self.gdb.Detach()

  def tearDown(self):
    # sel_ldr exits first because the test program runs to completion.
    # Intentionally bypass superclass's tearDown as it assumes gdb exits first.
    self.AssertSelLdrExits(expected_returncode=0)
    self.gdb.Quit()
    self.gdb.Wait()


if __name__ == '__main__':
  gdb_test.Main()
