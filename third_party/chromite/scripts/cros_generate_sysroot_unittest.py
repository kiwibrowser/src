# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cros_generate_sysroot."""

from __future__ import print_function

import mock
import os

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.scripts import cros_generate_sysroot as cros_gen
from chromite.lib import osutils
from chromite.lib import partial_mock


Dir = cros_test_lib.Directory


class CrosGenMock(partial_mock.PartialMock):
  """Helper class to Mock out cros_generate_sysroot.GenerateSysroot."""

  TARGET = 'chromite.scripts.cros_generate_sysroot.GenerateSysroot'
  ATTRS = ('_InstallToolchain', '_InstallKernelHeaders',
           '_InstallBuildDependencies')

  TOOLCHAIN = 'toolchain'
  KERNEL_HEADERS = 'kernel_headers'
  BUILD_DEPS = 'build-deps'

  def _InstallToolchain(self, inst):
    osutils.Touch(os.path.join(inst.sysroot, self.TOOLCHAIN))

  def _InstallKernelHeaders(self, inst):
    osutils.Touch(os.path.join(inst.sysroot, self.KERNEL_HEADERS))

  def _InstallBuildDependencies(self, inst):
    osutils.Touch(os.path.join(inst.sysroot, self.BUILD_DEPS))

  def VerifyTarball(self, tarball):
    dir_struct = [Dir('.', []), self.TOOLCHAIN, self.KERNEL_HEADERS,
                  self.BUILD_DEPS]
    cros_test_lib.VerifyTarball(tarball, dir_struct)


BOARD = 'lumpy'
TAR_NAME = 'test.tar.xz'


class OverallTest(cros_test_lib.MockTempDirTestCase):
  """Tests for cros_generate_sysroot."""

  def setUp(self):
    self.cg_mock = self.StartPatcher(CrosGenMock())

  def testTarballGeneration(self):
    """End-to-end test of tarball generation."""
    with mock.patch.object(cros_build_lib, 'IsInsideChroot'):
      cros_build_lib.IsInsideChroot.returnvalue = True
      cros_gen.main(
          ['--board', BOARD, '--out-dir', self.tempdir,
           '--out-file', TAR_NAME, '--package', constants.CHROME_CP])
      self.cg_mock.VerifyTarball(os.path.join(self.tempdir, TAR_NAME))


class InterfaceTest(cros_test_lib.TempDirTestCase):
  """Test Parsing and error checking functionality."""

  BAD_TARGET_DIR = '/path/to/nowhere'

  def _Parse(self, extra_args):
    return cros_gen.ParseCommandLine(
        ['--board', BOARD, '--out-dir', self.tempdir,
         '--package', constants.CHROME_CP] + extra_args)

  def testDefaultTargetName(self):
    """We are getting the right default target name."""
    options = self._Parse([])
    self.assertEquals(
        options.out_file, 'sysroot_chromeos-base_chromeos-chrome.tar.xz')

  def testMultiplePkgsTargetName(self):
    """Test getting the right target name with multiple pkgs."""
    pkgs = "%s virtual/target-os" %constants.CHROME_CP
    options = cros_gen.ParseCommandLine(
        ['--board', BOARD, '--out-dir', self.tempdir,
         '--package', pkgs])

    self.assertEquals(
        options.out_file, 'sysroot_chromeos-base_chromeos-chrome.tar.xz')

  def testExistingTarget(self):
    """Erroring out on pre-existing target."""
    options = self._Parse(['--out-file', TAR_NAME])
    osutils.Touch(os.path.join(self.tempdir, TAR_NAME))
    self.assertRaises(cros_build_lib.DieSystemExit,
                      cros_gen.FinishParsing, options)

  def testNonExisting(self):
    """Erroring out on non-existent output dir."""
    options = cros_gen.ParseCommandLine(
        ['--board', BOARD, '--out-dir', self.BAD_TARGET_DIR, '--package',
         constants.CHROME_CP])
    self.assertRaises(cros_build_lib.DieSystemExit,
                      cros_gen.FinishParsing, options)
