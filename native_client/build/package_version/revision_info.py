#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Encompasses a revision file of a package.

A revision file associates an SVN revision with a package. The revision
file will encompass the package information for all package targets. All
packages must have already been built by the buildbot before a revision file
can be made for it.
"""

import hashlib
import json

import archive_info
import error
import package_info
import packages_info


FIELD_PACKAGE_NAME = 'package_name'
FIELD_REVISION = 'revision'
FIELD_PACKAGE_TARGETS = 'package_targets'

FIELD_REVISION_HASH = 'revision_hash'


class RevisionInfo(object):
  """Revision information object describing a set revision for a package."""
  def __init__(self, packages_desc, revision_file=None):
    """Constructor for a RevisionInfo object.

    Args:
      packages_desc: Packages description containing all the necessary packages
                     and package targets to verify a revision file is complete.
      revision_file: Optional JSON file representing a RevisionInfo object.
    """
    assert isinstance(packages_desc, packages_info.PackagesInfo)
    self._packages_desc = packages_desc
    self._revision_num = None
    self._package_name = None

    # A revision describes all the package_info's for each package target.
    # Every package target in the package_desc must have its revision set
    # for the revision info to be valid.
    self._package_targets = {}

    if revision_file is not None:
      self.LoadRevisionFile(revision_file)

  def __eq__(self, other):
    return (type(self) == type(other) and
            self._revision_num == other._revision_num and
            self._package_name == other._package_name and
            self._package_targets == other._package_targets)

  def _GetRevisionHash(self):
    """Returns a stable hash for a revision file for validation purposes."""
    hash_string = str(self._revision_num)
    hash_string += str(self._package_name)
    for package_target in sorted(self._package_targets):
      package_desc = self._package_targets[package_target]
      archive_list = package_desc.GetArchiveList()

      hash_string += str(package_target)
      for archive in archive_list:
        for field, member in archive.GetArchiveData()._asdict().iteritems():
          hash_string += '[%s:%s]' % (field, member)

    return hashlib.sha1(hash_string).hexdigest()

  def _ValidateRevisionComplete(self):
    """Validate packages to make sure it matches the packages description."""
    if self._package_name is None:
      raise error.Error('Invalid revision information - '
                                  'no package name.')
    elif self._revision_num is None:
      raise error.Error('Invalid revision information - '
                                  'no revision identifier')

    package_targets = self._packages_desc.GetPackageTargetsForPackage(
        self._package_name
    )

    if package_targets:
      package_targets = set(package_targets)
      revision_targets = set(self._package_targets.keys())

      if package_targets != revision_targets:
        raise error.Error('Invalid revision information - '
                           'target mismatch:'
                           + '\n%s:' % self._package_name
                           + '\n  Required Target Packages:'
                           + '\n\t' + '\n\t'.join(sorted(package_targets))
                           + '\n  Supplied Target Packages:'
                           + '\n\t' + '\n\t'.join(sorted(revision_targets)))

  def LoadRevisionFile(self, revision_file, skip_hash_verify=False):
    """Loads a revision JSON file into this object.

    Args:
      revision_file: File name for a revision JSON file.
      skip_hash_verify: If True, will skip the hash validation check. This
                        should only be used if a field has been added or
                        removed in order to recalculate the revision hash.
    """
    try:
      with open(revision_file, 'rt') as f:
        revision_json = json.load(f)

      self._package_name = revision_json[FIELD_PACKAGE_NAME]
      self._revision_num = revision_json[FIELD_REVISION]
      self._package_targets = {}

      package_targets = revision_json[FIELD_PACKAGE_TARGETS]
      for package_target, archive_list in package_targets.iteritems():
        self._package_targets[package_target] = package_info.PackageInfo(
            archive_list
        )
    except (TypeError, KeyError) as e:
      raise error.Error('Invalid revision file [%s]: %s' %
                                  (revision_file, e))

    self._ValidateRevisionComplete()

    if not skip_hash_verify:
      hash_value = revision_json[FIELD_REVISION_HASH]
      if self._GetRevisionHash() != hash_value:
        raise error.Error('Invalid revision file [%s] - revision hash check '
            'failed' % revision_file)

  def SaveRevisionFile(self, revision_file):
    """Saves this object to a revision JSON file to be loaded later.

    Args:
      revision_file: File name where revision JSON file will be saved.
    """
    self._ValidateRevisionComplete()

    package_targets = {}
    for package_target, package_desc in self._package_targets.iteritems():
      package_targets[package_target] = package_desc.DumpPackageJson()

    revision_json = {
        FIELD_PACKAGE_NAME: self._package_name,
        FIELD_REVISION: self._revision_num,
        FIELD_PACKAGE_TARGETS: package_targets,
        FIELD_REVISION_HASH: self._GetRevisionHash()
    }

    with open(revision_file, 'wt') as f:
      json.dump(revision_json, f, sort_keys=True,
                indent=2, separators=(',', ': '))

  def SetRevisionNumber(self, revision_num):
    """Sets the current revision number for this object."""
    self._revision_num = revision_num

  def GetRevisionNumber(self):
    """Gets the currently set revision number for this object."""
    return self._revision_num

  def ClearRevisions(self):
    """Clears all package information for this object"""
    self._package_name = None
    self._package_targets = {}

  def SetTargetRevision(self, package_name, package_target, package_desc):
    """Sets a package description for a package target.

    The package description is a package_info object representing the package
    for this particular revision.

    Args:
      package_name: Name of the package this revision object represents.
      package_target: Package target name for the package we are setting.
      package_desc: package_info object representing the package target.
    """
    if self._package_name is None:
      self._package_name = package_name
    elif self._package_name != package_name:
      raise error.Error('Revision information must be all for the '
                        'same package\n'
                        'Original package name: %s\nNew package name: %s'
                        % (self._package_name, package_name))
    self._package_targets[package_target] = package_desc

  def GetPackageInfo(self, package_target):
    """Gets the package description for a particular package target.

    The package description is a package_info object representing the package
    for this particular revision.

    Args:
      package_target: Package target name for which we want the package info.

    Returns:
      A package_info object for the package target, or None for invalid targets.
    """
    return self._package_targets.get(package_target, None)
