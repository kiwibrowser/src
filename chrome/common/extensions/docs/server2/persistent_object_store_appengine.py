# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import google.appengine.ext.db as db

from datastore_models import PersistentObjectStoreItem
from environment import IsDevServer
from future import All, Future
from object_store import ObjectStore


class PersistentObjectStoreAppengine(ObjectStore):
  '''Stores or retrieves persistent data using the AppEngine Datastore API.
  '''
  def __init__(self, namespace):
    self._namespace = namespace

  def SetMulti(self, mapping):
    entities = [PersistentObjectStoreItem.CreateItem(
                    self._namespace, key, value)
                for key, value in mapping.iteritems()]
    # Some entites may be None if they were too large to insert. Skip those.
    rpcs = [db.put_async(entity for entity in entities if entity)]
    # If running the dev server, the futures don't complete until the server is
    # *quitting*. This is annoying. Flush now.
    if IsDevServer():
      [rpc.wait() for rpc in rpcs]
    return All(Future(callback=lambda: rpc.get_result()) for rpc in rpcs)

  def GetMulti(self, keys):
    db_futures = dict((k, db.get_async(
        PersistentObjectStoreItem.CreateKey(self._namespace, k)))
        for k in keys)
    def resolve():
      return dict((key, future.get_result().GetValue())
                  for key, future in db_futures.iteritems()
                  if future.get_result() is not None)
    return Future(callback=resolve)

  def DelMulti(self, keys):
    futures = []
    for key in keys:
      futures.append(db.delete_async(
          PersistentObjectStoreItem.CreateKey(self._namespace, key)))
    # If running the dev server, the futures don't complete until the server is
    # *quitting*. This is annoying. Flush now.
    if IsDevServer():
      [future.wait() for future in futures]
