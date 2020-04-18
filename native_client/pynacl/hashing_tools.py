#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Hashing related operations.

Provides for hashing files or directory trees.
Timestamps, archive order, and dot files/directories are ignored to keep
hashes stable.
"""

import hashlib
import os


def HashFileContents(filename):
  """Return the hash (sha1) of tthe contents of a file.

  Args:
    filename: Filename to read.
  Returns:
    The sha1 of a file.
  """
  hasher = hashlib.sha1()
  fh = open(filename, 'rb')
  try:
    while True:
      data = fh.read(4096)
      if not data:
        break
      hasher.update(data)
  finally:
    fh.close()
  return hasher.hexdigest()


def StableHashPath(path):
  """Hash (sha1) everything in a path in a stable (reproducible) way.

  Dot files and timestamps are ignored.
  Args:
    path: Path to hash.
  Returns:
    The sha1 of the file/directory.
  """
  hasher = hashlib.sha1()

  if os.path.isfile(path):
    hasher.update('singlefile:' + HashFileContents(path))
    return hasher.hexdigest()

  def RemoveExcludedPaths(paths):
    for p in [p for p in paths if p.startswith('.')]:
      paths.remove(p)

  for root, dirs, files in os.walk(path):
    dirs.sort()
    files.sort()
    RemoveExcludedPaths(dirs)
    RemoveExcludedPaths(files)
    # Including directory names in the hash so that
    # empty directories do count.
    # Also, as a side effect, all of the relative path components
    # are incorporated.
    # Terminating with \x00 to avoid injection attacks.
    for d in dirs:
      hasher.update('dir:' + d + '\x00')
    for f in files:
      hasher.update('filename:' + f + '\x00')
      hasher.update('contents:' + HashFileContents(
          os.path.join(root, f)))

  return hasher.hexdigest()
