# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the sysroot library."""

from __future__ import print_function

import os
import re

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import sysroot_lib
from chromite.lib import toolchain


class SysrootLibTest(cros_test_lib.MockTempDirTestCase):
  """Unittest for sysroot_lib.py"""

  def setUp(self):
    """Setup the test environment."""
    # Fake being root to avoid running all filesystem commands with
    # SudoRunCommand.
    self.PatchObject(os, 'getuid', return_value=0)

  def testGetStandardField(self):
    """Tests that standard field can be fetched correctly."""
    sysroot = sysroot_lib.Sysroot(self.tempdir)
    sysroot.WriteConfig('FOO="bar"')
    self.assertEqual('bar', sysroot.GetStandardField('FOO'))

    # Works with multiline strings
    multiline = """foo
bar
baz
"""
    sysroot.WriteConfig('TEST="%s"' % multiline)
    self.assertEqual(multiline, sysroot.GetStandardField('TEST'))

  def testReadWriteCache(self):
    """Tests that we can write and read to the cache."""
    sysroot = sysroot_lib.Sysroot(self.tempdir)

    # If a field is not defined we get None.
    self.assertEqual(None, sysroot.GetCachedField('foo'))

    # If we set a field, we can get it.
    sysroot.SetCachedField('foo', 'bar')
    self.assertEqual('bar', sysroot.GetCachedField('foo'))

    # Setting a field in an existing cache preserve the previous values.
    sysroot.SetCachedField('hello', 'bonjour')
    self.assertEqual('bar', sysroot.GetCachedField('foo'))
    self.assertEqual('bonjour', sysroot.GetCachedField('hello'))

    # Setting a field to None unsets it.
    sysroot.SetCachedField('hello', None)
    self.assertEqual(None, sysroot.GetCachedField('hello'))

  def testErrorOnBadCachedValue(self):
    """Tests that we detect bad value for the sysroot cache."""
    sysroot = sysroot_lib.Sysroot(self.tempdir)

    forbidden = [
        'hello"bonjour',
        'hello\\bonjour',
        'hello\nbonjour',
        'hello$bonjour',
        'hello`bonjour',
    ]
    for value in forbidden:
      with self.assertRaises(ValueError):
        sysroot.SetCachedField('FOO', value)

  def testProfileGeneration(self):
    """Tests that we generate the portage profile correctly."""
    # pylint: disable=protected-access
    overlay_dir = os.path.join(self.tempdir, 'overlays')
    sysroot_dir = os.path.join(self.tempdir, 'sysroot')
    overlays = [os.path.join(overlay_dir, letter) for letter in ('a', 'b', 'c')]
    for o in overlays:
      osutils.SafeMakedirs(o)
    sysroot = sysroot_lib.Sysroot(sysroot_dir)

    sysroot.WriteConfig(sysroot_lib._DictToKeyValue(
        {'ARCH': 'arm',
         'BOARD_OVERLAY': '\n'.join(overlays)}))

    sysroot._GenerateProfile()

    profile_link = os.path.join(sysroot.path, 'etc', 'portage', 'make.profile')
    profile_parent = osutils.ReadFile(
        os.path.join(profile_link, 'parent')).splitlines()
    self.assertTrue(os.path.islink(profile_link))
    self.assertEqual(1, len(profile_parent))
    self.assertTrue(re.match('chromiumos:.*arm.*', profile_parent[0]))

    profile_dir = os.path.join(overlays[1], 'profiles', 'base')
    osutils.SafeMakedirs(profile_dir)

    sysroot._GenerateProfile()
    profile_parent = osutils.ReadFile(
        os.path.join(profile_link, 'parent')).splitlines()
    self.assertEqual(2, len(profile_parent))
    self.assertEqual(profile_dir, profile_parent[1])

  def testGenerateConfigNoToolchainRaisesError(self):
    """Tests _GenerateConfig() with no toolchain raises an error."""
    self.PatchObject(toolchain, 'FilterToolchains', autospec=True,
                     return_value={})
    sysroot = sysroot_lib.Sysroot(self.tempdir)

    with self.assertRaises(sysroot_lib.ConfigurationError):
      # pylint: disable=protected-access
      sysroot._GenerateConfig({}, ['foo_overlay'], ['foo_overlay'], '')
