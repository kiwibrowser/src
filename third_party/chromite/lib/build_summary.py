# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to manage local build state."""

from __future__ import print_function

import json

from chromite.lib import constants
from chromite.lib import cros_logging as logging


class BuildSummary(object):
  """Summarizes a build state without any external references.

  This is basically a dictionary that can convert itself back and forth from
  JSON, but it provides the convenience of attribute-based access instead of
  carrying around dictionary key constants.

  Attributes:
    build_number: The build number of this build, as passed in with
        --buildnumber, or 0 if --buildnumber wasn't passed.
    buildbucket_id: The buildbucket id of this build, as passed in with
        --buildbucket-id, or 0 if --buildbucket-id wasn't passed.
    master_build_id: The CIDB id of the associated master build if there is one,
        or 0 if there isn't one.
    status: One of the status constants from
        chromite.lib.constants.BUILDER_ALL_STATUSES
    buildroot_layout: Version of the buildroot layout.
    branch: Name of the branch this repository is associated with.
    distfiles_ts: Float unix timestamp recording the last time the distfiles
        cache was cleaned.
  """

  # List of attributes that should be saved and restored to represent
  # this object.  We use an explicit list instead of vars() so that future
  # additions can be handled explicitly.
  _PERSIST_ATTRIBUTES = ('build_number', 'buildbucket_id', 'master_build_id',
                         'status', 'buildroot_layout', 'branch', 'distfiles_ts')

  def __init__(self, build_number=0, buildbucket_id=0, master_build_id=0,
               status=constants.BUILDER_STATUS_MISSING,
               buildroot_layout=0, branch='', distfiles_ts=None):
    self.build_number = build_number
    self.buildbucket_id = buildbucket_id
    self.master_build_id = master_build_id
    self.status = status
    self.buildroot_layout = buildroot_layout
    self.branch = branch
    self.distfiles_ts = distfiles_ts

  def __eq__(self, other):
    for a in self._PERSIST_ATTRIBUTES:
      if hasattr(other, a) != hasattr(self, a):
        return False
      if getattr(other, a) != getattr(self, a):
        return False
    return True

  def __repr__(self):
    return 'BuildSummary(%s)' % self.to_json()

  def from_json(self, raw_json):
    """Merge the state encoded in |raw_json| into this object.

    Unknown keys will be ignored (with a warning).  Values for missing keys will
    remain unchanged.

    Args:
      raw_json: String containing valid JSON representing a BuildSummary.

    Raises:
      ValueError: |raw_json| is not valid JSON.
    """
    new_state = json.loads(raw_json)
    for key, val in new_state.iteritems():
      if key in self._PERSIST_ATTRIBUTES:
        setattr(self, key, val)
      else:
        logging.warning('Ignoring unrecognized JSON key "%s"', key)

    if self.status not in constants.BUILDER_ALL_STATUSES:
      logging.warning('Ingoring unknown build status "%s"', self.status)
      self.status = constants.BUILDER_STATUS_MISSING

  def to_json(self):
    """Serialize this object to JSON.

    Attributes that have an empty/zero value are omitted from the output.  The
    output of this function can be passed to from_json() to get back another
    BuildSummary with the same values.

    Returns:
      A string containing a JSON-encoded representation of this object.
    """
    state = {}
    for a in self._PERSIST_ATTRIBUTES:
      val = getattr(self, a)
      if val:
        state[a] = getattr(self, a)
    return json.dumps(state)

  def is_valid(self):
    """Indicate whether this object has a valid status."""
    return self.status != constants.BUILDER_STATUS_MISSING

  def build_description(self):
    """Get a human-readable description of which build id is set.

    If multiple fields are set, buildbucket is preferred.  The result can be
    used for logging or display, but should not be parsed for database lookups.

    Returns:
      String describing which build id is set, or "local build" if none are set.
    """
    if self.buildbucket_id:
      return 'buildbucket_id=%s' % self.buildbucket_id
    if self.build_number:
      return 'build_number=%s' % self.build_number
    return 'local build'
