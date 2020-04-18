# -*- python -*-
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class CompleteTest(gdb_test.GdbTest):

  def test_complete(self):
    # Test that continue causes the debugged program to run to completion.
    self.gdb.ResumeCommand('continue')

  def tearDown(self):
    # Test program should run to completion and return a special value.
    # Intentionally bypass superclass's tearDown as it assumes gdb exits first.
    self.AssertSelLdrExits(expected_returncode=123)
    self.gdb.Quit()
    self.gdb.Wait()


if __name__ == '__main__':
  gdb_test.Main()
