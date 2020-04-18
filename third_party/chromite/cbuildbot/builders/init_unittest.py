# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for main builder logic (__init__.py)."""

from __future__ import print_function

import mock

from chromite.cbuildbot import builders
from chromite.cbuildbot.builders import simple_builders
from chromite.lib import cros_import
from chromite.lib import cros_test_lib


class ModuleTest(cros_test_lib.MockTempDirTestCase):
  """Module loading related tests"""

  def testGetBuilderClass(self):
    """Check behavior when requesting a valid builder."""
    result = builders.GetBuilderClass('simple_builders.SimpleBuilder')
    self.assertEqual(result, simple_builders.SimpleBuilder)

  def testGetBuilderClassError(self):
    """Check behavior when requesting missing builders."""
    self.assertRaises(ValueError, builders.GetBuilderClass, 'Foalksdjo')
    self.assertRaises(ImportError, builders.GetBuilderClass, 'foo.Foalksdjo')
    self.assertRaises(AttributeError, builders.GetBuilderClass,
                      'release_builders.Foalksdjo')

  def testGetBuilderClassConfig(self):
    """Check behavior when requesting config builders.

    This can't be done with live classes since the site config may or may not
    be there.
    """
    # Setup
    mock_module = mock.Mock()
    mock_module.MyBuilder = 'fake_class'
    mock_import = self.PatchObject(cros_import, 'ImportModule',
                                   return_value=mock_module)
    # Test
    result = builders.GetBuilderClass('config.my_builders.MyBuilder')
    # Verify
    mock_import.assert_called_once_with('chromite.config.my_builders')
    self.assertEqual(result, 'fake_class')

    # Test again with a nested builder class name.
    mock_import.reset_mock()

    # Test
    result = builders.GetBuilderClass('config.nested.my_builders.MyBuilder')
    # Verify
    mock_import.assert_called_once_with('chromite.config.nested.my_builders')
    self.assertEqual(result, 'fake_class')
