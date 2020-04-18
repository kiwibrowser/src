# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Caches processed query results in memcache and datastore.

Memcache is not very reliable for the perf dashboard. Prometheus team explained
that memcache is LRU and shared between multiple applications, so their activity
may result in our data being evicted. To prevent this, we cache processed
query results in the data store. Using NDB, the values are also cached in
memcache if possible. This improves performance because doing a get()
for a key which has a single BlobProperty is much quicker than a complex query
over a large dataset.
(Background: http://g/prometheus-discuss/othVtufGIyM/wjAS5djyG8kJ)

When an item is cached, layered_cache does the following:
1) Namespaces the key based on whether datastore_hooks says the request is
internal_only.
2) Pickles the value (memcache does this internally), and adds a data store
entity with the key and a BlobProperty with the pickled value.

Retrieving values checks memcache via NDB first, and if datastore is used it
unpickles.

When an item is removed from the the cache, it is removed from both internal and
external caches, since removals are usually caused by large changes that affect
both caches.

Although this module contains ndb.Model classes, these are not intended
to be used directly by other modules.
"""

import cPickle
import datetime
import logging

from google.appengine.api import datastore_errors
from google.appengine.runtime import apiproxy_errors
from google.appengine.ext import ndb

from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import stored_object


class DeleteExpiredEntitiesHandler(request_handler.RequestHandler):
  """URL endpoint for a cron job to delete expired entities from datastore."""

  def get(self):
    """This get handler is called from cron.

    It deletes only expired CachedPickledString entities from the datastore.
    """
    DeleteAllExpiredEntities()


class CachedPickledString(ndb.Model):
  value = ndb.BlobProperty()
  expire_time = ndb.DateTimeProperty()

  @classmethod
  def GetExpiredKeys(cls):
    """Gets keys of expired entities.

    Returns:
      List of keys for items which are expired.
    """
    current_time = datetime.datetime.now()
    query = cls.query(cls.expire_time < current_time)
    query = query.filter(cls.expire_time != None)
    return query.fetch(keys_only=True)


def _NamespaceKey(key, namespace=None):
  if not namespace:
    namespace = datastore_hooks.GetNamespace()
  return '%s__%s' % (namespace, key)


def Get(key):
  """Gets the value from the datastore."""
  if key is None:
    return None
  namespaced_key = _NamespaceKey(key)
  entity = ndb.Key('CachedPickledString', namespaced_key).get(
      read_policy=ndb.EVENTUAL_CONSISTENCY)
  if entity:
    return cPickle.loads(entity.value)
  else:
    return stored_object.Get(key)


def GetExternal(key):
  """Gets the value from the datastore for the externally namespaced key."""
  if key is None:
    return None
  namespaced_key = _NamespaceKey(key, datastore_hooks.EXTERNAL)
  entity = ndb.Key('CachedPickledString', namespaced_key).get(
      read_policy=ndb.EVENTUAL_CONSISTENCY)
  if entity:
    return cPickle.loads(entity.value)
  else:
    return stored_object.Get(key)


def Set(key, value, days_to_keep=None, namespace=None):
  """Sets the value in the datastore.

  Args:
    key: The key name, which will be namespaced.
    value: The value to set.
    days_to_keep: Number of days to keep entity in datastore, default is None.
    Entity will not expire when this value is 0 or None.
    namespace: Optional namespace, otherwise namespace will be retrieved
        using datastore_hooks.GetNamespace().
  """
  # When number of days to keep is given, calculate expiration time for
  # the entity and store it in datastore.
  # Once the entity expires, it will be deleted from the datastore.
  expire_time = None
  if days_to_keep:
    expire_time = datetime.datetime.now() + datetime.timedelta(
        days=days_to_keep)
  namespaced_key = _NamespaceKey(key, namespace)

  try:
    CachedPickledString(id=namespaced_key,
                        value=cPickle.dumps(value),
                        expire_time=expire_time).put()
  except datastore_errors.BadRequestError as e:
    logging.warning('BadRequestError for key %s: %s', key, e)
  except apiproxy_errors.RequestTooLargeError as e:
    stored_object.Set(key, value)


def SetExternal(key, value, days_to_keep=None):
  """Sets the value in the datastore for the externally namespaced key.

  Needed for things like /add_point that update internal/external data at the
  same time.

  Args:
    key: The key name, which will be namespaced as externally_visible.
    value: The value to set.
    days_to_keep: Number of days to keep entity in datastore, default is None.
        Entity will not expire when this value is 0 or None.
  """
  Set(key, value, days_to_keep, datastore_hooks.EXTERNAL)


@ndb.synctasklet
def Delete(key):
  """Clears the value from the datastore."""
  yield DeleteAsync(key)


@ndb.tasklet
def DeleteAsync(key):
  internal_key = _NamespaceKey(key, namespace=datastore_hooks.INTERNAL)
  external_key = _NamespaceKey(key, namespace=datastore_hooks.EXTERNAL)
  yield (
      ndb.delete_multi_async(
          [ndb.Key('CachedPickledString', internal_key),
           ndb.Key('CachedPickledString', external_key)]),
      stored_object.DeleteAsync(key))


def DeleteAllExpiredEntities():
  """Deletes all expired entities from the datastore."""
  ndb.delete_multi(CachedPickledString.GetExpiredKeys())
