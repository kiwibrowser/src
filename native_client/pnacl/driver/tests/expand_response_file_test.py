#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that @file (response files) are parsed as a command shell
would parse them (stripping quotes when necessary, etc.)
"""

import driver_test_utils
import driver_tools

import os
import unittest

class TestExpandResponseFile(driver_test_utils.DriverTesterCommon):
  def test_ShouldExpandCommandFile(self):
    """Test that response files are detected in commandline parser. """
    # NOTE: This is currently not just syntactic.  We currently require
    # that the file exist so we use a tempfile.
    t = self.getTemp()
    self.assertTrue(driver_tools.ShouldExpandCommandFile(
      '@' + t.name))
    self.assertTrue(driver_tools.ShouldExpandCommandFile(
      '@' + os.path.abspath(t.name)))
    # Test that no space can be between @ and file.
    self.assertFalse(driver_tools.ShouldExpandCommandFile(
      '@ ' + t.name))
    self.assertFalse(driver_tools.ShouldExpandCommandFile(
      t.name))
    # Testing that it's not just syntactic (file must exist).
    self.assertFalse(driver_tools.ShouldExpandCommandFile(
      '@some_truly_non_existent_file'))

  def CheckResponseFileWithQuotedFile(self, file_to_quote):
    """Test using a response file with quotes around the filename.

    We make sure that the quotes are stripped so that we do not
    attempt to open a file with quotes in its name.
    """
    t = self.getTemp(close=False)
    t.write('-E "%s" -I.. -o out.o\n' % file_to_quote)
    # Close to flush and ensure file is reopenable on windows.
    t.close()
    pre_argv = ['early_arg.c', '@' + t.name, 'later_arg.c']
    response_pos = 1
    argv = driver_tools.DoExpandCommandFile(pre_argv, response_pos)
    self.assertEqual(argv,
                     ['early_arg.c',
                      '-E', file_to_quote, '-I..', '-o', 'out.o',
                      'later_arg.c'])

  def test_FileWithQuotesWinBackSlash(self):
    self.CheckResponseFileWithQuotedFile('C:\\tmp\\myfile.c')

  def test_FileWithQuotesWinFwdSlash(self):
    self.CheckResponseFileWithQuotedFile('C:/tmp/myfile.c')

  def test_FileWithQuotesPosix(self):
    self.CheckResponseFileWithQuotedFile('/tmp/myfile.c')

  def test_FileWithQuotesWithSpace(self):
    self.CheckResponseFileWithQuotedFile('file with space.c')

  def test_EmptyFile(self):
    t = self.getTemp()
    pre_argv = ['early_arg.c', '@' + t.name, '-S']
    argv = driver_tools.DoExpandCommandFile(pre_argv, 1)
    self.assertEqual(argv,
                     ['early_arg.c', '-S'])

  def test_MultiLineNoContinuationChar(self):
    # Response files can span multiple lines and do not
    # require a line continuation char like '\' or '^'.
    t = self.getTemp(close=False)
    t.write('f.c\n')
    t.write(' -I.. \n')
    t.write(' -c\n')
    t.write('-o f.o  ')
    t.close()
    argv = driver_tools.DoExpandCommandFile(['@' + t.name], 0)
    self.assertEqual(argv, ['f.c', '-I..', '-c', '-o', 'f.o'])

  # TODO(jvoung): Test commandlines with multiple response files
  # and recursive response files.  This requires refactoring
  # the argument parsing to make it more testable (have a
  # version that does not modify the global env).


if __name__ == '__main__':
  unittest.main()
