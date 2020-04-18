# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common python commands used by various internal build scripts."""

from __future__ import print_function

from contextlib import contextmanager
import os
import tempfile

from chromite.lib import cros_logging as logging


# Give preference to /usr/local/google/tmp for space reasons.
TMPS = ('/usr/local/google/tmp', '/tmp')


class UnableToCreateTmpDir(Exception):
  """Raised if we are unable to find a suitable tmp area."""


def CreateTmpDir(prefix='cros-rel', tmps=TMPS, minimum_size=0):
  """Return a unique tmp dir with enough free space (if specified).

  Check if any tmp in tmps exists that also meets the minimum_size
  free space requirement. If so, return a unique tmp dir in that path.

  Args:
    prefix: Prefix to use with tempfile.mkdtemp.
    tmps: An iterable of directories to consider for tmp space.
    minimum_size: The minimum size the tmp dir needs to have. Default: 0.

  Raises:
    UnableToCreateTmpDir: If we are unable to find a suitable tmp dir.
  """
  for entry in tmps:
    if os.path.exists(entry):
      if not minimum_size or GetFreeSpace(entry) > minimum_size:
        return tempfile.mkdtemp(prefix=prefix, dir=entry)
      else:
        logging.warning('Not enough space in %s to create %s temp dir.',
                        entry, prefix)

  raise UnableToCreateTmpDir('Unable to find a suitable %s tmp dir.'
                             '  Considered: %s', prefix, ', '.join(tmps))


def GetFreeSpace(path):
  """Return the available free space in bytes.

  Args:
    path: The dir path to check. If this is a file it will be converted to a
        path.

  Returns:
    The byte representation of available space.
  """
  if os.path.isfile(path):
    path = os.path.dirname(path)

  stats = os.statvfs(path)
  return stats.f_bavail * stats.f_frsize


def CreateTempFileWithContents(contents, base_dir=None):
  """Creates a temp file containing contents which self deletes when closed.

  Args:
    contents: The string to write into the temp file.
    base_dir: The directory which files are created.

  Returns:
    tempfile.NamedTemporaryFile. A file object that will self delete
    when closed.
  """
  message_file = tempfile.NamedTemporaryFile(dir=base_dir)
  message_file.write(contents)
  message_file.flush()
  return message_file


def ListdirFullpath(directory):
  """Return all files in a directory with full pathnames.

  Args:
    directory: directory to find files for.

  Returns:
    Full paths to every file in that directory.
  """
  return [os.path.join(directory, f) for f in os.listdir(directory)]


class RestrictedAttrDict(dict):
  """Define a dictionary which is also a struct.

  The keys will belong to a restricted list of values.
  """

  _slots = ()

  def __init__(self, *args, **kwargs):
    """Ensure that only the expected keys are added during initialization."""
    dict.__init__(self, *args, **kwargs)

    # Ensure all slots are at least populated with None.
    for key in self._slots:
      self.setdefault(key)

    for key in self.keys():
      assert key in self._slots, 'Unexpected key %s in %s' % (key, self._slots)

  def __setattr__(self, name, val):
    """Setting an attribute, actually sets a dictionary value."""
    if name not in self._slots:
      raise AttributeError("'%s' may not have attribute '%s'" %
                           (self.__class__.__name__, name))
    self[name] = val

  def __getattr__(self, name):
    """Fetching an attribute, actually fetches a dictionary value."""
    if name not in self:
      raise AttributeError("'%s' has no attribute '%s'" %
                           (self.__class__.__name__, name))
    return self[name]

  def __setitem__(self, name, val):
    """Restrict which keys can be stored in this dictionary."""
    if name not in self._slots:
      raise KeyError(name)
    dict.__setitem__(self, name, val)

  def __str__(self):
    """Default stringification behavior."""
    name = self._name if hasattr(self, '_name') else self.__class__.__name__
    return '%s (%s)' % (name, self._GetAttrString())

  def _GetAttrString(self, delim=', ', equal='='):
    """Return string showing all non-None values of self._slots.

    The ordering of attributes in self._slots is honored in string.

    Args:
      delim: String for separating key/value elements in result.
      equal: String to put between key and associated value in result.

    Returns:
      A string like "a='foo', b=12".
    """
    slots = [s for s in self._slots if self[s] is not None]
    elems = ['%s%s%r' % (s, equal, self[s]) for s in slots]
    return delim.join(elems)

  def _clear_if_default(self, key, default):
    """Helper for constructors.

    If they key value is set to the default value, set it to None.

    Args:
      key: Key value to check and possibly clear.
      default: Default value to compare the key value against.
    """
    if self[key] == default:
      self[key] = None


def PathPrepend(new_dir, curr_path=None):
  """Prepends a directory to a given path (or system path, if none provided)."""
  if curr_path is None:
    curr_path = os.environ.get('PATH')
  return '%s:%s' % (new_dir, curr_path) if curr_path else new_dir


@contextmanager
def CheckedOpen(name, mode=None, buffering=None):
  """A context for opening/closing a file iff an actual name is provided."""
  # Open the file, as necessary.
  f = None
  if name:
    dargs = {'name': name}
    if mode is not None:
      dargs['mode'] = mode
    if buffering is not None:
      dargs['buffering'] = buffering
    f = open(**dargs)

  try:
    # Yield to the wait-statement body.
    yield f
  finally:
    # If an actual file was opened, close it.
    if f:
      f.close()
