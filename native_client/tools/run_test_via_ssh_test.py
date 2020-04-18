#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import unittest

import run_test_via_ssh


class Test(unittest.TestCase):

  def test_escaping(self):
    # All ASCII characters.
    for ascii_value in range(1, 256):
      # Make sure to put a character after: $ behaves well when on its own,
      # but not so well when followed by something else.
      c = '--%s--' % chr(ascii_value)
      escaped = run_test_via_ssh.ShellEscape(c)
      result = subprocess.check_output('echo ' + escaped, shell=True)
      self.assertEquals(result, '%s\n' % c)


if __name__ == '__main__':
  unittest.main()
