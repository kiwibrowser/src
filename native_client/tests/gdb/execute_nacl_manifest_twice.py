# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class ExecuteNaClManifestTwiceTest(gdb_test.GdbTest):

  def tearDown(self):
    # sel_ldr exits first because the test program runs to completion.
    # Intentionally bypass superclass's tearDown as it assumes gdb exits first.
    self.AssertSelLdrExits(expected_returncode=1)
    self.gdb.Quit()
    self.gdb.Wait()

  def test_execute_nacl_manifest_twice(self):
    # The second superfluous call to LoadManifestFile.  This is a regression
    # test for issue:
    #   https://code.google.com/p/nativeclient/issues/detail?id=3262 .
    self.gdb.LoadManifestFile()
    self.gdb.ResumeCommand('continue')


if __name__ == '__main__':
  gdb_test.Main()
