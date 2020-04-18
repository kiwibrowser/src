# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for infra_stages."""

from __future__ import print_function

import os

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import infra_stages
from chromite.lib import cipd
from chromite.lib import cros_build_lib
from chromite.lib import path_util


class PackageInfraGoBinariesTest(generic_stages_unittest.AbstractStageTestCase,
                                 cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for infra_stages.PackageInfraGoBinariesStage."""
  # pylint: disable=protected-access

  BOT_ID = 'amd64-generic-full'
  RELEASE_TAG = ''

  def setUp(self):
    # PackageInfraGoBinariesStage being tested.
    self._stage = None

    # Keys are Portage package names as hardcoded in infra_stages (i.e. without
    # categories). Values are lists of absolute paths of files belonging to each
    # package relative to the chroot (e.g. "/usr/bin/foo").
    self._portage_package_files = {}

    # Keys are names of built CIPD packages. Values are lists of absolute paths
    # of files included in each package relative to the input dir (e.g.
    # "/usr/bin/foo").
    self._cipd_packages = {}

    # Create a fake chroot directory.
    self._chroot_dir = os.path.join(self.build_root, 'chroot')
    os.makedirs(self._chroot_dir)
    self._mock_from_chroot_path = self.PatchObject(
        path_util, 'FromChrootPath', self._FromChrootPath)

    self._mock_run_build_script = self.PatchObject(
        commands, 'RunBuildScript', autospec=True,
        side_effect=self._FakeRunBuildScript)
    self._mock_build_package = self.PatchObject(
        cipd, 'BuildPackage', autospec=True, side_effect=self._FakeBuildPackage)
    self._mock_get_cipd_from_cache = self.PatchObject(
        cipd, 'GetCIPDFromCache', autospec=True)
    self._mock_upload_artifact = None

    self._Prepare()

  def ConstructStage(self):
    self._run.GetArchive().SetupArchivePath()
    self._stage = infra_stages.PackageInfraGoBinariesStage(self._run,
                                                           self._current_board)

    # Mock out a method that PackageInfraGoBinariesStage inherits from
    # generic_stages.
    self._mock_upload_artifact = \
        self.PatchObject(self._stage, 'UploadArtifact', autospec=True)

    return self._stage

  def _FromChrootPath(self, path):
    """Returns an absolute path within the fake chroot dir.

    Args:
      path: Absolute file path as seen within the chroot, e.g. "/usr/bin/foo".

    Returns:
      String containing chroot suffixed by path, e.g. "/tmp/chroot/usr/bin/foo".
    """
    return os.path.join(self._chroot_dir, os.path.relpath(path, '/'))

  def _FakeRunBuildScript(self, buildroot, cmd, chromite_cmd=False, **kwargs):
    """Fake implemenation of commands.RunBuildScript."""
    del buildroot, chromite_cmd, kwargs
    if (isinstance(cmd, list) or isinstance(cmd, tuple)) and \
        len(cmd) >= 3 and cmd[0] == 'equery' and 'f' in cmd:
      files = self._portage_package_files.get(cmd[-1], [])
      return cros_build_lib.CommandResult(returncode=0, output='\n'.join(files))

    raise RuntimeError('Command %s not handled by test' % str(cmd))

  def _FakeBuildPackage(self, cipd_path, package, in_dir, outfile):
    """Fake implementation of cipd.BuildPackage."""
    del cipd_path, outfile
    paths = []
    for base, _, files in os.walk(in_dir):
      for f in files:
        # Get an absolute path within in_dir (e.g. "/usr/bin/foo").
        paths.append('/' + os.path.relpath(os.path.join(base, f), in_dir))
    self._cipd_packages[package] = sorted(paths)

  def _RegisterPortagePackageFile(self, package, path):
    """Registers a file as belonging to a Portage package.

    An empty file is also created within _chroot_dir.

    Args:
      package: Package name as used within PackageInfraGoBinariesStage (i.e.
        without category).
      path: Absolute file path as seen within the chroot, e.g. "/usr/bin/foo".
    """
    self._portage_package_files.setdefault(package, []).append(path)
    full_path = self._FromChrootPath(path)
    full_dir = os.path.dirname(full_path)
    if not os.path.exists(full_dir):
      os.makedirs(full_dir)
    open(full_path, 'a').close()

  def testBuildPackages(self):
    """Tests that CIPD packages are built."""
    # CIPD packages expected to be built.
    cipd_packages = {}

    # Register two arbitrary files for each Portage package.
    for pkg in infra_stages._GO_PACKAGES:
      bin_path = os.path.join('/usr/bin', pkg)
      self._RegisterPortagePackageFile(pkg, bin_path)
      data_path = os.path.join('/usr/share', pkg, 'data.bin')
      self._RegisterPortagePackageFile(pkg, data_path)
      cipd_packages[infra_stages._CIPD_PACKAGE_PREFIX + pkg] = \
          [bin_path, data_path]

    self.RunStage()
    self.assertDictEqual(self._cipd_packages, cipd_packages)
    for pkg in infra_stages._GO_PACKAGES:
      self._mock_upload_artifact.assert_any_call(
          infra_stages._GetPackagePath(self._stage.archive_path, pkg),
          archive=False)
