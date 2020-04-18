# Copyright 2015 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Wraps os, os.path and shutil functions to work around MAX_PATH on Windows."""

import __builtin__
import inspect
import os
import re
import shutil
import subprocess
import sys


if sys.platform == 'win32':


  import ctypes
  CreateSymbolicLinkW = ctypes.windll.kernel32.CreateSymbolicLinkW
  CreateSymbolicLinkW.argtypes = (
      ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
  CreateSymbolicLinkW.restype = ctypes.c_ubyte
  DeleteFile = ctypes.windll.kernel32.DeleteFileW
  DeleteFile.argtypes = (ctypes.c_wchar_p,)
  DeleteFile.restype = ctypes.c_bool
  GetFileAttributesW = ctypes.windll.kernel32.GetFileAttributesW
  GetFileAttributesW.argtypes = (ctypes.c_wchar_p,)
  GetFileAttributesW.restype = ctypes.c_uint
  RemoveDirectory = ctypes.windll.kernel32.RemoveDirectoryW
  RemoveDirectory.argtypes = (ctypes.c_wchar_p,)
  RemoveDirectory.restype = ctypes.c_bool

  _SUPPORTS_SYMLINKS = None

  def _supports_unprivileged_symlinks():
    """Returns True if the OS supports sane symlinks without any user privilege
    modification.

    Actively work around AppCompat version lie shim. This is kinda insane to
    shell out to figure out the real Windows version but this is ironically the
    the most reliable way. This code was inspired by _get_os_numbers() in
    //appengine/swarming/swarming_bot/api/platforms/win.py.
    """
    global _SUPPORTS_SYMLINKS
    if _SUPPORTS_SYMLINKS != None:
      return _SUPPORTS_SYMLINKS

    _SUPPORTS_SYMLINKS = False
    # Windows is lying to us until python adds to its manifest:
    #   <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    # and it doesn't.
    # So ask nicely to cmd.exe instead, which will always happily report the
    # right version. Here's some sample output:
    # - XP: Microsoft Windows XP [Version 5.1.2600]
    # - Win10: Microsoft Windows [Version 10.0.10240]
    # - Win7 or Win2K8R2: Microsoft Windows [Version 6.1.7601]
    try:
      out = subprocess.check_output(['cmd.exe', '/c', 'ver']).strip()
      match = re.search(r'\[Version (\d+\.\d+)\.(\d+)\]', out, re.IGNORECASE)
      if match:
        # That's a bit gross but good enough.
        major = float(match.group(1))
        if major > 10:
          _SUPPORTS_SYMLINKS = True
        elif major == 10:
          # https://blogs.windows.com/buildingapps/2016/12/02/symlinks-windows-10/
          _SUPPORTS_SYMLINKS = int(match.group(2)) >= 14971
    except (subprocess.CalledProcessError, OSError):
      # Catastrophic issue.
      pass
    return _SUPPORTS_SYMLINKS


  def extend(path):
    """Adds '\\\\?\\' when given an absolute path so the MAX_PATH (260) limit is
    not enforced.
    """
    assert os.path.isabs(path), path
    assert isinstance(path, unicode), path
    prefix = u'\\\\?\\'
    return path if path.startswith(prefix) else prefix + path


  def trim(path):
    """Removes '\\\\?\\' when receiving a path."""
    assert isinstance(path, unicode), path
    prefix = u'\\\\?\\'
    if path.startswith(prefix):
      path = path[len(prefix):]
    assert os.path.isabs(path), path
    return path


  def islink(path):
    """Proper implementation of islink() for Windows.

    The stdlib is broken.
    https://msdn.microsoft.com/library/windows/desktop/aa365682.aspx
    """
    FILE_ATTRIBUTE_REPARSE_POINT = 1024
    INVALID_FILE_ATTRIBUTES = 0xFFFFFFFF
    res = GetFileAttributesW(extend(path))
    if res == INVALID_FILE_ATTRIBUTES:
      return False
    return bool(res & FILE_ATTRIBUTE_REPARSE_POINT)


  def symlink(source, link_name):
    """Creates a symlink on Windows 7 and later.

    This function will only work once SeCreateSymbolicLinkPrivilege has been
    enabled. See file_path.enable_symlink().

    Useful material:
    CreateSymbolicLinkW:
      https://msdn.microsoft.com/library/windows/desktop/aa363866.aspx
    UAC and privilege stripping:
      https://msdn.microsoft.com/library/bb530410.aspx
    Privilege constants:
      https://msdn.microsoft.com/library/windows/desktop/bb530716.aspx
    Windows 10 and developer mode:
      https://blogs.windows.com/buildingapps/2016/12/02/symlinks-windows-10/
    """
    SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE = 2
    # TODO(maruel): This forces always creating absolute path symlinks.
    source = extend(source)
    flags = 1 if os.path.isdir(source) else 0
    if _supports_unprivileged_symlinks():
      # This enables support for this specific case:
      # - Windows 10 with build 14971 or later
      # - Admin account
      # - UAC enabled
      # - Developer mode enabled (not the default)
      #
      # In this specific case, file_path.enable_symlink() is unnecessary and the
      # following flag make it magically work.
      flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
    if not CreateSymbolicLinkW(extend(link_name), source, flags):
      # pylint: disable=undefined-variable
      raise WindowsError(
          u'symlink(%r, %r) failed: %s' %
            (source, link_name, ctypes.GetLastError()))


  def unlink(path):
    """Removes a symlink on Windows 7 and later.

    Does not delete the link source.

    If path is not a link, but a non-empty directory, will fail with a
    WindowsError.

    Useful material:
    CreateSymbolicLinkW:
      https://msdn.microsoft.com/library/windows/desktop/aa363866.aspx
    DeleteFileW:
      https://msdn.microsoft.com/en-us/library/windows/desktop/aa363915(v=vs.85).aspx
    RemoveDirectoryW:
      https://msdn.microsoft.com/en-us/library/windows/desktop/aa365488(v=vs.85).aspx
    """
    path = extend(path)
    if os.path.isdir(path):
      if not RemoveDirectory(path):
        # pylint: disable=undefined-variable
        raise WindowsError(
            u'unlink(%r): could not remove directory: %s' %
              (path, ctypes.GetLastError()))
    else:
      if not DeleteFile(path):
        # pylint: disable=undefined-variable
        raise WindowsError(
            u'unlink(%r): could not delete file: %s' %
              (path, ctypes.GetLastError()))


  def readlink(path):
    extend(path)
    raise NotImplementedError(
        'Implement readlink() via DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, '
        '...)')


  def walk(top, *args, **kwargs):
    return os.walk(extend(top), *args, **kwargs)


else:


  def extend(path):
    """Convert the path back to utf-8.

    In some rare case, concatenating str and unicode may cause a
    UnicodeEncodeError because the default encoding is 'ascii'.
    """
    assert os.path.isabs(path), path
    assert isinstance(path, unicode), path
    return path.encode('utf-8')


  def trim(path):
    """Path mangling is not needed on POSIX."""
    assert os.path.isabs(path), path
    assert isinstance(path, str), path
    return path.decode('utf-8')


  def islink(path):
    return os.path.islink(extend(path))


  def symlink(source, link_name):
    return os.symlink(source, extend(link_name))


  def unlink(path):
    return os.unlink(extend(path))


  def readlink(path):
    return os.readlink(extend(path)).decode('utf-8')


  def walk(top, *args, **kwargs):
    for root, dirs, files in os.walk(extend(top), *args, **kwargs):
      yield trim(root), dirs, files


## builtin


def open(path, *args, **kwargs):  # pylint: disable=redefined-builtin
  return __builtin__.open(extend(path), *args, **kwargs)


## os


def link(source, link_name):
  return os.link(extend(source), extend(link_name))


def rename(old, new):
  return os.rename(extend(old), extend(new))


def renames(old, new):
  return os.renames(extend(old), extend(new))


## shutil


def copy2(src, dst):
  return shutil.copy2(extend(src), extend(dst))


def rmtree(path, *args, **kwargs):
  return shutil.rmtree(extend(path), *args, **kwargs)


## The rest


def _get_lambda(func):
  return lambda path, *args, **kwargs: func(extend(path), *args, **kwargs)


def _is_path_fn(func):
  return (inspect.getargspec(func)[0] or [None]) == 'path'


_os_fns = (
  'access', 'chdir', 'chflags', 'chroot', 'chmod', 'chown', 'lchflags',
  'lchmod', 'lchown', 'listdir', 'lstat', 'mknod', 'mkdir', 'makedirs',
  'remove', 'removedirs', 'rmdir', 'stat', 'statvfs', 'unlink', 'utime')

_os_path_fns = (
  'exists', 'lexists', 'getatime', 'getmtime', 'getctime', 'getsize', 'isfile',
  'isdir', 'ismount')


for _fn in _os_fns:
  if hasattr(os, _fn):
    sys.modules[__name__].__dict__.setdefault(
        _fn, _get_lambda(getattr(os, _fn)))


for _fn in _os_path_fns:
  if hasattr(os.path, _fn):
    sys.modules[__name__].__dict__.setdefault(
        _fn, _get_lambda(getattr(os.path, _fn)))
