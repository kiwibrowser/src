# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for json_lib."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib import json_lib
from chromite.lib import osutils


class JsonHelpersTest(cros_test_lib.MockTestCase):
  """Tests for chromite.lib.json_lib."""

  def testAssertIsInstance(self):
    """Test that AssertIsInstance is correct."""
    self.assertRaises(ValueError, json_lib.AssertIsInstance,
                      tuple(), list, 'a bad value')
    self.assertRaises(ValueError, json_lib.AssertIsInstance,
                      1, float, 'a bad value')
    self.assertRaises(ValueError, json_lib.AssertIsInstance,
                      1, bool, 'a bad value')
    json_lib.AssertIsInstance([1], list, 'good value')
    json_lib.AssertIsInstance(True, bool, 'good value')
    json_lib.AssertIsInstance({'foo': 2}, dict, 'good value')

  def testGetValueOfType(self):
    """Test that GetValueOfType is correct."""
    self.assertRaises(
        ValueError, json_lib.GetValueOfType,
        {}, 'missing key', str, 'missing value')
    self.assertRaises(
        ValueError, json_lib.GetValueOfType,
        {'key': 1}, 'key', bool, 'bad type')
    self.assertRaises(
        ValueError, json_lib.GetValueOfType,
        {'key': [1]}, 'key', int, 'bad type')
    self.assertEqual(
        json_lib.GetValueOfType({'key': 1}, 'key', int, 'good value'),
        1)

  def testPopValueOfType(self):
    """Test that PopValueOfType is correct."""
    input_dict = {'key': 'value'}
    self.assertEqual(
        'value',
        json_lib.GetValueOfType(input_dict, 'key', str, 'value'))
    self.assertEqual(
        'value',
        json_lib.PopValueOfType(input_dict, 'key', str, 'value'))
    self.assertFalse(input_dict)

  def testParseJsonFileWithComments(self):
    """Test that we can parse a JSON file with comments."""
    JSON_WITH_COMMENTS = """
        {
        # I am a comment.
        "foo": []
        }
        """
    self.PatchObject(osutils, 'ReadFile', return_value=JSON_WITH_COMMENTS)
    self.assertEqual({u'foo': []},
                     json_lib.ParseJsonFileWithComments('fake path'))
    self.PatchObject(osutils, 'ReadFile', return_value='')
    self.assertRaises(ValueError,
                      json_lib.ParseJsonFileWithComments,
                      'fake path')

  def testGetNestedDictValue(self):
    """Test that GetNestedDictValue is correct."""
    NESTED_DICT = {
        'lv1.1': {
            'lv2.1': {'value': '1.1'},
            'lv2.2': {'value': '1.2'}},
        'lv1.2': {
            'lv2.1': {'value': '2.1'},
            'lv2.2': {'value': '2.2'}}}
    self.assertEqual(
        '1.1',
        json_lib.GetNestedDictValue(NESTED_DICT, ['lv1.1', 'lv2.1', 'value']))
    self.assertEqual(
        '2.1',
        json_lib.GetNestedDictValue(NESTED_DICT, ['lv1.2', 'lv2.1', 'value']))
    # level 2 key missing
    self.assertIsNone(
        json_lib.GetNestedDictValue(NESTED_DICT, ['lv1.1', 'lv2.3', 'value']))
    # level 1 key missing
    self.assertIsNone(
        json_lib.GetNestedDictValue(NESTED_DICT, ['lv1.3', 'lv2.1', 'value']))
    # no level 3 key
    self.assertIsNone(
        json_lib.GetNestedDictValue(NESTED_DICT,
                                    ['lv1.1', 'lv2.1', 'lv3', 'value']))

    # no level 4
    self.assertIsNone(
        json_lib.GetNestedDictValue(NESTED_DICT,
                                    ['lv1.1', 'lv2.1', 'value', '1.1']))
