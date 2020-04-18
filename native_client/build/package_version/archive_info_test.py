#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for archive info."""

import os
import random
import unittest
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.working_directory

import archive_info


class TestArchiveInfo(unittest.TestCase):

  def CreateTemporaryArchive(self):
    archive_name = 'test_name' + str(random.random())
    archive_hash = 'test_hash' + str(random.random())
    archive_url = 'test_url' + str(random.random())
    tar_src_dir = 'test_src' + str(random.random())
    extract_dir = 'test_extr' + str(random.random())
    log_url = 'test_log_url' + str(random.random())

    archive = archive_info.ArchiveInfo(name=archive_name,
                                       hash=archive_hash,
                                       url=archive_url,
                                       tar_src_dir=tar_src_dir,
                                       extract_dir=extract_dir,
                                       log_url=log_url)

    return archive

  def test_HashEmptyForMissingFiles(self):
    # Many scripts rely on the archive hash returning None for missing files.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.assertEqual(None, archive_info.GetArchiveHash('missingfile.tgz'))

  def test_ArchiveHashStable(self):
    # Check if archive hash produces a stable hash
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1.txt')
      temp2 = os.path.join(work_dir, 'temp2.txt')

      temp_contents = 'this is a test'
      with open(temp1, 'wt') as f:
        f.write(temp_contents)
      with open(temp2, 'wt') as f:
        f.write(temp_contents)

      self.assertEqual(archive_info.GetArchiveHash(temp1),
                       archive_info.GetArchiveHash(temp2))

  def test_ArchiveConstructor(self):
    archive_name = 'test_archive'
    archive_hash = 'test_archive_hash'
    archive_url = 'test_archive_url'
    tar_src_dir = 'test_archive_dir'
    extract_dir = 'test_extraction_dir'
    log_url = 'test_log_url'

    archive = archive_info.ArchiveInfo(name=archive_name,
                                       hash=archive_hash,
                                       url=archive_url,
                                       tar_src_dir=tar_src_dir,
                                       extract_dir=extract_dir,
                                       log_url=log_url)

    archive_data = archive.GetArchiveData()
    self.assertEqual(archive_data.name, archive_name)
    self.assertEqual(archive_data.hash, archive_hash)
    self.assertEqual(archive_data.url, archive_url)
    self.assertEqual(archive_data.tar_src_dir, tar_src_dir)
    self.assertEqual(archive_data.extract_dir, extract_dir)
    self.assertEqual(archive_data.log_url, log_url)

  def test_ArchiveFileSaveLoad(self):
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      archive = self.CreateTemporaryArchive()
      temp_json = os.path.join(work_dir, 'archive.json')
      archive.SaveArchiveInfoFile(temp_json)
      loaded_archive = archive_info.ArchiveInfo(archive_info_file=temp_json)
      self.assertEqual(archive, loaded_archive)

  def test_DumpArchiveJson(self):
    archive = self.CreateTemporaryArchive()
    archive_json = archive.DumpArchiveJson()
    loaded_archive = archive_info.ArchiveInfo(archive_info_file=archive_json)
    self.assertEqual(archive, loaded_archive)


if __name__ == '__main__':
  unittest.main()
