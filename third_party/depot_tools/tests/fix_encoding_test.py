#!/usr/bin/env python
# coding=utf8
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for fix_encoding.py."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import fix_encoding


class FixEncodingTest(unittest.TestCase):
  # Nice mix of latin, hebrew, arabic and chinese. Doesn't mean anything.
  text = u'Héllô 偉大 سيد'

  def test_code_page(self):
    # Make sure printing garbage won't throw.
    print self.text.encode() + '\xff'
    print >> sys.stderr, self.text.encode() + '\xff'

  def test_utf8(self):
    # Make sure printing utf-8 works.
    print self.text.encode('utf-8')
    print >> sys.stderr, self.text.encode('utf-8')

  def test_unicode(self):
    # Make sure printing unicode works.
    print self.text
    print >> sys.stderr, self.text

  def test_default_encoding(self):
    self.assertEquals('utf-8', sys.getdefaultencoding())

  def test_win_console(self):
    if sys.platform != 'win32':
      return
    # This should fail if not redirected, e.g. run directly instead of through
    # the presubmit check. Can be checked with:
    # python tests\fix_encoding_test.py
    self.assertEquals(
        sys.stdout.__class__, fix_encoding.WinUnicodeOutput)
    self.assertEquals(
        sys.stderr.__class__, fix_encoding.WinUnicodeOutput)
    self.assertEquals(sys.stdout.encoding, sys.getdefaultencoding())
    self.assertEquals(sys.stderr.encoding, sys.getdefaultencoding())

  def test_multiple_calls(self):
    # Shouldn't do anything.
    self.assertEquals(False, fix_encoding.fix_encoding())


if __name__ == '__main__':
  assert fix_encoding.fix_encoding()
  unittest.main()
