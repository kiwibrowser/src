# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import base64
import json

from qbot import api
from qbot import build_model


class Builder(object):
  def __init__(self, master, name):
    self._master = master
    self._name = name

  def __str__(self):
    return '%s/%s' % (self.master, self.name)

  @property
  def master(self):
    return self._master

  @property
  def name(self):
    return self._name

  def IterBuilds(self, limit=None):
    """Iterate over some recent builds of this builder."""
    params = {'master': self.master, 'builder': self.name}
    if limit is not None:
      params['limit'] = limit
    resp = api.MiloRequest('GetBuildbotBuildsJSON', params)
    builds = resp['builds']
    if not builds:
      raise LookupError('No builds found')
    for build in builds:
      build_data = json.loads(base64.b64decode(build['data']).decode('utf-8'))
      yield build_model.Build(self, build_data)
