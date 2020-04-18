#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for packages info."""

import json
import os
import sys
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.working_directory

import archive_info
import error
import package_info
import packages_info
import revision_info

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
TEST_PACKAGES_JSON = os.path.join(CURRENT_DIR, 'test_packages.json')
TEST_PLATFORM = 'platform'
TEST_ARCH_ALL = 'arch_all'
TEST_ARCH_SHARED = 'arch_shared'
TEST_ARCH_NON_SHARED = 'arch_non_shared'
TEST_EMPTY_PACKAGE_TARGET = 'empty_package_target'
TEST_SINGLE_PACKAGE_PACKAGE_TARGET = 'package_1'
TEST_MULTI_PACKAGE_PACKAGE_TARGET = 'package_2'


class TestRevisionInfo(unittest.TestCase):

  def setUp(self):
    self._packages = packages_info.PackagesInfo(TEST_PACKAGES_JSON)

  def test_RevTargetSets(self):
    # Tests that we can properly set a target revision.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    revision_desc = revision_info.RevisionInfo(self._packages)
    revision_desc.SetTargetRevision('test_package', 'package_target', package)

    self.assertEqual(package, revision_desc.GetPackageInfo('package_target'))

  def test_RevisionTargetSamePackage(self):
    # Tests that all the targets must all be the same.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    revision_desc = revision_info.RevisionInfo(self._packages)
    revision_desc.SetTargetRevision('test1', 'package_target', package)

    self.assertRaises(
        error.Error,
        revision_desc.SetTargetRevision,
        'test2',
        'package_target',
        package
    )

  def test_RevisionFileSaveLoad(self):
    # Tests that we can properly save and load a revision file.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    revision = revision_info.RevisionInfo(self._packages)
    revision.SetRevisionNumber('123abc')
    package_targets = self._packages.GetPackageTargetsForPackage(
        TEST_SINGLE_PACKAGE_PACKAGE_TARGET
    )
    self.assertEqual(
        1,
        len(package_targets),
        "Invalid test data, single package package target requires 1 target"
    )

    revision.SetTargetRevision(
        TEST_SINGLE_PACKAGE_PACKAGE_TARGET,
        package_targets[0],
        package
    )

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      revision_file = os.path.join(work_dir, 'test_revision.json')
      revision.SaveRevisionFile(revision_file)

      new_revision = revision_info.RevisionInfo(self._packages, revision_file)

    self.assertEqual(revision, new_revision)

  def test_RevisionFileRequiresRevisionNumber(self):
    # Tests that we can properly save and load a revision file.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    revision = revision_info.RevisionInfo(self._packages)
    package_targets = self._packages.GetPackageTargetsForPackage(
        TEST_SINGLE_PACKAGE_PACKAGE_TARGET
    )
    for package_target in package_targets:
      revision.SetTargetRevision(
          TEST_SINGLE_PACKAGE_PACKAGE_TARGET,
          package_target,
          package
      )

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      revision_file = os.path.join(work_dir, 'test_revision.json')

      self.assertRaises(
          error.Error,
          revision.SaveRevisionFile,
          revision_file
      )

  def test_AlteredRevisionFileFails(self):
    # Tests that an altered revision file will fail to load.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    revision = revision_info.RevisionInfo(self._packages)
    revision.SetRevisionNumber('123abc')
    package_targets = self._packages.GetPackageTargetsForPackage(
        TEST_SINGLE_PACKAGE_PACKAGE_TARGET
    )
    for package_target in package_targets:
      revision.SetTargetRevision(
          TEST_SINGLE_PACKAGE_PACKAGE_TARGET,
          package_target,
          package
      )

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      revision_file = os.path.join(work_dir, 'altered_revision.json')
      revision.SaveRevisionFile(revision_file)

      # Alter the file directly and save it back out
      with open(revision_file, 'rt') as f:
        revision_json = json.load(f)
      revision_json[revision_info.FIELD_REVISION] = 'noise'
      with open(revision_file, 'wt') as f:
        json.dump(revision_json, f)

      new_revision = revision_info.RevisionInfo(self._packages)
      self.assertRaises(
          error.Error,
          new_revision.LoadRevisionFile,
          revision_file
      )

  def test_RevisionFileMustSetAllTargets(self):
    # Tests that a revision file fails if not all package targets are set.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    package_targets = self._packages.GetPackageTargetsForPackage(
        TEST_MULTI_PACKAGE_PACKAGE_TARGET
    )
    self.assertTrue(
        len(package_targets) > 0,
        'Invalid test data, multiple package targets expected'
    )

    revision = revision_info.RevisionInfo(self._packages)
    revision.SetRevisionNumber('123abc')
    revision.SetTargetRevision(
        TEST_MULTI_PACKAGE_PACKAGE_TARGET,
        package_targets[0],
        package
    )

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      revision_file = os.path.join(work_dir, 'incomplete_revision.json')
      self.assertRaises(
          error.Error,
          revision.SaveRevisionFile,
          revision_file
      )

  def test_RevisionFileSavesForMultiTargets(self):
    # Tests that a revision successfully saves a multi-package target package.
    package = package_info.PackageInfo()
    package.AppendArchive(archive_info.ArchiveInfo(name='test_name',
                                                   hash='hash_value'))

    package_targets = self._packages.GetPackageTargetsForPackage(
        TEST_MULTI_PACKAGE_PACKAGE_TARGET
    )
    self.assertTrue(
        len(package_targets) > 0,
        'Invalid test data, multiple package targets expected'
    )

    revision = revision_info.RevisionInfo(self._packages)
    revision.SetRevisionNumber('123abc')
    for package_target in package_targets:
      revision.SetTargetRevision(
          TEST_MULTI_PACKAGE_PACKAGE_TARGET,
          package_target,
          package
      )

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      revision_file = os.path.join(work_dir, 'complete_revision.json')
      revision.SaveRevisionFile(revision_file)

      new_revision = revision_info.RevisionInfo(self._packages, revision_file)

    self.assertEqual(revision, new_revision)


if __name__ == '__main__':
  unittest.main()
