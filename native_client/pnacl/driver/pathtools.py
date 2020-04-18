#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pathtools is meant to be a drop-in replacement for "os.path"
#
# All pathnames passed into the driver are "normalized" to
# a posix representation (with / as the separator). For example,
# on Windows, C:\foo\bar.c would become /cygdrive/c/foo/bar.c
# On all other platforms, pathnames are already in the correct form.
#
# This is convenient for two reasons:
# 1) All of the tools invoked by the driver expect this type
#    of pathname. (since on Windows, they are compiled with cygwin)
# 2) Everywhere in the driver, we can assume / is the path separator.
#
# Special functions:
#
#   pathtools.normalize: Convert an OS-style path into a normalized path
#   pathtools.tosys    : Convert a normalized path into an OS-style path
#   pathtools.touser   : Convert a normalized path into a representation
#                        suitable for presentation to the user.

import os
import platform
import posixpath

# This is only true when the driver is invoked on
# Windows, but outside of Cygwin.
WINDOWS_MANGLE = 'windows' in platform.system().lower()

def normalize(syspath):
  """ Convert an input path into a normalized path. """

  if WINDOWS_MANGLE:
    # Recognize paths which are already normalized.
    # (Should only happen during recursive driver calls)
    if '\\' not in syspath:
      return syspath

    return syspath.replace('\\', '/')
  else:
    return syspath

# All functions below expect a normalized path as input

def touser(npath):
  """ Convert a unix-style path into a user-displayable format """
  return tosys(npath)

def tosys(npath):
  """ Convert a normalized path into a system-style path """
  if WINDOWS_MANGLE:
    if npath.startswith('/cygdrive'):
      components = npath.split('/')
      assert(components[0] == '')
      assert(len(components[2]) == 1)
      drive = components[2]
      components = components[3:]
      return '%s:\\%s' % (drive.upper(), '\\'.join(components))
    else:
      # Work around for an issue that windows has opening long
      # relative paths.  http://bugs.python.org/issue4071
      npath = os.path.abspath(unicode(npath))
      return npath.replace('/', '\\')
  else:
    return npath

def join(*args):
  return posixpath.join(*args)

def exists(npath):
  return os.path.exists(tosys(npath))

def split(npath):
  return posixpath.split(npath)

def splitext(npath):
  return posixpath.splitext(npath)

def basename(npath):
  return posixpath.basename(npath)

def dirname(npath):
  return posixpath.dirname(npath)

def abspath(npath):
  if WINDOWS_MANGLE:
    # We always use absolute paths for (non-cygwin) windows
    return npath
  else:
    return posixpath.abspath(npath)

def normpath(npath):
  return posixpath.normpath(npath)

def isdir(npath):
  return os.path.isdir(tosys(npath))

def isfile(npath):
  return os.path.isfile(tosys(npath))

def getsize(npath):
  return os.path.getsize(tosys(npath))
