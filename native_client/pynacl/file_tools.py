#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Convience file system related operations."""


import os
import platform
import shutil
import stat
import sys
import tempfile
import time


def AtomicWriteFile(data, filename):
  """Write a file atomically.

  NOTE: Not atomic on Windows!
  Args:
    data: String to write to the file.
    filename: Filename to write.
  """
  filename = os.path.abspath(filename)
  handle, temp_file = tempfile.mkstemp(
      prefix='atomic_write', suffix='.tmp',
      dir=os.path.dirname(filename))
  fh = os.fdopen(handle, 'wb')
  fh.write(data)
  fh.close()
  # Window's can't move into place atomically, delete first.
  if sys.platform in ['win32', 'cygwin']:
    try:
      os.remove(filename)
    except OSError:
      pass
  Retry(os.rename, temp_file, filename)


def WriteFile(data, filename):
  """Write a file in one step.

  Args:
    data: String to write to the file.
    filename: Filename to write.
  """
  fh = open(filename, 'wb')
  fh.write(data)
  fh.close()


def ReadFile(filename):
  """Read a file in one step.

  Args:
    filename: Filename to read.
  Returns:
    String containing complete file.
  """
  fh = open(filename, 'rb')
  data = fh.read()
  fh.close()
  return data


class ExecutableNotFound(Exception):
  pass


def Which(command, paths=None, require_executable=True):
  """Find the absolute path of a command in the current PATH.

  Args:
    command: Command name to look for.
    paths: Optional paths to search.
  Returns:
    Absolute path of the command (first one found),
    or default to a bare command if nothing is found.
  """
  if paths is None:
    paths = os.environ.get('PATH', '').split(os.pathsep)
  exe_suffixes = ['']
  if sys.platform == 'win32':
    exe_suffixes += ['.exe']
  for p in paths:
    np = os.path.abspath(os.path.join(p, command))
    for suffix in exe_suffixes:
      full_path = np + suffix
      if (os.path.isfile(full_path) and
          (not require_executable or os.access(full_path, os.X_OK))):
        return full_path
  raise ExecutableNotFound('Unable to find: ' + command)


def MakeDirectoryIfAbsent(path):
  """Create a directory if it doesn't already exist.

  Args:
    path: Directory to create.
  """
  if not os.path.isdir(path):
    os.makedirs(path)


def MakeParentDirectoryIfAbsent(path):
  """Creates a directory for the parent if it doesn't already exist.

  Args:
    path: Path of child where parent directory should be created for.
  """
  abs_path = os.path.abspath(path)
  MakeDirectoryIfAbsent(os.path.dirname(abs_path))


def RemoveDirectoryIfPresent(path):
  """Remove a directory if it exists.

  Args:
    path: Directory to remove.
  """

  # On POSIX systems, attempts to remove a file fail if the containing
  # directory lacks write and execute (search) permissions.  So change
  # the directory permissions before trying again.
  def make_parents_accessible(path):
    did_anything = False
    while not os.access(path, os.F_OK):
      path = os.path.dirname(path)
      if os.path.exists(path) and not os.access(path, os.W_OK | os.X_OK):
        os.chmod(path, stat.S_IRWXU)
        did_anything = True
    return did_anything

  # On Windows, attempts to remove read-only files get Error 5.
  # This error handler fixes the permissions and retries the removal.
  def onerror_readonly(func, path, exc_info):
    if make_parents_accessible(path) or not os.access(path, os.W_OK):
      os.chmod(path, stat.S_IWUSR)
      func(path)

  if os.path.exists(path):
    Retry(shutil.rmtree, path, onerror=onerror_readonly)


def CopyTree(src, dst):
  """Recursively copy the items in the src directory to the dst directory.

  Unlike shutil.copytree, the destination directory and any subdirectories and
  files may exist. Existing directories are left untouched, and existing files
  are removed and copied from the source using shutil.copy2. It is also not
  symlink-aware.

  Args:
    src: Source. Must be an existing directory.
    dst: Destination directory. If it exists, must be a directory. Otherwise it
         will be created, along with parent directories.
  """
  if not os.path.isdir(dst):
    os.makedirs(dst)
  for root, dirs, files in os.walk(src):
    relroot = os.path.relpath(root, src)
    dstroot = os.path.join(dst, relroot)
    for d in dirs:
      dstdir = os.path.join(dstroot, d)
      if not os.path.isdir(dstdir):
        os.mkdir(dstdir)
    for f in files:
      dstfile = os.path.join(dstroot, f)
      if os.path.isfile(dstfile):
        Retry(os.remove, dstfile)
      shutil.copy2(os.path.join(root, f), dstfile)


def MoveAndMergeDirTree(src_dir, dest_dir):
  """Moves everything from a source directory to a destination directory.

  This is different from shutil's move implementation in that it only operates
  on directories, and if the destination directory exists, it will move the
  contents into the directory and merge any existing directories.

  Args:
    src_dir: Source directory which files should be moved from.
    dest_dir: Destination directory where files should be moved and merged to.
  """
  if not os.path.isdir(src_dir):
    raise OSError('MoveAndMergeDirTree can only operate on directories.')

  if not os.path.exists(dest_dir):
    # Simply move the directory over if destination doesn't exist.
    MakeParentDirectoryIfAbsent(dest_dir)
    Retry(os.rename, src_dir, dest_dir)
  else:
    # Merge each item if destination directory exists.
    for dir_item in os.listdir(src_dir):
      source_item = os.path.join(src_dir, dir_item)
      destination_item = os.path.join(dest_dir, dir_item)
      if os.path.islink(destination_item):
        Retry(os.unlink, destination_item)
      if os.path.exists(destination_item):
        if os.path.isdir(destination_item) and os.path.isdir(source_item):
          # Merge the sub-directories together if they are both directories.
          MoveAndMergeDirTree(source_item, destination_item)
        elif os.path.isfile(destination_item) and os.path.isfile(source_item):
          # Overwrite the file if they are both files.
          Retry(os.unlink, destination_item)
          Retry(os.rename, source_item, destination_item)
        else:
          raise OSError('Cannot move directory tree, mismatching types.'
                        ' Source - %s. Destination - %s' %
                        (source_item, destination_item))
      else:
        Retry(os.rename, source_item, destination_item)

    # Remove the directory once all the contents have been moved
    if os.path.islink(src_dir):
      Retry(os.unlink, src_dir)
    else:
      Retry(os.rmdir, src_dir)


def Retry(op, *args, **kwargs):
  # Windows seems to be prone to having commands that delete files or
  # directories fail.  We currently do not have a complete understanding why,
  # and as a workaround we simply retry the command a few times.
  # It appears that file locks are hanging around longer than they should.  This
  # may be a secondary effect of processes hanging around longer than they
  # should.  This may be because when we kill a browser sel_ldr does not exit
  # immediately, etc.
  # Virus checkers can also accidently prevent files from being deleted, but
  # that shouldn't be a problem on the bots.
  if platform.IsWindows():
    count = 0
    while True:
      try:
        op(*args, **kwargs)
        break
      except Exception:
        sys.stdout.write('FAILED: %s %s %s\n' % (
            op.__name__, repr(args), repr(kwargs)))
        count += 1
        if count < 5:
          sys.stdout.write('RETRYING\n')
          time.sleep(pow(2, count))
        else:
          # Don't mask the exception.
          raise
  else:
    op(*args, **kwargs)


def MoveDirCleanly(src, dst):
  RemoveDirectoryIfPresent(dst)
  MoveDir(src, dst)


def MoveDir(src, dst):
  Retry(shutil.move, src, dst)


def RemoveFile(path):
  if os.path.exists(path):
    Retry(os.unlink, path)
