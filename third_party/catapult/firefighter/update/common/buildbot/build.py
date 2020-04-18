# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.buildbot import step


class Build(object):

  def __init__(self, data, builder_url):
    self._number = data['number']
    self._slave_name = data['slave']
    self._status = data['results']
    self._start_time, self._end_time = data['times']
    self._url = '%s/builds/%d' % (builder_url, self._number)

    source_stamp = data['sourceStamp']
    if 'revision' in source_stamp:
      self._revision = source_stamp['revision']
    if 'changes' in source_stamp and source_stamp['changes']:
      self._revision_time = data['sourceStamp']['changes'][-1]['when']

    self._steps = tuple(step.Step(step_data, self._url)
                        for step_data in data['steps'])

  def __lt__(self, other):
    return self.number < other.number

  def __str__(self):
    return str(self.number)

  @property
  def number(self):
    return self._number

  @property
  def url(self):
    return self._url

  @property
  def slave_name(self):
    return self._slave_name

  @property
  def status(self):
    return self._status

  @property
  def complete(self):
    return self.status is not None

  @property
  def revision(self):
    return self._revision

  @property
  def revision_time(self):
    """The time the revision was committed.

    Warning: this field may not be populated.
    """
    return self._revision_time

  @property
  def start_time(self):
    return self._start_time

  @property
  def end_time(self):
    return self._end_time

  @property
  def steps(self):
    return self._steps
