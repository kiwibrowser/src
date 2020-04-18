# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from future import All, Future
from object_store import ObjectStore


class CacheChainObjectStore(ObjectStore):
  '''Maintains an in-memory cache along with a chain of other object stores to
  try for the same keys. This is useful for implementing a multi-layered cache.
  The in-memory cache is inbuilt since it's synchronous, but the object store
  interface is asynchronous.
  The rules for the object store chain are:
    - When setting (or deleting) items, all object stores in the hierarcy will
      have that item set.
    - When getting items, the behaviour depends on |start_empty|.
      - If false, each object store is tried in order. The first object
        store to find the item will trickle back up, setting it on all object
        stores higher in the hierarchy.
      - If true, only the first in-memory cache is checked, as though the store
        had been initialized with no content as opposed to the union of its
        delegate stores.
  '''
  def __init__(self, object_stores, start_empty=False):
    self._object_stores = object_stores
    self._start_empty = start_empty
    self._cache = {}

  def SetMulti(self, mapping):
    self._cache.update(mapping)
    return All([object_store.SetMulti(mapping)
                for object_store in self._object_stores])

  def GetMulti(self, keys):
    missing_keys = list(keys)
    cached_items = {}
    for key in keys:
      if key in self._cache:
        cached_items[key] = self._cache.get(key)
        missing_keys.remove(key)
    if len(missing_keys) == 0 or self._start_empty:
      return Future(value=cached_items)
    object_store_futures = [(object_store, object_store.GetMulti(missing_keys))
                            for object_store in self._object_stores]
    def resolve():
      # Approach:
      #
      # Try each object store in order, until there are no more missing keys.
      # Don't realise the Future value of an object store that we don't need to;
      # this is important e.g. to avoid querying data store constantly.
      #
      # When a value is found, cache it in all object stores further up the
      # chain, including the object-based cache on CacheChainObjectStore.
      object_store_updates = []
      for object_store, object_store_future in object_store_futures:
        if len(missing_keys) == 0:
          break
        result = object_store_future.Get()
        for k, v in result.items():  # use items(); changes during iteration
          if v is None or k not in missing_keys:
            del result[k]
            continue
          self._cache[k] = v
          cached_items[k] = v
          missing_keys.remove(k)
        for _, updates in object_store_updates:
          updates.update(result)
        object_store_updates.append((object_store, {}))
      # Update the caches of all object stores that need it.
      for object_store, updates in object_store_updates:
        if updates:
          object_store.SetMulti(updates)
      return cached_items
    return Future(callback=resolve)

  def DelMulti(self, keys):
    for k in keys:
      self._cache.pop(k, None)
    for object_store in self._object_stores:
      object_store.DelMulti(keys)
