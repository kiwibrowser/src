# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urllib

from common.buildbot import builds
from common.buildbot import network


def Builders(master_name):
  builder_data = network.FetchData(network.BuildUrl(
      master_name, 'json/builders'))
  return sorted(Builder(master_name, builder_name, builder_info)
                for builder_name, builder_info in builder_data.iteritems())


class Builder(object):

  def __init__(self, master_name, name, data):
    self._master_name = master_name
    self._name = name
    self._url = network.BuildUrl(
        master_name, 'builders/%s' % urllib.quote(self.name))
    self._builds = builds.Builds(master_name, name, self._url)

    self.Update(data)

  def __lt__(self, other):
    return self.name < other.name

  def __str__(self):
    return self.name

  def Update(self, data=None):
    if not data:
      data = network.FetchData(network.BuildUrl(
          self.master_name, 'json/builders/%s' % urllib.quote(self.name)))
    self._state = data['state']
    self._pending_build_count = data['pendingBuilds']
    self._current_builds = frozenset(data['currentBuilds'])
    self._cached_builds = frozenset(data['cachedBuilds'])
    self._slaves = frozenset(data['slaves'])

  @property
  def master_name(self):
    return self._master_name

  @property
  def name(self):
    return self._name

  @property
  def url(self):
    return self._url

  @property
  def state(self):
    return self._state

  @property
  def builds(self):
    return self._builds

  @property
  def pending_build_count(self):
    return self._pending_build_count

  @property
  def current_builds(self):
    """Set of build numbers currently building.

    There may be multiple entries if there are multiple build slaves.
    """
    return self._current_builds

  @property
  def cached_builds(self):
    """Set of builds whose data are visible on the master in increasing order.

    More builds may be available than this.
    """
    return self._cached_builds

  @property
  def available_builds(self):
    return self.cached_builds - self.current_builds

  @property
  def last_build(self):
    """Last completed build."""
    return max(self.available_builds)

  @property
  def slaves(self):
    return self._slaves
