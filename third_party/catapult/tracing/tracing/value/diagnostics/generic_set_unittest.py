# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from tracing.value.diagnostics import diagnostic
from tracing.value.diagnostics import generic_set


class GenericSetUnittest(unittest.TestCase):

  def testRoundtrip(self):
    a_set = generic_set.GenericSet([
        None,
        True,
        False,
        0,
        1,
        42,
        [],
        {},
        [0, False],
        {'a': 1, 'b': True},
    ])
    self.assertEqual(a_set, diagnostic.Diagnostic.FromDict(a_set.AsDict()))

  def testEq(self):
    a_set = generic_set.GenericSet([
        None,
        True,
        False,
        0,
        1,
        42,
        [],
        {},
        [0, False],
        {'a': 1, 'b': True},
    ])
    b_set = generic_set.GenericSet([
        {'b': True, 'a': 1},
        [0, False],
        {},
        [],
        42,
        1,
        0,
        False,
        True,
        None,
    ])
    self.assertEqual(a_set, b_set)

  def testMerge(self):
    a_set = generic_set.GenericSet([
        None,
        True,
        False,
        0,
        1,
        42,
        [],
        {},
        [0, False],
        {'a': 1, 'b': True},
    ])
    b_set = generic_set.GenericSet([
        {'b': True, 'a': 1},
        [0, False],
        {},
        [],
        42,
        1,
        0,
        False,
        True,
        None,
    ])
    self.assertTrue(a_set.CanAddDiagnostic(b_set))
    self.assertTrue(b_set.CanAddDiagnostic(a_set))
    a_set.AddDiagnostic(b_set)
    self.assertEqual(a_set, b_set)
    b_set.AddDiagnostic(a_set)
    self.assertEqual(a_set, b_set)

    c_dict = {'a': 1, 'b': 1}
    c_set = generic_set.GenericSet([c_dict])
    a_set.AddDiagnostic(c_set)
    self.assertEqual(len(a_set), 1 + len(b_set))
    self.assertIn(c_dict, a_set)

  def testGetOnlyElement(self):
    gs = generic_set.GenericSet(['foo'])
    self.assertEqual(gs.GetOnlyElement(), 'foo')

  def testGetOnlyElementRaises(self):
    gs = generic_set.GenericSet([])
    with self.assertRaises(AssertionError):
      gs.GetOnlyElement()
