# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module used for run-time determination of toplogy information.

By Toplogy, we mean a specification of which external server dependencies are
located where. At the moment, this module provides a default key-value api via
the |topology| member, and a cidb-backed store to provide environment-specific
overrides of the default values.
"""

from __future__ import print_function

import collections

BUILDBUCKET_HOST_KEY = '/buildbucket/host'
DATASTORE_WRITER_CREDS_KEY = '/datastore/creds_file'
SWARMING_PROXY_HOST_KEY = '/swarming_proxy/host'
LUCI_LOGDOG_HOST_KEY = '/luci-logdog/host'
LUCI_MILO_HOST_KEY = '/luci-milo/host'
SHERIFFOMATIC_HOST_KEY = '/sheriffomatic/host'

TOPOLOGY_DEFAULTS = {
    SWARMING_PROXY_HOST_KEY: 'fake_swarming_server',
    LUCI_LOGDOG_HOST_KEY: 'luci-logdog.appspot.com',
    LUCI_MILO_HOST_KEY: 'luci-milo.appspot.com',
    SHERIFFOMATIC_HOST_KEY: 'sheriff-o-matic-staging.appspot.com',
}


class LockedDictAccessException(Exception):
  """Raised when attempting to access a locked dict."""


class LockedDefaultDict(collections.defaultdict):
  """collections.defaultdict which cannot be read from until unlocked."""

  def __init__(self):
    super(LockedDefaultDict, self).__init__()
    self._locked = True

  def get(self, key):
    if self._locked:
      raise LockedDictAccessException()
    return super(LockedDefaultDict, self).get(key)

  def unlock(self):
    self._locked = False


topology = LockedDefaultDict()
topology.update(TOPOLOGY_DEFAULTS)


def FetchTopologyFromCIDB(db):
  """Update and unlock topology based on cidb-backed keyval store.

  Args:
    db: cidb.CIDBConnection instance for database to fetch keyvals from,
        or None.
  """
  if db:
    topology.update(db.GetKeyVals())

  topology.unlock()
