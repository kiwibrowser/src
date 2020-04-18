# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for virtualenv_wrapper"""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.scripts import virtualenv_wrapper

_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))


class VirtualEnvTest(cros_test_lib.TestCase):
  """Test that we are running in a virtualenv."""

  # pylint: disable=protected-access


  def testModuleIsFromVenv(self):
    """Test that we import |six| from the virtualenv."""
    # Note: The |six| module is chosen somewhat arbitrarily, but it happens to
    # be provided inside the chromite virtualenv.
    six = __import__('six')
    req_path = os.path.dirname(os.path.realpath(six.__file__))
    self.assertIn('/.cache/cros_venv/', req_path)


  def testInsideVenv(self):
    """Test that we are inside a virtualenv."""
    # pylint: disable=protected-access
    self.assertTrue(virtualenv_wrapper._IsInsideVenv(os.environ))


  def testVenvMarkers(self):
    """Test that the virtualenv marker functions work."""
    # pylint: disable=protected-access
    test_env = {'PATH': '/bin:/usr/bin'}
    self.assertFalse(virtualenv_wrapper._IsInsideVenv(test_env))
    new_test_env = virtualenv_wrapper._CreateVenvEnvironment(test_env)
    self.assertTrue(virtualenv_wrapper._IsInsideVenv(new_test_env))


  def testCreateVenvEnvironmentNoSideEffect(self):
    """Test that _CreateVenvEnvironment doesn't modify input dict."""
    # pylint: disable=protected-access
    test_env = {'PATH': '/bin:/usr/bin'}
    original = test_env.copy()
    virtualenv_wrapper._CreateVenvEnvironment(test_env)
    self.assertEqual(test_env, original)
