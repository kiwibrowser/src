# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Select an Android build, and download symbols for it."""

from __future__ import print_function

import json

from chromite.lib import osutils


class ProcessedBuildsStorage(object):
  """A context manager for storing processed builds.

  This is a context manager that loads recent builds, and allows them to be
  manipulated, and then saves them on exit. Processed builds are stored per
  branch/target as a list of integers.
  """
  def __init__(self, filename):
    self.filename = filename
    self.value = self._read()

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    self._write(self.value)

  def _read(self):
    """Load from disk, and default to an empty store on error."""
    try:
      return json.loads(osutils.ReadFile(self.filename))
    except (ValueError, IOError):
      # If there was no file, or it was corrupt json, return default.
      return {}

  def _write(self, new_value):
    """Write the current store to disk."""
    return osutils.WriteFile(self.filename,
                             json.dumps(new_value, sort_keys=True))

  def GetProcessedBuilds(self, branch, target):
    """Get a list of builds for a branch/target.

    Args:
      branch: Name of branch as a string.
      target: Name of target as a string.

    Returns:
      List of integers associated with the given branch/target.
    """
    self.value.setdefault(branch, {})
    self.value[branch].setdefault(target, [])
    return self.value[branch][target]

  def PurgeOldBuilds(self, branch, target, retain_list):
    """Removes uninteresting builds for a branch/target.

    Any build ids not in the retain list are removed.

    Args:
      branch: Name of branch as a string.
      target: Name of target as a string.
      retain_list: List of build ids that are still relevent.
    """
    processed = set(self.GetProcessedBuilds(branch, target))
    retained_processed = processed.intersection(retain_list)
    self.value[branch][target] = list(retained_processed)

  def AddProcessedBuild(self, branch, target, build_id):
    """Adds build_id to list for a branch/target.

    It's safe to add a build_id that is already present.

    Args:
      branch: Name of branch as a string.
      target: Name of target as a string.
      build_id: build_id to add, as an integer.
    """
    processed = set(self.GetProcessedBuilds(branch, target))
    processed.add(build_id)
    self.value[branch][target] = sorted(processed)
