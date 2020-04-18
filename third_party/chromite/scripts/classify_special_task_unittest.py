# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for classify_special_task."""

from __future__ import print_function
import mock

from chromite.lib import cros_test_lib
from chromite.scripts import classify_special_task as cst


class TraverseTreeTest(cros_test_lib.MockTestCase):
  """Test that TraverseTree behaves correctly."""
  _BASIC_JSON = {
      'keyA': 'keyAvalue',
      'keyB': ['keyB0value', 'keyB1value', 'keyB2value',],
      'keyC': {
          'keyCA': 'keyCAvalue',
          'keyCB': ['keyCB0value', 'keyCB1value', 'keyCB2value',],
          'keyCC': [
              {'keyCCA': 'keyCC0Avalue',},
              {'keyCCA': 'keyCC1Avalue',},
              {'keyCCA-missing': 'keyCC2Avalue',},
              {'keyCCA': 'keyCC3Avalue',},
          ],
      },
  }

  def setUp(self):
    self.mock_func = mock.MagicMock()
    self.mock_logging = self.PatchObject(cst.logging, 'error')

  def assertFuncCalls(self, *args):
    calls = [mock.call(arg) for arg in args]
    self.mock_func.assert_has_calls(calls)

  def testScalar(self):
    """Test traversing directly to a scalar."""
    cst.TraverseTree(self._BASIC_JSON, ['keyA'], self.mock_func)
    self.assertFuncCalls('keyAvalue')
    self.mock_logging.assert_not_called()

  def testList(self):
    """Test traversing a list."""
    cst.TraverseTree(self._BASIC_JSON, ['keyB'], self.mock_func)
    self.assertFuncCalls('keyB0value', 'keyB1value', 'keyB2value')
    self.mock_logging.assert_not_called()

  def testDict(self):
    """Test traversing a dictionary."""
    cst.TraverseTree(self._BASIC_JSON, ['keyC', 'keyCA'], self.mock_func)
    self.assertFuncCalls('keyCAvalue')
    self.mock_logging.assert_not_called()

  def testMultiple(self):
    """Test traversing a dictionary and list."""
    cst.TraverseTree(self._BASIC_JSON, ['keyC', 'keyCB'], self.mock_func)
    self.assertFuncCalls('keyCB0value', 'keyCB1value', 'keyCB2value')
    self.mock_logging.assert_not_called()

  def testMissing(self):
    """Test traversing a dictionary and list with missing values."""
    cst.TraverseTree(self._BASIC_JSON, ['keyC', 'keyCC', 'keyCCA'],
                     self.mock_func)
    self.assertFuncCalls('keyCC0Avalue', 'keyCC1Avalue', 'keyCC3Avalue')
    self.mock_logging.assert_not_called()

  def testNonDictRoot(self):
    """Test traversing to a key with a non-dict root."""
    cst.TraverseTree(self._BASIC_JSON, ['keyA', 'keyAA'], self.mock_func)
    self.mock_func.assert_not_called()
    self.assertEqual(self.mock_logging.call_count, 1)

  def testNonStringLeaf(self):
    """Test traversing to a non-string leaf."""
    cst.TraverseTree(self._BASIC_JSON, ['keyC'], self.mock_func)
    self.mock_func.assert_not_called()
    self.assertEqual(self.mock_logging.call_count, 1)
