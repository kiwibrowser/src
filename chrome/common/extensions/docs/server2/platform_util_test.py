#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from platform_util import (GetPlatforms,
                           GetExtensionTypes,
                           ExtractPlatformFromURL,
                           PluralToSingular,
                           PlatformToExtensionType)


class PlatformBundleUtilityTest(unittest.TestCase):
  def testGetPlatforms(self):
    self.assertEqual(('apps', 'extensions'), GetPlatforms())

  def testGetExtensionTypes(self):
    self.assertEqual(('platform_app', 'extension'), GetExtensionTypes())

  def testExtractPlatformFromURL(self):
    self.assertEqual('apps', ExtractPlatformFromURL('apps/something'))
    self.assertEqual('apps', ExtractPlatformFromURL('apps'))
    self.assertEqual('extensions', ExtractPlatformFromURL('extensions/a/b'))
    self.assertTrue(ExtractPlatformFromURL('a/b') is None)
    self.assertTrue(ExtractPlatformFromURL('app') is None)

  def testPluralToSingular(self):
    self.assertEqual('app', PluralToSingular('apps'))
    self.assertEqual('extension', PluralToSingular('extensions'))
    self.assertRaises(AssertionError, PluralToSingular, 'ab')

  def testPlatformToExtensionType(self):
    self.assertEqual('platform_app', PlatformToExtensionType('apps'))
    self.assertEqual('extension', PlatformToExtensionType('extensions'))
    self.assertRaises(AssertionError, PlatformToExtensionType, 'ab')

if __name__ == '__main__':
  unittest.main()
