#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This library keeps track of all the standard locations for package files"""

import os
import posixpath

SHARED_FOLDER = 'shared'
ARCHIVE_DIR = 'package_archives'


def GetRemotePackageKey(is_shared, rev_num, package_target, package_name):
  """Returns key for package files in the google storage cloud.

  Args:
    is_shared: Whether or not the package is marked as shared.
    rev_num: The revision identifier of when the package was built.
    package_target: The package target which this package belongs to.
    package_name: The name of the package.
  Returns:
    The google cloud storage key where the package file should be found.
  """
  if is_shared:
    intermediate_dir = SHARED_FOLDER
  else:
    intermediate_dir = package_target

  return posixpath.join('builds',
                        str(rev_num),
                        intermediate_dir,
                        package_name + '.json')


def GetRemotePackageArchiveKey(archive_name, hash_value):
  """Returns key for package archive files in the google storage cloud.

  Args:
    archive_name: The name of the archive.
    hash_value: The hash of the archive.
  Returns:
    The google cloud storage key where the package archive should be found.
  """
  return posixpath.join('archives',
                        archive_name,
                        hash_value)


def GetLocalPackageFile(tar_dir, package_target, package_name):
  """Returns the local package file location.

  Args:
    tar_dir: The tar directory for where package archives would be found.
    package_target: The package target of the package.
    package_name: The name of the package.
  Returns:
    The standard location where local package file is found.
  """
  return os.path.join(tar_dir,
                      package_target,
                      package_name + '.json')


def GetArchiveExtension(archive_name):
  """Returns the extension of an archive.

  Note that the archive extension is different from how os.path.splitext splits
  extensions. The standard python one splits on the last period, while this one
  will split on the first period.

  Args:
    archive_name: The name of the archive.
  Returns:
    The extension of the archive.
  """
  name_split = archive_name.split('.', 1)
  if len(name_split) == 2:
    return '.' + name_split[1]
  return ''


def GetLocalPackageArchiveDir(tar_dir, archive_name):
  """Returns directory where local package archive files live.

  Args:
    tar_dir: The tar root directory for where package archives would be found.
    archive_name: The name of the archive contained within the package.
  Returns:
    The standard location where local package archive files are found.
  """
  return os.path.join(tar_dir,
                      ARCHIVE_DIR,
                      archive_name)


def GetLocalPackageArchiveFile(tar_dir, archive_name, archive_hash):
  """Returns the local package archive file location.

  Args:
    tar_dir: The tar root directory for where package archives would be found.
    archive_name: The name of the archive contained within the package.
    archive_hash: The hash of the archive, which will be part of the final name.
  Returns:
    The standard location where local package archive file is found.
  """
  if isinstance(archive_hash, (int, long)):
    archive_hash = '%040x' % archive_hash

  archive_directory = GetLocalPackageArchiveDir(tar_dir, archive_name)

  # Have the file keep the extension so that extractions know the file type.
  archive_filename = archive_hash + GetArchiveExtension(archive_name)

  return os.path.join(archive_directory, archive_filename)


def GetLocalPackageArchiveLogFile(archive_file):
  """Returns the local package archive log file location.

  Args:
    archive_file: Location of the local archive file location.
  Returns:
    The standard location where local package archive log file is found.
  """
  return os.path.splitext(archive_file)[0] + '.log'


def GetRevisionFile(revision_dir, package_name):
  """Returns the local revision file location.

  Args:
    revision_dir: Revision directory where revision files should be found.
    package_name: The name of the package revision file represents.
  Returns:
    The standard location where the revision file is found.
  """
  return os.path.join(revision_dir,
                      package_name + '.json')


def GetFullDestDir(dest_dir, package_target, package_name):
  """Returns the destination directory for a package archive.

  Args:
    dest_dir: Destination directory root.
    package_target: Package target of the package to extract.
    package_name: The package name of the package to extract.
  Returns:
    The package directory within the destination directory.
  """
  return os.path.join(dest_dir, package_target, package_name)


def GetDestPackageFile(dest_dir, package_target, package_name):
  """Returns the package file stored in the destination directory.

  The package file is normally stored inside of the tar directory, but upon
  extraction a copy of the package file is also made into the extraction
  directory for book keeping purposes.

  Args:
    dest_dir: Destination directory root.
    package_target: Package target of the package to extract.
    package_name: The package name of the package to extract.
  Returns:
    The location of the package file within the destination directory.
  """

  return os.path.join(GetFullDestDir(dest_dir, package_target, package_name),
                      package_name + '.json')


def WalkPackages(tar_dir):
  """Generator for local package target packages within a root tar directory.

  Use this generator to walk through the list of package targets and their
  respective packages found within a local tar directory. This function does
  not guarantee that these are valid package targets or packages, so it could
  yield invalid names for malformed tar directories.

  Args:
    tar_dir: The tar root directory where package archives would be found.

  Yields:
    Tuple containing (package_target, [list-of package_names]).
  """
  if os.path.isdir(tar_dir):
    for package_target_dir in os.listdir(tar_dir):
      # Skip the package archive directory
      if package_target_dir == ARCHIVE_DIR:
        continue

      full_package_target_dir = os.path.join(tar_dir, package_target_dir)
      if os.path.isdir(full_package_target_dir):
        packages = [os.path.splitext(package_name)[0]
                    for package_name in os.listdir(full_package_target_dir)
                    if package_name.endswith('.json') and
                       os.path.isfile(os.path.join(full_package_target_dir,
                                                   package_name))]
        yield (package_target_dir, packages)
