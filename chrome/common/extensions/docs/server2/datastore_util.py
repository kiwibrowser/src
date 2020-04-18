# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cPickle
import logging

from future import Future
from gcloud import datastore

# N.B.: In order to use this module you should have a working cloud development
# environment configured with the googledatastore module installed.
#
# Please see https://cloud.google.com/datastore/docs/getstarted/start_python/


_PROJECT_NAME = 'chrome-apps-doc'
_PERSISTENT_OBJECT_KIND = 'PersistentObjectStoreItem'
_VALUE_PROPERTY_NAME = 'pickled_value'

# The max number of entities to include in a single request. This is capped at
# 500 by the service. In practice we may send fewer due to _MAX_REQUEST_SIZE
_MAX_BATCH_SIZE = 500


# The maximum entity size allowed by Datastore.
_MAX_ENTITY_SIZE = 1024*1024


# The maximum request size (in bytes) to send Datastore. This is an approximate
# size based on the sum of entity blob_value sizes.
_MAX_REQUEST_SIZE = 5*1024*1024


def _CreateEntity(client, name, value):
  key = client.key(_PERSISTENT_OBJECT_KIND, name)
  entity = datastore.Entity(
      key=key, exclude_from_indexes=[_VALUE_PROPERTY_NAME])
  entity[_VALUE_PROPERTY_NAME] = value
  return entity


def _CreateBatches(client, data):
  '''Constructs batches of at most _MAX_BATCH_SIZE entities to cover all
  entities defined in |data| without exceeding the transaction size limit.
  This is a generator emitting lists of entities.
  '''
  def get_size(entity):
    return len(entity[_VALUE_PROPERTY_NAME])

  entities = [_CreateEntity(client, name, value)
              for name, value in data.iteritems()]
  batch_start = 0
  batch_end = 1
  batch_size = get_size(entities[0])
  while batch_end < len(entities):
    next_size = get_size(entities[batch_end])
    if (batch_size + next_size > _MAX_REQUEST_SIZE or
        batch_end - batch_start >= _MAX_BATCH_SIZE):
      yield entities[batch_start:batch_end], batch_end, len(entities)
      batch_start = batch_end
      batch_size = 0
    else:
      batch_size += next_size
    batch_end = batch_end + 1
  if batch_end > batch_start and batch_start < len(entities):
    yield entities[batch_start:batch_end], batch_end, len(entities)


def PushData(data, original_data={}):
  '''Pushes a bunch of data into the datastore. The data should be a dict. Each
  key is treated as a namespace, and each value is also a dict. A new datastore
  entry is upserted for every inner key, with the value pickled into the
  |pickled_value| field.

  For example, if given the dictionary:

  {
    'fruit': {
      'apple': 1234,
      'banana': 'yellow',
      'trolling carrot': { 'arbitrarily complex': ['value', 'goes', 'here'] }
    },
    'animal': {
      'sheep': 'baaah',
      'dog': 'woof',
      'trolling cat': 'moo'
    }
  }

  this would result in a push of 6 keys in total, with the following IDs:

    Key('PersistentObjectStoreItem', 'fruit/apple')
    Key('PersistentObjectStoreItem', 'fruit/banana')
    Key('PersistentObjectStoreItem', 'fruit/trolling carrot')
    Key('PersistentObjectStoreItem', 'animal/sheep')
    Key('PersistentObjectStoreItem', 'animal/dog')
    Key('PersistentObjectStoreItem', 'animal/trolling cat')

  If given |original_data|, this will only push key-value pairs for entries that
  are either new or have changed from their original (pickled) value.

  Caveat: Pickling and unpickling a dictionary can (but does not always) change
  its key order. This means that objects will often be seen as changed even when
  they haven't changed.
  '''
  client = datastore.Client(_PROJECT_NAME)

  def flatten(dataset):
    flat = {}
    for namespace, items in dataset.iteritems():
      for k, v in items.iteritems():
        flat['%s/%s' % (namespace, k)] = cPickle.dumps(v)
    return flat

  logging.info('Flattening data sets...')
  data = flatten(data)
  original_data = flatten(original_data)

  logging.info('Culling new data...')
  for k in data.keys():
    if ((k in original_data and original_data[k] == data[k]) or
        (len(data[k]) > _MAX_ENTITY_SIZE)):
      del data[k]

  for entities, n, total in _CreateBatches(client, data):
    batch = client.batch()
    for e in entities:
      batch.put(e)
    logging.info('Committing %s/%s entities...' % (n, total))
    batch.commit()
