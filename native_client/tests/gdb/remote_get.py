# -*- python -*-
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class RemoteGetTest(gdb_test.GdbTest):

  def test_remote_get_main_nexe(self):
    # Test that you can fetch the main nexe.
    fetched = self.gdb.FetchMainNexe()
    actual = self.gdb.GetMainNexe()
    fetched_content = open(fetched, 'rb').read()
    actual_content = open(actual, 'rb').read()
    self.assertEqual(fetched_content, actual_content)
    # Check the contents are of a reasonable size.
    self.assertGreater(len(fetched_content), 10000)

  def test_remote_get_irt_nexe(self):
    actual = self.gdb.GetIrtNexe()
    if actual is None:
      self.skipTest('Does not work in non-irt mode.')
    # Test that you can fetch the main nexe.
    fetched = self.gdb.FetchIrtNexe()
    fetched_content = open(fetched, 'rb').read()
    actual_content = open(actual, 'rb').read()
    self.assertEqual(fetched_content, actual_content)
    # Check the contents are of a reasonable size.
    self.assertGreater(len(fetched_content), 10000)


if __name__ == '__main__':
  gdb_test.Main()
