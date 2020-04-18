# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

class FailureDatabase(object):
  """Logs the failures happening in the Clovis backend."""
  DIRTY_STATE_ERROR = 'startup_with_dirty_state'
  CRITICAL_ERROR = 'critical_error'

  def __init__(self, json_string=None):
    """Loads a FailureDatabase from a string returned by ToJsonString()."""
    self.is_dirty = False
    if json_string:
      self._failures_dict = json.loads(json_string)
    else:
      self._failures_dict = {}

  def ToJsonDict(self):
    """Returns a dict representing this instance."""
    return self._failures_dict

  def ToJsonString(self):
    """Returns a string representing this instance."""
    return json.dumps(self.ToJsonDict(), indent=2)

  def AddFailure(self, failure_name, failure_content=None):
    """Adds a failure with the given name and content. If the failure already
    exists, it will increment the associated count.
    Sets the 'is_dirty' bit to True.

    Args:
      failure_name (str): name of the failure.
      failure_content (str): content of the failure (e.g. the URL or task that
                             is failing).
    """
    self.is_dirty = True
    content = failure_content if failure_content else 'error_count'
    if failure_name not in self._failures_dict:
      self._failures_dict[failure_name] = {}
    error_count = self._failures_dict[failure_name].get(content, 0)
    self._failures_dict[failure_name][content] = error_count + 1

