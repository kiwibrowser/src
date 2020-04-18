# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Represents a database of on-disk traces."""

import json


class LoadingTraceDatabase(object):
  def __init__(self, traces_dict):
    """traces_dict is a dictionary mapping filenames of traces to metadata
       about those traces."""
    self._traces_dict = traces_dict

  def SetTrace(self, filename, trace_dict):
    """Sets a mapping from |filename| to |trace_dict| into the database.
    If there is an existing mapping for filename, it is replaced.
    """
    self._traces_dict[filename] = trace_dict

  def GetTraceFilesForURL(self, url):
    """Given a URL, returns the set of filenames of traces that were generated
       for this URL."""
    trace_files = [f for f in self._traces_dict.keys()
        if self._traces_dict[f]["url"] == url]
    return trace_files

  def ToJsonDict(self):
    """Returns a dict representing this instance."""
    return self._traces_dict

  def ToJsonString(self):
    """Returns a string representing this instance."""
    return json.dumps(self._traces_dict, indent=2)

  def ToJsonFile(self, json_path):
    """Saves a json file representing this instance."""
    json_dict = self.ToJsonDict()
    with open(json_path, 'w') as output_file:
       json.dump(json_dict, output_file, indent=2)

  @classmethod
  def FromJsonDict(cls, json_dict):
    """Returns an instance from a dict returned by ToJsonDict()."""
    return LoadingTraceDatabase(json_dict)

  @classmethod
  def FromJsonString(cls, json_string):
    """Returns an instance from a string returned by ToJsonString()."""
    return LoadingTraceDatabase(json.loads(json_string))

  @classmethod
  def FromJsonFile(cls, json_path):
    """Returns an instance from a json file saved by ToJsonFile()."""
    with open(json_path) as input_file:
      return cls.FromJsonDict(json.load(input_file))
