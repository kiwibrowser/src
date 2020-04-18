#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A package is a JSON file describing a list of package archives."""

import json
import os
import posixpath
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.file_tools
import pynacl.gsd_storage

import archive_info
import error

PACKAGE_KEY_ARCHIVES = 'archives'
PACKAGE_KEY_VERSION = 'version'

CURRENT_PACKAGE_VERSION = 1


def ReadPackageFile(package_file):
  """Returns a PackageInfoTuple representation of a JSON package file."""
  with open(package_file, 'rt') as f:
    json_value = json.load(f)

    # TODO(dyen): Support old format temporarily when it was a list of archives.
    if isinstance(json_value, list):
      return { PACKAGE_KEY_ARCHIVES: json_value, PACKAGE_KEY_VERSION: 0 }
    else:
      return json_value


def GetFileBaseName(filename):
  """Removes all extensions from a file.

  (Note: os.path.splitext() only removes the last extension).
  """
  first_ext = filename.find('.')
  if first_ext != -1:
    filename = filename[:first_ext]
  return filename


def GetLocalPackageName(local_package_file):
  """Returns the package name given a local package file."""
  package_basename = os.path.basename(local_package_file)
  return GetFileBaseName(package_basename)


def GetRemotePackageName(remote_package_file):
  """Returns the package name given a remote posix based package file."""
  package_basename = posixpath.basename(remote_package_file)
  return GetFileBaseName(package_basename)


def DownloadPackageInfoFiles(local_package_file, remote_package_file,
                             downloader=None):
  """Downloads all package info files from a downloader.

  Downloads a package file from the cloud along with all of the archive
  info files. Archive info files are expected to be in a directory with the
  name of the package along side the package file. Files will be downloaded
  in the same structure.

  Args:
    local_package_file: Local package file where root file will live.
    remote_package_file: Remote package URL to download from.
    downloader: Optional downloader if standard HTTP one should not be used.
  """
  if downloader is None:
    downloader = pynacl.gsd_storage.HttpDownload

  pynacl.file_tools.MakeParentDirectoryIfAbsent(local_package_file)
  downloader(remote_package_file, local_package_file)
  if not os.path.isfile(local_package_file):
    raise error.Error('Could not download package file: %s.' %
                      remote_package_file)

  package_data = ReadPackageFile(local_package_file)
  archive_list = package_data[PACKAGE_KEY_ARCHIVES]
  local_package_name = GetLocalPackageName(local_package_file)
  remote_package_name = GetRemotePackageName(remote_package_file)

  local_archive_dir = os.path.join(os.path.dirname(local_package_file),
                                   local_package_name)
  remote_archive_dir = posixpath.join(posixpath.dirname(remote_package_file),
                                      remote_package_name)

  pynacl.file_tools.MakeDirectoryIfAbsent(local_archive_dir)
  for archive in archive_list:
    archive_file = archive + '.json'
    local_archive_file = os.path.join(local_archive_dir, archive_file)
    remote_archive_file = posixpath.join(remote_archive_dir, archive_file)
    downloader(remote_archive_file, local_archive_file)
    if not os.path.isfile(local_archive_file):
      raise error.Error('Could not download archive file: %s.' %
                        remote_archive_file)

def UploadPackageInfoFiles(storage, package_target, package_name,
                           remote_package_file, local_package_file,
                           skip_missing=False, annotate=False):
  """Uploads all package info files from a downloader.

  Uploads a package file to the cloud along with all of the archive info
  files. Archive info files are expected to be in a directory with the
  name of the package along side the package file. Files will be uploaded
  using the same file structure.

  Args:
    storage: Cloud storage object to store the files to.
    remote_package_file: Remote package URL to upload to.
    local_package_file: Local package file where root file lives.
    skip_missing: Whether to skip missing archive files or error.
    annotate: Whether to annotate build bot links.
  Returns:
    The URL where the root package file is located.
  """
  package_data = ReadPackageFile(local_package_file)
  archive_list = package_data[PACKAGE_KEY_ARCHIVES]
  local_package_name = GetLocalPackageName(local_package_file)
  remote_package_name = GetRemotePackageName(remote_package_file)

  local_archive_dir = os.path.join(os.path.dirname(local_package_file),
                                   local_package_name)
  remote_archive_dir = posixpath.join(posixpath.dirname(remote_package_file),
                                      remote_package_name)

  num_archives = len(archive_list)
  for index, archive in enumerate(archive_list):
    archive_file = archive + '.json'
    local_archive_file = os.path.join(local_archive_dir, archive_file)
    remote_archive_file = posixpath.join(remote_archive_dir, archive_file)
    if skip_missing and not os.path.isfile(local_archive_file):
      continue

    archive_url = storage.PutFile(local_archive_file, remote_archive_file)
    if annotate:
      print ('@@@STEP_LINK@download (%s/%s/%s.json [%d/%d])@%s@@@' %
             (package_target, package_name, archive, index+1, num_archives,
              archive_url))

  package_url = storage.PutFile(local_package_file, remote_package_file)
  if annotate:
    print ('@@@STEP_LINK@download (%s/%s.json)@%s@@@' %
           (package_target, package_name, package_url))
  return package_url


class PackageInfo(object):
  """A package file is a list of package archives (usually .tar or .tgz files).

  PackageInfo will contain a list of ArchiveInfo objects, ArchiveInfo will
  contain all the necessary information for an archive (name, URL, hash...etc.).
  """
  def __init__(self, package_file=None, skip_missing=False):
    self._archive_list = []
    self._package_version = CURRENT_PACKAGE_VERSION

    if package_file is not None:
      self.LoadPackageFile(package_file, skip_missing)

  def __eq__(self, other):
    if type(self) != type(other):
      return False
    elif self.GetPackageVersion() != other.GetPackageVersion():
      return False

    archives1 = [archive.GetArchiveData() for archive in self.GetArchiveList()]
    archives2 = [archive.GetArchiveData() for archive in other.GetArchiveList()]
    return set(archives1) == set(archives2)

  def __repr__(self):
    return 'PackageInfo(%s)' % self.DumpPackageJson()

  def LoadPackageFile(self, package_file, skip_missing=False):
    """Loads a package file into this object.

    Args:
      package_file: Filename or JSON dictionary.
    """
    archive_names = None
    self._archive_list = []

    # TODO(dyen): Support old format temporarily when it was a list of archives.
    if isinstance(package_file, list) or isinstance(package_file, dict):
      if isinstance(package_file, list):
        self._package_version = 0
        archive_list = package_file
      else:
        self._package_version = package_file[PACKAGE_KEY_VERSION]
        archive_list = package_file[PACKAGE_KEY_ARCHIVES]

      if archive_list:
        if isinstance(archive_list[0], archive_info.ArchiveInfo):
          # Setting a list of ArchiveInfo objects, no need to interpret JSON.
          self._archive_list = archive_list
        else:
          # Assume to be JSON.
          for archive_json in archive_list:
            archive = archive_info.ArchiveInfo(archive_info_file=archive_json)
            self._archive_list.append(archive)

    elif isinstance(package_file, basestring):
      package_data = ReadPackageFile(package_file)
      self._package_version = package_data[PACKAGE_KEY_VERSION]
      archive_names = package_data[PACKAGE_KEY_ARCHIVES]

      package_name = GetLocalPackageName(package_file)
      archive_dir = os.path.join(os.path.dirname(package_file), package_name)
      for archive in archive_names:
        arch_file = archive + '.json'
        arch_path = os.path.join(archive_dir, arch_file)
        if not os.path.isfile(arch_path):
          if not skip_missing:
            raise error.Error(
                'Package (%s) points to invalid archive file (%s).' %
                (package_file, arch_path))
          archive_desc = archive_info.ArchiveInfo(name=archive)
        else:
          archive_desc = archive_info.ArchiveInfo(archive_info_file=arch_path)
        self._archive_list.append(archive_desc)
    else:
      raise error.Error('Invalid load package file type (%s): %s.' %
                         (type(package_file), package_file))

  def SavePackageFile(self, package_file):
    """Saves this object as a serialized JSON file.

    Args:
      package_file: File path where JSON file will be saved.
    """
    package_name = GetLocalPackageName(package_file)
    archive_dir = os.path.join(os.path.dirname(package_file), package_name)
    pynacl.file_tools.RemoveDirectoryIfPresent(archive_dir)
    os.makedirs(archive_dir)

    archive_list = []

    for archive in self.GetArchiveList():
      archive_data = archive.GetArchiveData()
      archive_list.append(archive_data.name)

      archive_file = archive_data.name + '.json'
      archive_path = os.path.join(archive_dir, archive_file)
      archive.SaveArchiveInfoFile(archive_path)

    package_json = {
        PACKAGE_KEY_ARCHIVES: archive_list,
        PACKAGE_KEY_VERSION: self._package_version
        }

    with open(package_file, 'wt') as f:
      json.dump(package_json, f, sort_keys=True,
                indent=2, separators=(',', ': '))

  def DumpPackageJson(self):
    """Returns a dictionary representation of the JSON of this object."""
    archives = [archive.DumpArchiveJson() for archive in self.GetArchiveList()]
    return {
        PACKAGE_KEY_ARCHIVES: archives,
        PACKAGE_KEY_VERSION: self._package_version
        }

  def ClearArchiveList(self):
    """Clears this object so it represents no archives."""
    self._archive_list = []

  def AppendArchive(self, archive_info):
    """Append a package archive into this object"""
    self._archive_list.append(archive_info)

  def GetArchiveList(self):
    """Returns the sorted list of ARCHIVE_INFOs this object represents."""
    return sorted(self._archive_list,
                  key=lambda archive : archive.GetArchiveData().name)

  def GetPackageVersion(self):
    """Returns the version of this package."""
    return self._package_version
