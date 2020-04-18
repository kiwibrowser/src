#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import PRESUBMIT

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))))))
from PRESUBMIT_test_mocks import MockFile, MockInputApi, MockOutputApi


class ColorFormatTest(unittest.TestCase):

  def testColorFormatIgnoredFile(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color2">#FFFFFF</color>',
             '<color name="color3">#CCC</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.java', lines)]
    errors = PRESUBMIT._CheckColorFormat(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(errors))

  def testColorFormatTooShort(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color2">#FFFFFF</color>',
             '<color name="color3">#CCC</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorFormat(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(1, len(errors[0].items))
    self.assertEqual('  chrome/path/test.xml:3',
                     errors[0].items[0].splitlines()[0])

  def testColorInvalidAlphaValue(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color2">#FEFFFFFF</color>',
             '<color name="color3">#FFCCCCCC</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorFormat(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(1, len(errors[0].items))
    self.assertEqual('  chrome/path/test.xml:3',
                     errors[0].items[0].splitlines()[0])

  def testColorFormatLowerCase(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color2">#EFFFFFFF</color>',
             '<color name="color3">#CcCcCC</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorFormat(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(1, len(errors[0].items))
    self.assertEqual('  chrome/path/test.xml:3',
                     errors[0].items[0].splitlines()[0])


class ColorReferencesTest(unittest.TestCase):

  def testVectorDrawbleIgnored(self):
    lines = ['<vector',
             'tools:targetApi="21"',
             'android:fillColor="#CCCCCC">',
             '</vector>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorReferences(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(errors))

  def testInvalidReference(self):
    lines = ['<TextView',
             'android:textColor="#FFFFFF" />']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorReferences(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(1, len(errors[0].items))
    self.assertEqual('  chrome/path/test.xml:2',
                     errors[0].items[0].splitlines()[0])

  def testValidReference(self):
    lines = ['<TextView',
             'android:textColor="@color/color1" />']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/test.xml', lines)]
    errors = PRESUBMIT._CheckColorReferences(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(errors))

  def testValidReferenceInColorResources(self):
    lines = ['<color name="color1">#61000000</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/colors.xml', lines)]
    errors = PRESUBMIT._CheckColorReferences(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(errors))


class DuplicateColorsTest(unittest.TestCase):

  def testFailure(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color2">#61000000</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/colors.xml', lines)]
    errors = PRESUBMIT._CheckDuplicateColors(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertEqual(2, len(errors[0].items))
    self.assertEqual('  chrome/path/colors.xml:1',
                     errors[0].items[0].splitlines()[0])
    self.assertEqual('  chrome/path/colors.xml:2',
                     errors[0].items[1].splitlines()[0])

  def testSucess(self):
    lines = ['<color name="color1">#61000000</color>',
             '<color name="color1">#FFFFFF</color>']
    mock_input_api = MockInputApi()
    mock_input_api.files = [MockFile('chrome/path/colors.xml', lines)]
    errors = PRESUBMIT._CheckDuplicateColors(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(errors))


if __name__ == '__main__':
  unittest.main()
