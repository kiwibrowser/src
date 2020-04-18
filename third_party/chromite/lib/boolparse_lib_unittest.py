# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for boolparse_lib methods."""

from __future__ import print_function

import boolparse_lib
import cros_test_lib


class ParserTest(cros_test_lib.TestCase):
  """Unittest for boolean expression parser."""

  def testSingleItem(self):
    self.assertFalse(boolparse_lib.BoolstrResult('A', {}))
    self.assertTrue(boolparse_lib.BoolstrResult('A', {'A', 'B'}))
    self.assertFalse(boolparse_lib.BoolstrResult('A', {'B'}))

  def testAndlogic(self):
    self.assertFalse(boolparse_lib.BoolstrResult('A and B', {}))
    self.assertFalse(boolparse_lib.BoolstrResult('A and B', {}))
    self.assertTrue(boolparse_lib.BoolstrResult('A and B', {'A', 'B', 'C'}))

  def testOrlogic(self):
    self.assertFalse(boolparse_lib.BoolstrResult('A or B', {}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or B', {'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or B', {'A', 'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or B', {'A', 'C'}))
    self.assertFalse(boolparse_lib.BoolstrResult('A or B', {'C'}))

  def testNotlogic(self):
    self.assertTrue(boolparse_lib.BoolstrResult('not A', {}))
    self.assertFalse(boolparse_lib.BoolstrResult('not A', {'A'}))
    self.assertFalse(boolparse_lib.BoolstrResult('not A', {'A', 'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('not A', {'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('not not A', {'A'}))

  def testComplexBoolExpr(self):
    self.assertFalse(boolparse_lib.BoolstrResult('A and not B', {}))
    self.assertTrue(boolparse_lib.BoolstrResult('A and not B', {'A'}))
    self.assertFalse(boolparse_lib.BoolstrResult('A and not B', {'A', 'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('not (A and B)', {'A'}))
    self.assertFalse(boolparse_lib.BoolstrResult('not (A and B)', {'A', 'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or not B', {'A'}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or not B', {'A', 'B'}))
    self.assertFalse(boolparse_lib.BoolstrResult('A or not B', {'B'}))
    self.assertTrue(boolparse_lib.BoolstrResult('A or not B', {'C'}))

  def testInvalidBoolExprInput(self):
    """Test invalid boolean expression.

    Test whether captures the exceptions caused by invalid boolean expression
    input. Note that only the lowercase and, or, not are allowed.
    """

    with self.assertRaises(boolparse_lib.BoolParseError):
      boolparse_lib.BoolstrResult('', {'A'})
    with self.assertRaises(TypeError):
      boolparse_lib.BoolstrResult(None, {'A'})

  def testCollectionInput(self):
    """Test whether handle different types of collection input."""

    self.assertFalse(boolparse_lib.BoolstrResult('A', None))
    self.assertTrue(boolparse_lib.BoolstrResult('A', ['A', 'A']))
    self.assertFalse(boolparse_lib.BoolstrResult('not A', ('A', 'B', 'B')))

  def testVariousOperand(self):
    """Test on various operand format."""
    self.assertFalse(boolparse_lib.BoolstrResult('A:foo and B:foo', {'A:foo'}))
