#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for packages info."""

import collections
import os
import unittest

import packages_info

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
TEST_PACKAGES_JSON = os.path.join(CURRENT_DIR, 'test_packages.json')
TEST_PLATFORM = 'platform'
TEST_ARCH_ALL = 'arch_all'
TEST_ARCH_SHARED = 'arch_shared'
TEST_ARCH_NON_SHARED = 'arch_non_shared'
TEST_EMPTY_PACKAGE_TARGET = 'empty_package_target'


class TestPackagesInfo(unittest.TestCase):

  def setUp(self):
    self._packages = packages_info.PackagesInfo(TEST_PACKAGES_JSON)

  def test_LoadPackagesFilePath(self):
    self.assertTrue(
        self._packages.GetPackageTargets(TEST_PLATFORM, TEST_ARCH_ALL)
    )

  def test_LoadPackagesFilePointer(self):
    with open(TEST_PACKAGES_JSON, 'rt') as f:
      packages = packages_info.PackagesInfo(f)
    self.assertTrue(packages.GetPackageTargets(TEST_PLATFORM, TEST_ARCH_ALL))

  def test_SharedMarkShared(self):
    shared_package_targets = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_SHARED
    )
    self.assertTrue(shared_package_targets)

    for package_target in shared_package_targets:
      for package in self._packages.GetPackages(package_target):
        self.assertTrue(self._packages.IsSharedPackage(package))

  def test_NonSharedNotMarkShared(self):
    non_shared_targets = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_NON_SHARED
    )
    self.assertTrue(non_shared_targets)

    for package_target in non_shared_targets:
      for package in self._packages.GetPackages(package_target):
        self.assertFalse(self._packages.IsSharedPackage(package))

  def test_SplitPackageTargets(self):
    all_package_targets = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_ALL
    )
    package_targets_1 = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_SHARED
    )
    package_targets_2 = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_NON_SHARED
    )

    set_all = set(all_package_targets)
    set_1 = set(package_targets_1)
    set_2 = set(package_targets_2)

    self.assertFalse(set_1.intersection(set_2))
    self.assertEqual(set_all, set_1.union(set_2))

  def test_InvalidPackageTargetPackages(self):
    self.assertEqual(None, self._packages.GetPackages('InvalidPackageTarget'))

  def test_ValidPackageTargetPackages(self):
    all_package_targets = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_ALL
    )
    for package_target in all_package_targets:
      packages = self._packages.GetPackages(package_target)
      self.assertTrue(isinstance(packages,list))

  def test_EmptyPackageTargetPackages(self):
    packages = self._packages.GetPackages(TEST_EMPTY_PACKAGE_TARGET)
    self.assertEqual([], packages)

  def test_GetPackagesTargetsForPackage(self):
    all_package_targets = self._packages.GetPackageTargets(
        TEST_PLATFORM,
        TEST_ARCH_ALL
    )

    package_targets_dict = collections.defaultdict(set)
    for package_target in all_package_targets:
      for package in self._packages.GetPackages(package_target):
        package_targets_dict[package].add(package_target)

    for package, expected_package_targets in package_targets_dict.iteritems():
      package_targets = self._packages.GetPackageTargetsForPackage(package)
      self.assertEqual(expected_package_targets, set(package_targets))


if __name__ == '__main__':
  unittest.main()
