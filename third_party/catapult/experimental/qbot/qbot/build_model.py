# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Build(object):
  def __init__(self, builder, data):
    self._builder = builder
    self._data = data

  def __str__(self):
    return '%s/%d' % (self.builder, self.number)

  @property
  def builder(self):
    return self._builder

  @property
  def number(self):
    return self._data['number']
