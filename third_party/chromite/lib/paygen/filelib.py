# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common local file interface library."""

from __future__ import print_function

import base64
import filecmp
import fnmatch
import hashlib
import os
import shutil

from chromite.lib import osutils


class MissingFileError(RuntimeError):
  """Raised when required file is missing."""


class MissingDirectoryError(RuntimeError):
  """Raised when required directory is missing."""


def Cmp(path1, path2):
  """Return True if paths hold identical files.

  If either file is missing then always return False.

  Args:
    path1: Path to a local file.
    path2: Path to a local file.

  Returns:
    True if files are the same, False otherwise.
  """
  return (os.path.exists(path1) and os.path.exists(path2) and
          filecmp.cmp(path1, path2))


def Copy(src_path, dest_path):
  """Copy one path to another.

  Automatically create the directory for dest_path, if necessary.

  Args:
    src_path: Path to local file to copy from.
    dest_path: Path to local file to copy to.
  """
  dest_dir = os.path.dirname(dest_path)
  if dest_dir and not Exists(dest_dir, as_dir=True):
    osutils.SafeMakedirs(dest_dir)

  shutil.copy2(src_path, dest_path)


def Size(path):
  """Return size of file in bytes.

  Args:
    path: Path to a local file.

  Returns:
    Size of file in bytes.

  Raises:
    MissingFileError if file is missing.
  """
  if os.path.isfile(path):
    return os.stat(path).st_size

  raise MissingFileError('No file at %r.' % path)


def Exists(path, as_dir=False):
  """Return True if file exists at given path.

  If path is a directory and as_dir is False then this will return False.

  Args:
    path: Path to a local file.
    as_dir: If True then check path as a directory, otherwise check as a file.

  Returns:
    True if file (or directory) exists at path, False otherwise.
  """
  if as_dir:
    return os.path.isdir(path)
  else:
    return os.path.isfile(path)


def Remove(*args, **kwargs):
  """Delete the file(s) at path_or_paths, or directory with recurse set.

  The first path to fail to be removed will abort the command, unless
  the failure is for a path that cannot be found and ignore_no_match is True.
  For example, if paths is [pathA, pathB, pathC] and pathB fails to be removed
  then pathC will also not be removed, but pathA will.

  Args:
    args: One or more paths to local files.
    ignore_no_match: If True, then do not complain if anything was not
      removed because no file was found at path.  Like rm -f.  Defaults to
      False.
    recurse: Remove recursively starting at path.  Same as rm -R.  Defaults
      to False.

  Returns:
    True if everything was removed, False if anything was not removed (which can
      only happen with no exception if ignore_no_match is True).

  Raises:
    MissingFileError if file is missing and ignore_no_match was False.
  """
  ignore_no_match = kwargs.pop('ignore_no_match', False)
  recurse = kwargs.pop('recurse', False)

  any_no_match = False

  for path in args:
    if os.path.isdir(path) and recurse:
      shutil.rmtree(path)
    elif os.path.exists(path):
      # Note that a directory path with recurse==False will call os.remove here,
      # which will fail, causing this function to fail.  As it should.
      os.remove(path)
    elif ignore_no_match:
      any_no_match = True
    else:
      raise MissingFileError('No file at %r.' % path)

  return not any_no_match


def ListFiles(root_path, recurse=False, filepattern=None, sort=False):
  """Return list of full file paths under given root path.

  Directories are intentionally excluded.

  Args:
    root_path: e.g. /some/path/to/dir
    recurse: Look for files in subdirectories, as well
    filepattern: glob pattern to match against basename of file
    sort: If True then do a default sort on paths.

  Returns:
    List of paths to files that matched
  """
  # Smoothly accept trailing '/' in root_path.
  root_path = root_path.rstrip('/')

  paths = []

  if recurse:
    # Recursively walk paths starting at root_path, filter for files.
    for entry in os.walk(root_path):
      dir_path, _, files = entry
      for file_entry in files:
        paths.append(os.path.join(dir_path, file_entry))

  else:
    # List paths directly in root_path, filter for files.
    for filename in os.listdir(root_path):
      path = os.path.join(root_path, filename)
      if os.path.isfile(path):
        paths.append(path)

  # Filter by filepattern, if specified.
  if filepattern:
    paths = [p for p in paths
             if fnmatch.fnmatch(os.path.basename(p), filepattern)]

  # Sort results, if specified.
  if sort:
    paths = sorted(paths)

  return paths


def CopyFiles(src_dir, dst_dir):
  """Recursively copy all files from src_dir into dst_dir

  Args:
    src_dir: directory to copy from.
    dst_dir: directory to copy into.

  Returns:
    A list of absolute path files for all copied files.
  """
  dst_paths = []
  src_paths = ListFiles(src_dir, recurse=True)
  for src_path in src_paths:
    dst_path = src_path.replace(src_dir, dst_dir)
    Copy(src_path, dst_path)
    dst_paths.append(dst_path)

  return dst_paths


def RemoveDirContents(base_dir):
  """Remove all contents of a directory.

  Args:
    base_dir: directory to delete contents of.
  """
  for obj_name in os.listdir(base_dir):
    Remove(os.path.join(base_dir, obj_name), recurse=True)


def MD5Sum(file_path):
  """Computer the MD5Sum of a file.

  Args:
    file_path: The full path to the file to compute the sum.

  Returns:
    A string of the md5sum if the file exists or
    None if the file does not exist or is actually a directory.
  """
  # For some reason pylint refuses to accept that md5 is a function in
  # the hashlib module, hence this pylint disable.
  # pylint: disable=E1101
  if not os.path.exists(file_path):
    return None

  if os.path.isdir(file_path):
    return None

  # Note that there is anecdotal evidence in other code that not using the
  # binary flag with this open (open(file_path, 'rb')) can malfunction.  The
  # problem has not shown up here, but be aware.
  md5_hash = hashlib.md5()
  with open(file_path) as file_fobj:
    for line in file_fobj:
      md5_hash.update(line)

  return md5_hash.hexdigest()


def ReadBlock(file_obj, size=1024):
  """Generator function to Read and return a specificed number of bytes.

  Args:
    file_obj: The file object to read data from
    size: The size in bytes to read in at a time.

  Yields:
    The block of data that was read.
  """
  while True:
    data = file_obj.read(size)
    if not data:
      break

    yield data


def ShaSums(file_path):
  """Calculate the SHA1 and SHA256 checksum of a file.

  Args:
    file_path: The full path to the file.

  Returns:
    A tuple of base64 encoded sha1 and sha256 hashes.
  """
  # pylint: disable=E1101
  sha1 = hashlib.sha1()
  sha256 = hashlib.sha256()
  with open(file_path, mode='r') as file_fobj:
    for block in ReadBlock(file_fobj):
      sha1.update(block)
      sha256.update(block)

  # Encode in base 64 string.  Other bases could be supported here.
  sha1_hex = base64.b64encode(sha1.digest())
  sha256_hex = base64.b64encode(sha256.digest())

  return sha1_hex, sha256_hex


def TruncateToSize(file_path, size):
  """Truncates a file down to a given size, if it is bigger.

  Args:
    file_path: path to the file to truncate
    size: the size to truncate down to, in bytes
  """
  if size < os.path.getsize(file_path):
    with open(file_path, 'r+') as file_obj:
      file_obj.truncate(size)
