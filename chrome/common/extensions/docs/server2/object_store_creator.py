# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from cache_chain_object_store import CacheChainObjectStore
from environment import GetAppVersion
from environment_wrappers import CreatePersistentObjectStore
from memcache_object_store import MemcacheObjectStore
from test_object_store import TestObjectStore

_unspecified = object()

class ObjectStoreCreator(object):
  '''Creates ObjectStores with a namespacing and behaviour configuration.

  The initial configuration is specified on object store construction. When
  creating ObjectStores via Create this configuration can be overridden (or
  via the variants of Create which do this automatically).
  '''
  def __init__(self,
               # TODO(kalman): rename start_dirty?
               start_empty=_unspecified,
               # Override for testing. A custom ObjectStore type to construct
               # on Create(). Useful with TestObjectStore, for example.
               store_type=None,
               # Override for testing. Whether the ObjectStore type specified
               # with |store_type| should be wrapped e.g. with Caching. This is
               # useful to override when specific state tests/manipulations are
               # being done on the underlying object store.
               disable_wrappers=False):
    if start_empty is _unspecified:
      raise ValueError('start_empty must be specified (typically False)')
    self._start_empty = start_empty
    self._store_type = store_type
    if disable_wrappers and store_type is None:
      raise ValueError('disable_wrappers much specify a store_type')
    self._disable_wrappers = disable_wrappers

  @staticmethod
  def ForTest(start_empty=False,
              store_type=TestObjectStore,
              disable_wrappers=True):
    return ObjectStoreCreator(start_empty=start_empty,
                              store_type=store_type,
                              disable_wrappers=disable_wrappers)

  def Create(self,
             cls,
             category=None,
             # Override any of these for a custom configuration.
             start_empty=_unspecified,
             app_version=_unspecified):
    # Resolve namespace components.
    if start_empty is not _unspecified:
      start_empty = bool(start_empty)
    else:
      start_empty = self._start_empty
    if app_version is _unspecified:
      app_version = GetAppVersion()

    # Reserve & and = for namespace separators.
    for component in (category, app_version):
      if component and ('&' in component or '=' in component):
        raise ValueError('%s cannot be used in a namespace')

    namespace = '&'.join(
        '%s=%s' % (key, value)
        for key, value in (('class', cls.__name__),
                           ('category', category),
                           ('app_version', app_version))
        if value is not None)

    if self._disable_wrappers:
      return self._store_type(namespace, start_empty=start_empty)

    if self._store_type is not None:
      chain = (self._store_type(namespace),)
    else:
      chain = (MemcacheObjectStore(namespace),
               CreatePersistentObjectStore(namespace))
    return CacheChainObjectStore(chain, start_empty=start_empty)
