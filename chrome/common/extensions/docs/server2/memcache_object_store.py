# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import traceback

from appengine_wrappers import memcache
from future import Future
from object_store import ObjectStore


class MemcacheObjectStore(ObjectStore):
  def __init__(self, namespace):
    self._namespace = namespace

  def SetMulti(self, mapping):
    # Some files are too big to fit in memcache, and will throw a ValueError if
    # we try. That's caught below, but to avoid log spew as much as possible
    # (and to try to store as many things as possible, since this is Set*Multi*
    # after all, and there may be other things to store from |mapping|), delete
    # the paths which we know don't work.
    #
    # TODO(kalman): Store big things (like example zips) in blobstore so that
    # this doesn't happen.
    log_spewers = ('assets/remote-debugging/remote-debug-banner.ai',)
    for spewer in log_spewers:
      mapping.pop(spewer, None)

    try:
      rpc = memcache.Client().set_multi_async(mapping,
                                              namespace=self._namespace)
      return Future(callback=rpc.get_result)
    except ValueError as e:
      logging.error('Caught error when memcache-ing keys %s: %s' %
                    (mapping.keys(), traceback.format_exc()))
      return Future(value=None)

  def GetMulti(self, keys):
    rpc = memcache.Client().get_multi_async(keys, namespace=self._namespace)
    return Future(callback=rpc.get_result)

  def DelMulti(self, keys):
    memcache.delete_multi(keys, namespace=self._namespace)
