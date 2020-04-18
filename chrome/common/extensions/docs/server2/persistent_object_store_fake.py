# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cPickle
import logging

from future import Future
from object_store import ObjectStore


class PersistentObjectStoreFake(ObjectStore):
  '''Stores or retrieves data in memory. Not really persistent.
  '''
  # Static storage shared across all fake object stores.
  DATA = {}

  def __init__(self, namespace):
    if namespace not in PersistentObjectStoreFake.DATA:
      PersistentObjectStoreFake.DATA[namespace] = {}
    self._data = PersistentObjectStoreFake.DATA[namespace]

  def SetMulti(self, mapping):
    self._data.update(mapping)
    return Future(value=True)

  def GetMulti(self, keys):
    result = dict((key, self._data[key]) for key in keys if key in self._data)
    return Future(value=result)

  def DelMulti(self, keys):
    for key in keys:
      del self._data[key]

  @classmethod
  def LoadFromFile(cls, filename):
    with open(filename, 'r') as f:
      cls.DATA = cPickle.load(f)
    for k, v in cls.DATA.iteritems():
      cls.DATA[k] = cPickle.loads(v)
    logging.info('Loaded %s keys from %s.' % (len(cls.DATA), filename))

  @classmethod
  def SaveToFile(cls, filename):
    data = dict((k, cPickle.dumps(v)) for k, v, in cls.DATA.iteritems())
    with open(filename, 'w') as f:
      cPickle.dump(data, f)
    logging.info('Saved %s keys to %s.' % (len(cls.DATA), filename))
