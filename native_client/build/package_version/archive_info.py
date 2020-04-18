#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A archive_info is a json file describing a single package archive."""

import collections
import hashlib
import json
import os

import error


ArchiveInfoTuple = collections.namedtuple(
    'ArchiveInfoTuple',
    ['name', 'hash', 'url', 'tar_src_dir', 'extract_dir', 'log_url'])

DEFAULT_ARCHIVE_INFO = ArchiveInfoTuple(name='',
                                        hash=0,
                                        url=None,
                                        tar_src_dir='',
                                        extract_dir='',
                                        log_url=None)


def GetArchiveHash(archive_file):
  """Gets the standardized hash value for a given archive.

  This hash value is the expected value used to verify package archives.

   Args:
     archive_file: Path to archive file to hash.
   Returns:
     Hash value of archive file, or None if file is invalid.
  """
  if os.path.isfile(archive_file):
    with open(archive_file, 'rb') as f:
      return hashlib.sha1(f.read()).hexdigest()
  return None


def ConstructArchiveInfo(default_archive_info=None, **archive_args):
  """Constructs an ArchiveInfoTuple using default values.

  This function fills in unspecified values using default values specified under
  default_archive_info. This function also ignores extra arguments so if a field
  is removed we can still properly load old files.
  """
  if default_archive_info is None:
    default_archive_info = DEFAULT_ARCHIVE_INFO

  archive_info_values = [(key, value) for key, value in archive_args.iteritems()
                          if key in ArchiveInfoTuple._fields]

  archive_info = default_archive_info._asdict()
  archive_info.update(archive_info_values)
  return ArchiveInfoTuple(**archive_info)


class ArchiveInfo(object):
  """An ArchiveInfo object represents information about an archive."""
  def __init__(self, archive_info_file=None, **archive_args):
    """Constructor for ArchiveInfo object.

    Args:
      archive_info_file: A JSON file representing an ArchiveInfo object.
      archive_args: Fields for an ArchiveInfoTuple when no file is specified.
    """
    self._archive_tuple = None

    if archive_info_file is not None:
      self.LoadArchiveInfoFile(archive_info_file)
    else:
      self.SetArchiveData(**archive_args)

  def __eq__(self, other):
    return (type(self) == type(other) and
            self.GetArchiveData() == other.GetArchiveData())

  def __repr__(self):
    return "ArchiveInfo(" + str(self._archive_tuple) + ")"

  def Copy(self, **archive_args):
    """Returns a copy of the ArchiveInfo object with fields overridden."""
    arch_tuple = ConstructArchiveInfo(default_archive_info=self._archive_tuple,
                                      **archive_args)
    return ArchiveInfo(**arch_tuple._asdict())

  def LoadArchiveInfoFile(self, archive_info_file):
    """Loads a archive info file JSON into this object.

    Args:
      archive_info_file: Filename or archive info json.
    """
    archive_json = None
    if isinstance(archive_info_file, dict):
      archive_json = archive_info_file
    elif isinstance(archive_info_file, basestring):
      with open(archive_info_file, 'rt') as f:
        archive_json = json.load(f)
    else:
      raise error.Error('Invalid load archive file type (%s): %s',
                        type(archive_info_file),
                        archive_info_file)

    self.SetArchiveData(**archive_json)

  def SaveArchiveInfoFile(self, archive_info_file):
    """Saves this object as a serialized JSON file if the object is valid.

    Args:
      archive_info_file: File path where JSON file will be saved.
    """
    if self._archive_tuple and self._archive_tuple.hash:
      archive_json = self.DumpArchiveJson()
      with open(archive_info_file, 'wt') as f:
        json.dump(archive_json, f, sort_keys=True,
                  indent=2, separators=(',', ': '))

  def DumpArchiveJson(self):
    """Returns a dict representation of this object for JSON."""
    if self._archive_tuple is None or not self._archive_tuple.hash:
      return {}

    return dict(self._archive_tuple._asdict())

  def SetArchiveData(self, **archive_args):
    """Replaces currently set archive data with fields from ArchiveInfoTuple."""
    self._archive_tuple = ConstructArchiveInfo(**archive_args)
    if not self._archive_tuple.name:
      raise RuntimeError('Invalid Archive Data - "name" is not set.')

  def GetArchiveData(self):
    """Returns the current ArchiveInfoTuple tuple."""
    return self._archive_tuple
