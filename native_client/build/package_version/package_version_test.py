#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for package version."""

import os
import random
import shutil
import sys
import tarfile
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.fake_downloader
import pynacl.fake_storage
import pynacl.file_tools
import pynacl.log_tools
import pynacl.platform
import pynacl.working_directory

import archive_info
import error
import package_info
import package_locations
import package_version

class TestPackageVersion(unittest.TestCase):

  def setUp(self):
    self._fake_storage = pynacl.fake_storage.FakeStorage()
    self._fake_downloader = pynacl.fake_downloader.FakeDownloader()

  def GenerateMockFile(self, work_dir, mock_dir = 'mock_dir',
                       mock_file=None, contents='mock contents'):
    """Generates a file with random content in it.

    Args:
      work_dir: Root working directory where mock directory will live.
      mock_dir: Mock directory under root directory where mock file will live.
      mock_file: Mock filename which generated file will be generated under.
      contents: Base content for the mock file.
    Returns:
      File path for the mock file with the mock contents with random content.
    """
    if mock_file is None:
      mock_file = 'mockfile_%s.txt' % str(random.random())

    full_mock_dir = os.path.join(work_dir, mock_dir)
    pynacl.file_tools.MakeDirectoryIfAbsent(full_mock_dir)

    full_mock_file = os.path.join(full_mock_dir, mock_file)
    with open(full_mock_file, 'wt') as f:
      f.write(contents)
      f.write(str(random.random()))

    return full_mock_file

  def GeneratePackageInfo(self, archive_list,
                          url_dict={}, src_dir_dict={}, dir_dict={},
                          log_url_dict={}):
    """Generates a package_info.PackageInfo object for list of archives."

    Args:
      archive_list: List of file paths where package archives sit.
      url_dict: dict of archive file path to URL if url exists.
      src_dir_dict: dict of archive file path to source tar dir if exists.
      dir_dict: dict of archive file path to root dir if exists.
    """
    package_desc = package_info.PackageInfo()
    for archive_file in archive_list:
      archive_name = os.path.basename(archive_file)

      if os.path.isfile(archive_file):
        archive_hash = archive_info.GetArchiveHash(archive_file)
      else:
        archive_hash = 'invalid'

      archive_url = url_dict.get(archive_file, None)
      archive_src_tar_dir = src_dir_dict.get(archive_file, '')
      archive_dir = dir_dict.get(archive_file, '')
      archive_log_url = log_url_dict.get(archive_file, None)
      archive_desc = archive_info.ArchiveInfo(name=archive_name,
                                              hash=archive_hash,
                                              url=archive_url,
                                              tar_src_dir=archive_src_tar_dir,
                                              extract_dir=archive_dir,
                                              log_url=archive_log_url)
      package_desc.AppendArchive(archive_desc)

    return package_desc

  def CopyToLocalArchiveFile(self, archive_file, tar_dir):
      local_archive_file = package_locations.GetLocalPackageArchiveFile(
          tar_dir,
          os.path.basename(archive_file),
          archive_info.GetArchiveHash(archive_file)
      )

      pynacl.file_tools.MakeParentDirectoryIfAbsent(local_archive_file)
      shutil.copy2(archive_file, local_archive_file)

  def test_DownloadArchive(self):
    # Check that we can download a package archive correctly.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar = self.GenerateMockFile(work_dir)
      mock_hash = archive_info.GetArchiveHash(mock_tar)

      fake_url = 'http://www.fake.com/archive.tar'
      self._fake_downloader.StoreURL(fake_url, mock_tar)

      mock_log = self.GenerateMockFile(work_dir)
      fake_log_url = 'http://www.fake.com/archive_log.txt'
      self._fake_downloader.StoreURL(fake_log_url, mock_log)

      package_desc = self.GeneratePackageInfo(
          [mock_tar],
          url_dict={mock_tar: fake_url},
          log_url_dict={mock_tar: fake_log_url},
      )

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'archive_target'
      package_name = 'archive_name'
      package_version.DownloadPackageArchives(
          tar_dir,
          package_target,
          package_name,
          package_desc,
          downloader=self._fake_downloader.Download,
          include_logs=False,
      )
      self.assertEqual(self._fake_downloader.GetDownloadCount(), 1,
                       "Expected a single archive to have been downloaded.")

      mock_name = os.path.basename(mock_tar)
      local_archive_file = package_locations.GetLocalPackageArchiveFile(
          tar_dir,
          mock_name,
          mock_hash)

      self.assertEqual(archive_info.GetArchiveHash(local_archive_file),
                       mock_hash)

      # Check log is not downloaded.
      local_archive_log = package_locations.GetLocalPackageArchiveLogFile(
          local_archive_file)
      self.assertFalse(os.path.isfile(local_archive_log))

      # Check log is downloaded when include_logs is True
      package_version.DownloadPackageArchives(
          tar_dir,
          package_target,
          package_name,
          package_desc,
          downloader=self._fake_downloader.Download,
          include_logs=True,
      )
      self.assertEqual(self._fake_downloader.GetDownloadCount(), 2,
                       "Expected only log to have been downloaded.")

      self.assertEqual(
          archive_info.GetArchiveHash(local_archive_log),
          archive_info.GetArchiveHash(mock_log))

  def test_DownloadArchiveMissingURLFails(self):
    # Checks that we fail when the archive has no URL set.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      package_desc = package_info.PackageInfo()
      archive_desc = archive_info.ArchiveInfo(name='missing_name.tar',
                                              hash='missing_hash',
                                              url=None)
      package_desc.AppendArchive(archive_desc)

      tar_dir = os.path.join(work_dir, 'tar_dir')
      self.assertRaises(
          error.Error,
          package_version.DownloadPackageArchives,
          tar_dir,
          'missing_target',
          'missing_name',
          package_desc,
          downloader=self._fake_downloader.Download
      )

  def test_DownloadArchiveLogMissingURLFails(self):
    # Checks that we fail when the archive log has an invalid URL set.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar = self.GenerateMockFile(work_dir)
      fake_url = 'http://www.fake.com/archive.tar'
      self._fake_downloader.StoreURL(fake_url, mock_tar)

      package_desc = self.GeneratePackageInfo(
          [mock_tar],
          url_dict={mock_tar: fake_url},
          log_url_dict={mock_tar: 'http://www.missingurl.com'},
      )

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'archive_target'
      package_name = 'archive_name'

      # Not including logs should not fail.
      package_version.DownloadPackageArchives(
          tar_dir,
          package_target,
          package_name,
          package_desc,
          downloader=self._fake_downloader.Download,
          include_logs=False,
      )

      # Including logs should now fail.
      self.assertRaises(
          IOError,
          package_version.DownloadPackageArchives,
          tar_dir,
          package_target,
          package_name,
          package_desc,
          downloader=self._fake_downloader.Download,
          include_logs=True,
      )

  def test_DownloadArchiveMismatchFails(self):
    # Check download archive fails when the hash does not match expected hash.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar = self.GenerateMockFile(work_dir)
      fake_url = 'http://www.fake.com/archive.tar'
      self._fake_downloader.StoreURL(fake_url, mock_tar)

      package_desc = package_info.PackageInfo()
      archive_desc = archive_info.ArchiveInfo(name='invalid_name.tar',
                                              hash='invalid_hash',
                                              url=fake_url)
      package_desc.AppendArchive(archive_desc)

      tar_dir = os.path.join(work_dir, 'tar_dir')
      self.assertRaises(
          error.Error,
          package_version.DownloadPackageArchives,
          tar_dir,
          'mismatch_target',
          'mismatch_name',
          package_desc,
          downloader=self._fake_downloader.Download
      )

  def test_DownloadSharedTarsDownloadsOnce(self):
  # Test that tars with same name and hash get downloaded only once.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      tar_dir = os.path.join(work_dir, 'tar_dir')
      overlay_dir = os.path.join(work_dir, 'overlay_dir')
      dest_dir = os.path.join(work_dir, 'dest_dir')
      package_target_1 = 'custom_package_target_1'
      package_name_1 = 'custom_package_1'
      package_target_2 = 'custom_package_target_2'
      package_name_2 = 'custom_package_2'
      package_revision = 10

      mock_file = self.GenerateMockFile(work_dir)
      mock_tar = os.path.join(work_dir, 'shared_archive.tar')
      with tarfile.TarFile(mock_tar, 'w') as f:
        f.add(mock_file, arcname=os.path.basename(mock_file))

      fake_url = 'http://www.fake.com/archive.tar'
      self._fake_downloader.StoreURL(fake_url, mock_tar)

      # Generate 2 package files both containing the same tar file
      package_desc_1 = self.GeneratePackageInfo([mock_tar],
                                                url_dict={mock_tar: fake_url})
      package_file_1 = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target_1,
          package_name_1
      )
      package_desc_1.SavePackageFile(package_file_1)

      package_desc_2 = self.GeneratePackageInfo([mock_tar],
                                                url_dict={mock_tar: fake_url})
      package_file_2 = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target_2,
          package_name_2
      )
      package_desc_2.SavePackageFile(package_file_2)

      package_version.DownloadPackageArchives(
          tar_dir,
          package_target_1,
          package_name_1,
          package_desc_1,
          downloader=self._fake_downloader.Download,
          include_logs=False,
      )
      package_version.DownloadPackageArchives(
          tar_dir,
          package_target_2,
          package_name_2,
          package_desc_2,
          downloader=self._fake_downloader.Download,
          include_logs=False,
      )
      self.assertEqual(self._fake_downloader.GetDownloadCount(), 1,
                       "Expected a single archive to have been downloaded.")

  def test_ArchivePackageArchives(self):
    # Check if we can archive a list of archives to the tar directory.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar1 = self.GenerateMockFile(
          work_dir,
          mock_file='file1.tar',
          contents='mock contents 1'
      )
      mock_tar2 = self.GenerateMockFile(
          work_dir,
          mock_file='file2.tar',
          contents='mock contents 2'
      )

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'test_package_archives'
      package_name = 'package_archives'
      package_version.ArchivePackageArchives(
          tar_dir,
          package_target,
          package_name,
          [mock_tar1, mock_tar2]
      )

      package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      expected_package_desc = self.GeneratePackageInfo([mock_tar1, mock_tar2])
      package_desc = package_info.PackageInfo(package_file)

      self.assertEqual(expected_package_desc, package_desc)
                      # "Archived package does not match mock package.")

  def test_PackageUpload(self):
    # Check if we can properly upload a package file from the tar directory.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'test_package_archives'
      package_name = 'package_archives'
      package_revision = 10
      package_version.ArchivePackageArchives(
          tar_dir,
          package_target,
          package_name,
          []
      )

      package_version.UploadPackage(
          self._fake_storage,
          package_revision,
          tar_dir,
          package_target,
          package_name,
          False
      )
      self.assertEqual(
          self._fake_storage.WriteCount(),
          1,
          "Package did not get properly uploaded"
      )

      remote_package_key = package_locations.GetRemotePackageKey(
          False,
          package_revision,
          package_target,
          package_name
      )
      downloaded_package = os.path.join(work_dir, 'download_package.json')
      package_info.DownloadPackageInfoFiles(
          downloaded_package,
          remote_package_key,
          downloader=self._fake_storage.GetFile)
      downloaded_package_desc = package_info.PackageInfo(downloaded_package)

      original_package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      original_package_desc = package_info.PackageInfo(original_package_file)

      self.assertEqual(downloaded_package_desc, original_package_desc)

  def test_CustomPackageUpload(self):
    # Check if we can upload a package file from a custom location.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      custom_package_file = os.path.join(work_dir, 'custom_package.json')
      package_desc = self.GeneratePackageInfo([])
      package_desc.SavePackageFile(custom_package_file)

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      package_version.UploadPackage(
          self._fake_storage,
          package_revision,
          tar_dir,
          package_target,
          package_name,
          False,
          custom_package_file=custom_package_file
      )
      self.assertEqual(
          self._fake_storage.WriteCount(),
          1,
          "Package did not get properly uploaded"
      )

      remote_package_key = package_locations.GetRemotePackageKey(
          False,
          package_revision,
          package_target,
          package_name
      )
      downloaded_package = os.path.join(work_dir, 'download_package.json')
      package_info.DownloadPackageInfoFiles(
          downloaded_package,
          remote_package_key,
          downloader=self._fake_storage.GetFile)
      downloaded_package_desc = package_info.PackageInfo(downloaded_package)

      original_package_desc = package_info.PackageInfo(custom_package_file)

      self.assertEqual(downloaded_package_desc, original_package_desc)

  def test_UploadKeepsArchiveURL(self):
    # Checks if the archive URL is kept after a package upload.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar = self.GenerateMockFile(work_dir)
      mock_url = 'http://www.mock.com/mock.tar'
      package_desc = self.GeneratePackageInfo(
          [mock_tar],
          url_dict={mock_tar: mock_url}
      )

      package_file = os.path.join(work_dir, 'package_file.json')
      package_desc.SavePackageFile(package_file)

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      package_version.UploadPackage(
          self._fake_storage,
          package_revision,
          tar_dir,
          package_target,
          package_name,
          False,
          custom_package_file=package_file
      )
      self.assertEqual(
          self._fake_storage.WriteCount(),
          2,
          "Package did not get properly uploaded"
      )

      remote_package_key = package_locations.GetRemotePackageKey(
          False,
          package_revision,
          package_target,
          package_name
      )
      downloaded_package = os.path.join(work_dir, 'download_package.json')
      package_info.DownloadPackageInfoFiles(
          downloaded_package,
          remote_package_key,
          downloader=self._fake_storage.GetFile)
      downloaded_package_desc = package_info.PackageInfo(downloaded_package)

      # Verify everything (including URL) still matches.
      self.assertEqual(downloaded_package_desc, package_desc)

  def test_NoArchiveURLDoesUpload(self):
    # Checks when uploading package with no archive URL, archive is uploaded.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      mock_file = self.GenerateMockFile(work_dir)
      mock_tar = os.path.join(work_dir, 'archive_name.tar')
      with tarfile.TarFile(mock_tar, 'w') as f:
        f.add(mock_file)

      package_desc = self.GeneratePackageInfo([mock_tar])
      package_file = os.path.join(work_dir, 'package_file.json')
      package_desc.SavePackageFile(package_file)

      self.CopyToLocalArchiveFile(mock_tar, tar_dir)

      package_version.UploadPackage(
          self._fake_storage,
          package_revision,
          tar_dir,
          package_target,
          package_name,
          False,
          custom_package_file=package_file
      )
      self.assertEqual(
          self._fake_storage.WriteCount(),
          3,
          "3 files (package, archive_info, archive) should have been uploaded."
      )

      remote_package_key = package_locations.GetRemotePackageKey(
          False,
          package_revision,
          package_target,
          package_name
      )
      downloaded_package = os.path.join(work_dir, 'download_package.json')
      package_info.DownloadPackageInfoFiles(
          downloaded_package,
          remote_package_key,
          downloader=self._fake_storage.GetFile)
      downloaded_package_desc = package_info.PackageInfo(downloaded_package)

      archive_list = downloaded_package_desc.GetArchiveList()
      self.assertEqual(len(archive_list), 1,
                       "The downloaded package does not have 1 archive.")
      self.assertTrue(archive_list[0].GetArchiveData().url,
                      "The downloaded archive still does not have a proper URL")

  def test_ExtractPackageTargets(self):
    # Tests that we can extract package targets from the tar directory properly.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_file1 = self.GenerateMockFile(work_dir, mock_file='mockfile1.txt')
      mock_file2 = self.GenerateMockFile(work_dir, mock_file='mockfile2.txt')
      mock_file3 = self.GenerateMockFile(work_dir, mock_file='mockfile3.txt')

      tar_dir = os.path.join(work_dir, 'tar_dir')
      dest_dir = os.path.join(work_dir, 'dest_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      mock_tar1 = os.path.join(work_dir, 'archive_name1.tar')
      with tarfile.TarFile(mock_tar1, 'w') as f:
        f.add(mock_file1, arcname=os.path.basename(mock_file1))

      mock_tar2 = os.path.join(work_dir, 'archive_name2.tar')
      with tarfile.TarFile(mock_tar2, 'w') as f:
        f.add(mock_file2, arcname=os.path.basename(mock_file2))

      mock_tar3 = os.path.join(work_dir, 'archive_name3.tar')
      with tarfile.TarFile(mock_tar3, 'w') as f:
        arcname = os.path.join('rel_dir', os.path.basename(mock_file3))
        f.add(mock_file3, arcname=arcname)

      self.CopyToLocalArchiveFile(mock_tar1, tar_dir)
      self.CopyToLocalArchiveFile(mock_tar2, tar_dir)
      self.CopyToLocalArchiveFile(mock_tar3, tar_dir)

      package_desc = self.GeneratePackageInfo(
        [mock_tar1, mock_tar2, mock_tar3],
        dir_dict={mock_tar2: 'tar2_dir'},
        src_dir_dict={mock_tar3: 'rel_dir'},
      )
      package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      package_desc.SavePackageFile(package_file)

      package_version.ExtractPackageTargets(
          [(package_target, package_name)],
          tar_dir,
          dest_dir,
          downloader=self._fake_downloader.Download
      )
      self.assertEqual(self._fake_downloader.GetDownloadCount(), 0,
                       "Extracting a package should not download anything.")

      full_dest_dir = package_locations.GetFullDestDir(
          dest_dir,
          package_target,
          package_name
      )
      dest_mock_file1 = os.path.join(full_dest_dir,
                                     os.path.basename(mock_file1))
      dest_mock_file2 = os.path.join(full_dest_dir,
                                     'tar2_dir',
                                     os.path.basename(mock_file2))
      dest_mock_file3 = os.path.join(full_dest_dir,
                                     os.path.basename(mock_file3))

      with open(mock_file1, 'rb') as f:
        mock_contents1 = f.read()
      with open(mock_file2, 'rb') as f:
        mock_contents2 = f.read()
      with open(mock_file3, 'rb') as f:
        mock_contents3 = f.read()
      with open(dest_mock_file1, 'rb') as f:
        dest_mock_contents1 = f.read()
      with open(dest_mock_file2, 'rb') as f:
        dest_mock_contents2 = f.read()
      with open(dest_mock_file3, 'rb') as f:
        dest_mock_contents3 = f.read()

      self.assertEqual(mock_contents1, dest_mock_contents1)
      self.assertEqual(mock_contents2, dest_mock_contents2)
      self.assertEqual(mock_contents3, dest_mock_contents3)

  def test_OverlayPackageTargets(self):
    # Tests that we can extract package targets with an overlay directory
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_file1 = self.GenerateMockFile(work_dir, mock_file='mockfile1.txt')
      mock_file2 = self.GenerateMockFile(work_dir, mock_file='mockfile2.txt')
      mock_file3 = self.GenerateMockFile(work_dir, mock_file='mockfile3.txt')

      tar_dir = os.path.join(work_dir, 'tar_dir')
      overlay_dir = os.path.join(work_dir, 'overlay_dir')
      dest_dir = os.path.join(work_dir, 'dest_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      os.makedirs(tar_dir)
      os.makedirs(overlay_dir)

      # Tar1 (mockfile1) will be a regular archive within the tar directory,
      # while tar2 (mockfile2) will be overlaid and replaced by
      # overlay_tar2 (mockfile3).
      mock_tar1 = os.path.join(tar_dir, 'archive_name1.tar')
      with tarfile.TarFile(mock_tar1, 'w') as f:
        f.add(mock_file1, arcname=os.path.basename(mock_file1))

      mock_tar2 = os.path.join(tar_dir, 'archive_name2.tar')
      with tarfile.TarFile(mock_tar2, 'w') as f:
        f.add(mock_file2, arcname=os.path.basename(mock_file2))

      overlay_tar2 = os.path.join(overlay_dir, 'archive_name2.tar')
      with tarfile.TarFile(overlay_tar2, 'w') as f:
        f.add(mock_file3, arcname=os.path.basename(mock_file3))

      self.CopyToLocalArchiveFile(mock_tar1, tar_dir)
      self.CopyToLocalArchiveFile(mock_tar2, tar_dir)
      self.CopyToLocalArchiveFile(overlay_tar2, overlay_dir)

      # Generate the regular package file, along with the overlay package file.
      package_desc = self.GeneratePackageInfo([mock_tar1, mock_tar2])
      package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      package_desc.SavePackageFile(package_file)

      overlay_package_desc = self.GeneratePackageInfo([overlay_tar2])
      overlay_package_file = package_locations.GetLocalPackageFile(
          overlay_dir,
          package_target,
          package_name
      )
      overlay_package_desc.SavePackageFile(overlay_package_file)

      package_version.ExtractPackageTargets(
          [(package_target, package_name)],
          tar_dir,
          dest_dir,
          downloader=self._fake_downloader.Download,
          overlay_tar_dir=overlay_dir,
      )

      full_dest_dir = package_locations.GetFullDestDir(
          dest_dir,
          package_target,
          package_name
      )

      dest_mock_file1 = os.path.join(
          full_dest_dir,
          os.path.basename(mock_file1)
      )
      dest_mock_file2 = os.path.join(
          full_dest_dir,
          os.path.basename(mock_file2)
      )
      dest_mock_file3 = os.path.join(
          full_dest_dir,
          os.path.basename(mock_file3)
      )

      # mock_file2 should not exist in the destination since it was replaced.
      self.assertFalse(os.path.isfile(dest_mock_file2))

      with open(mock_file1, 'rb') as f:
        mock_contents1 = f.read()
      with open(mock_file3, 'rb') as f:
        mock_contents3 = f.read()
      with open(dest_mock_file1, 'rb') as f:
        dest_mock_contents1 = f.read()
      with open(dest_mock_file3, 'rb') as f:
        dest_mock_contents3 = f.read()

      self.assertEqual(mock_contents1, dest_mock_contents1)
      self.assertEqual(mock_contents3, dest_mock_contents3)

  def test_DownloadMismatchArchiveUponExtraction(self):
    # Test that mismatching archive files are downloaded upon extraction.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_file1 = self.GenerateMockFile(work_dir, mock_file='mockfile1.txt')
      mock_file2 = self.GenerateMockFile(work_dir, mock_file='mockfile2.txt')

      tar_dir = os.path.join(work_dir, 'tar_dir')
      dest_dir = os.path.join(work_dir, 'dest_dir')
      mock_tars_dir = os.path.join(work_dir, 'mock_tars')
      package_target = 'custom_package_target'
      package_name = 'custom_package'
      package_revision = 10

      # Create mock tars and mock URLS where the tars can be downloaded from.
      os.makedirs(mock_tars_dir)
      mock_tar1 = os.path.join(mock_tars_dir, 'mock1.tar')
      mock_url1 = 'https://www.mock.com/tar1.tar'
      with tarfile.TarFile(mock_tar1, 'w') as f:
        f.add(mock_file1, arcname=os.path.basename(mock_file1))
      self._fake_downloader.StoreURL(mock_url1, mock_tar1)

      mock_tar2 = os.path.join(mock_tars_dir, 'mock2.tar')
      mock_url2 = 'https://www.mock.com/tar2.tar'
      with tarfile.TarFile(mock_tar2, 'w') as f:
        f.add(mock_file2, arcname=os.path.basename(mock_file2))
      self._fake_downloader.StoreURL(mock_url2, mock_tar2)

      # Have tar1 be missing, have tar2 be a file with invalid data.
      mismatch_tar2 = package_locations.GetLocalPackageArchiveFile(
          tar_dir,
          os.path.basename(mock_tar2),
          archive_info.GetArchiveHash(mock_tar2)
      )
      os.makedirs(os.path.dirname(mismatch_tar2))
      with open(mismatch_tar2, 'wb') as f:
        f.write('mismatch tar')

      package_desc = self.GeneratePackageInfo(
        [mock_tar1, mock_tar2],
        url_dict= {mock_tar1: mock_url1, mock_tar2: mock_url2}
      )
      package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      package_desc.SavePackageFile(package_file)

      package_version.ExtractPackageTargets(
          [(package_target, package_name)],
          tar_dir,
          dest_dir,
          downloader=self._fake_downloader.Download
      )
      self.assertEqual(self._fake_downloader.GetDownloadCount(), 2,
                       "Expected to download exactly 2 mismatched archives.")

      full_dest_dir = package_locations.GetFullDestDir(
          dest_dir,
          package_target,
          package_name
      )

      dest_mock_file2 = os.path.join(
          full_dest_dir,
          os.path.basename(mock_file2)
      )
      dest_mock_file1 = os.path.join(
          full_dest_dir,
          os.path.basename(mock_file1)
      )

      with open(mock_file1, 'rb') as f:
        mock_contents1 = f.read()
      with open(mock_file2, 'rb') as f:
        mock_contents2 = f.read()
      with open(dest_mock_file1, 'rb') as f:
        dest_mock_contents1 = f.read()
      with open(dest_mock_file2, 'rb') as f:
        dest_mock_contents2 = f.read()

      self.assertEqual(mock_contents1, dest_mock_contents1)
      self.assertEqual(mock_contents2, dest_mock_contents2)

  def test_CleanupExtraFiles(self):
    # Test that the cleanup function properly cleans up extra files
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      mock_tar = self.GenerateMockFile(work_dir)

      tar_dir = os.path.join(work_dir, 'tar_dir')
      package_target = 'custom_package_target'
      package_name = 'custom_package'

      package_desc = self.GeneratePackageInfo(
        [mock_tar],
      )
      package_file = package_locations.GetLocalPackageFile(
          tar_dir,
          package_target,
          package_name
      )
      package_desc.SavePackageFile(package_file)
      self.CopyToLocalArchiveFile(mock_tar, tar_dir)

      package_dir = os.path.dirname(package_file)
      archive_file = package_locations.GetLocalPackageArchiveFile(
          tar_dir,
          os.path.basename(mock_tar),
          archive_info.GetArchiveHash(mock_tar))

      extra_file = self.GenerateMockFile(tar_dir)
      extra_file2 = self.GenerateMockFile(package_dir)
      extra_dir = os.path.join(tar_dir, 'extra_dir')
      os.makedirs(extra_dir)
      extra_file3 = self.GenerateMockFile(extra_dir)

      package_version.CleanupTarDirectory(tar_dir)

      # Make sure all the key files were not deleted
      self.assertTrue(os.path.isfile(package_file))
      self.assertTrue(os.path.isfile(archive_file))

      # Make sure package file can be loaded and nothing vital was deleted.
      new_package_desc = package_info.PackageInfo(package_file)

      # Make sure all the extra directories and files were deleted
      self.assertFalse(os.path.isfile(extra_file))
      self.assertFalse(os.path.isfile(extra_file2))
      self.assertFalse(os.path.isfile(extra_file3))
      self.assertFalse(os.path.isdir(extra_dir))


if __name__ == '__main__':
  verbose = '-v' in sys.argv or '--verbose' in sys.argv
  quiet = not verbose
  pynacl.log_tools.SetupLogging(verbose, quiet=quiet)

  unittest.main()
