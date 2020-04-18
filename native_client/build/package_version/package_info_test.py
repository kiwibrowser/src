#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for package info."""

import json
import os
import unittest
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.working_directory

import archive_info
import package_info


class TestPackageInfo(unittest.TestCase):

  def test_AddArchive(self):
    # Check that we can successfully add an archive.
    archive_name = 'test_archive'
    archive_hash = 'test_archive_hash'
    archive_url = 'test_archive_url'
    tar_src_dir = 'test_archive_dir'
    extract_dir = 'test_extraction_dir'

    package = package_info.PackageInfo()
    archive = archive_info.ArchiveInfo(name=archive_name,
                                       hash=archive_hash,
                                       url=archive_url,
                                       tar_src_dir=tar_src_dir,
                                       extract_dir=extract_dir)
    package.AppendArchive(archive)

    archive_list = package.GetArchiveList()

    self.assertEqual(len(archive_list), 1)
    archive_item = archive_list[0].GetArchiveData()
    self.assertEqual(archive_item.name, archive_name)
    self.assertEqual(archive_item.hash, archive_hash)
    self.assertEqual(archive_item.url, archive_url)
    self.assertEqual(archive_item.tar_src_dir, tar_src_dir)
    self.assertEqual(archive_item.extract_dir, extract_dir)

  def test_ClearArchiveListClearsEverything(self):
    # Check that clear archive list actually clears everything
    package1 = package_info.PackageInfo()
    archive1 = archive_info.ArchiveInfo(name='name',
                                        hash='hash',
                                        url='url',
                                        tar_src_dir='tar_src_dir',
                                        extract_dir='extract_dir')
    package1.AppendArchive(archive1)
    package1.ClearArchiveList()

    # Test to be sure the archive list is clear.
    self.assertEqual(len(package1.GetArchiveList()), 0)

    # Test to be sure the state is equal to an empty PackageInfo.
    package2 = package_info.PackageInfo()
    self.assertEqual(package1, package2)

  def test_OrderIndependentEquality(self):
    # Check that order does not matter when adding multiple archives.
    arch_name1 = 'archive1.tar'
    arch_name2 = 'archive2.tar'
    arch_hash1 = 'archive_hash1'
    arch_hash2 = 'archive_hash2'

    package1 = package_info.PackageInfo()
    package1.AppendArchive(archive_info.ArchiveInfo(name=arch_name1,
                                                    hash=arch_hash1))
    package1.AppendArchive(archive_info.ArchiveInfo(name=arch_name2,
                                                    hash=arch_hash2))

    package2 = package_info.PackageInfo()
    package2.AppendArchive(archive_info.ArchiveInfo(name=arch_name2,
                                                    hash=arch_hash2))
    package2.AppendArchive(archive_info.ArchiveInfo(name=arch_name1,
                                                    hash=arch_hash1))

    self.assertEqual(len(package1.GetArchiveList()), 2)
    self.assertEqual(len(package2.GetArchiveList()), 2)
    self.assertEqual(package1, package2)

  def test_PackageLoadJsonList(self):
    # Check if we can successfully load a list of archive.
    arch_name1 = 'archive_item1.tar'
    arch_name2 = 'archive_item2.tar'
    arch_hash1 = 'archive_item_hash1'
    arch_hash2 = 'archive_item_hash2'

    mast_package = package_info.PackageInfo()
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name1,
                                                        hash=arch_hash1))
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name2,
                                                        hash=arch_hash2))
    json_data = mast_package.DumpPackageJson()

    constructed_package = package_info.PackageInfo(json_data)
    loaded_package = package_info.PackageInfo()
    loaded_package.LoadPackageFile(json_data)

    self.assertEqual(mast_package, constructed_package)
    self.assertEqual(mast_package, loaded_package)

  def test_PackageSaveLoadFile(self):
    # Check if we can save/load a package file and retain it's values.
    arch_name1 = 'archive_saveload1.tar'
    arch_name2 = 'archive_saveload2.tar'
    arch_hash1 = 'archive_saveload_hash1'
    arch_hash2 = 'archive_saveload_hash2'

    mast_package = package_info.PackageInfo()
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name1,
                                                        hash=arch_hash1))
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name2,
                                                        hash=arch_hash2))

    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      package_file = os.path.join(work_dir, 'test_package.json')
      mast_package.SavePackageFile(package_file)

      constructed_package = package_info.PackageInfo(package_file)
      loaded_package = package_info.PackageInfo()
      loaded_package.LoadPackageFile(package_file)

      self.assertEqual(mast_package, constructed_package)
      self.assertEqual(mast_package, loaded_package)

  def test_PackageJsonDumpLoad(self):
    # Check if Json from DumpPackageJson() represents a package correctly.
    arch_name1 = 'archive_json1.tar'
    arch_name2 = 'archive_json2.tar'
    arch_hash1 = 'archive_json_hash1'
    arch_hash2 = 'archive_json_hash2'

    mast_package = package_info.PackageInfo()
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name1,
                                                        hash=arch_hash1))
    mast_package.AppendArchive(archive_info.ArchiveInfo(name=arch_name2,
                                                        hash=arch_hash2))

    json_package = package_info.PackageInfo(mast_package.DumpPackageJson())
    self.assertEqual(mast_package, json_package)


if __name__ == '__main__':
  unittest.main()
