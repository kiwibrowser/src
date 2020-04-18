# -*- python -*-
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class InvalidMemoryTest(gdb_test.GdbTest):

  def test_invalid_memory(self):
    self.gdb.ExpectToFailCommand('x 0x1000000')
    self.gdb.Command('break *0x1000000')
    # GDB sets breakpoints on "continue" and similar commands and removes them
    # when program stops, so it is continue command that we expect to fail.
    self.gdb.ExpectToFailCommand('continue')


if __name__ == '__main__':
  gdb_test.Main()
