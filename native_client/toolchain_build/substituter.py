#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import multiprocessing
import os
import sys


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)


def AbsPath(path, cwd):
  """Absolutize a possibly-relative path.

  If 'path' is a relative path, return an absolute path (assuming
  it is relative to 'cwd'). Otherwise return 'path' unchanged.
  """
  if os.path.isabs(path):
    return path
  return os.path.normpath(os.path.join(os.path.normpath(cwd), path))


def FixPath(path):
  """Convert to msys paths on windows."""
  if sys.platform != 'win32':
    return path
  drive, path = os.path.splitdrive(path)
  # Replace X:\... with /x/....
  # Msys does not like x:\ style paths (especially with mixed slashes).
  if drive:
    drive = '/' + drive.lower()[0]
  path = drive + path
  path = path.replace('\\', '/')
  return path


class Substituter(object):
  """Utility class to perform variable substitution and path resolution."""
  def __init__(self, cwd, paths, nonpaths):
    """Construct a substituter and create variable mappings.

    Args:
      cwd: The path against which relative paths will be resolved.
      paths: A mapping where the keys are variable names, and the values are
             paths. For each key foo, 2 variables are created: 'foo' will map to
             a relative path (from cwd to the given path value), and 'abs_foo'
             will map to its absolute path.
      nonpaths: A mapping for nonpath values, which will simply create variables
                corresponding to its keys and values.
    """
    self._paths = paths.copy()
    for key, value in self._paths.iteritems():
      if key.startswith('abs_'):
        raise Exception('Invalid key starts with "abs_": %s' % key)
    self._nonpaths = nonpaths.copy()
    self._cwd = cwd
    self._SetUpCommonNonPaths()
    self.SetUpPathVariables()

  def SetCwd(self, path):
    """Change the working directory against which paths are resolved."""
    self._cwd = path
    self.SetUpPathVariables()

  def SetUpPathVariables(self):
    """Create the internal mappings for all path variables.

    Paths are calculated relative to the current working directory.
    """
    self._variables = self._nonpaths.copy()
    self._variables['cwd'] = FixPath(os.path.abspath(self._cwd))
    for key, value in self._paths.iteritems():
      if value:
        abs_path = FixPath(os.path.abspath(value))
        key_path = FixPath(os.path.relpath(value, self._cwd))
      else:
        abs_path = value
        key_path = value

      self._variables['abs_' + key] = abs_path
      self._variables[key] = key_path

  def Substitute(self, template):
    """ Substitute the %-variables in 'template' """
    return template % self._variables

  def SubstituteAbsPaths(self, template):
    """Substitute the %-variables in 'template'.

    All variables in the template must be paths, and if the values are relative
    paths, they are resolved relative to cwd and returned as absolute paths.
    """
    path = self.Substitute(template)
    if path:
      path = AbsPath(path, self._cwd)
    return path

  def _SetUpCommonNonPaths(self):
    try:
      self._nonpaths['cores'] = multiprocessing.cpu_count()
    except NotImplementedError:
      self._nonpaths['cores'] = 4  # Assume 4 if we can't measure.
