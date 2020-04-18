# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from future import Future
from object_store import ObjectStore

class TestObjectStore(ObjectStore):
  '''An object store which records its namespace and behaves like a dict.
  Specify |init| with an initial object for the object store.
  Use CheckAndReset to assert how many times Get/Set/Del have been called. Get
  is a special case; it is only incremented once the future has had Get called.
  '''
  def __init__(self, namespace, start_empty=False, init=None):
    self.namespace = namespace
    self.start_empty = start_empty
    self._store = {} if init is None else init
    if start_empty:
      assert not self._store
    self._get_count = 0
    self._set_count = 0
    self._del_count = 0

  #
  # ObjectStore implementation.
  #

  def GetMulti(self, keys):
    def callback():
      self._get_count += 1
      return dict((k, self._store.get(k)) for k in keys if k in self._store)
    return Future(callback=callback)

  def SetMulti(self, mapping):
    self._set_count += 1
    self._store.update(mapping)
    return Future(value=None)

  def DelMulti(self, keys):
    self._del_count += 1
    for k in keys:
      self._store.pop(k, None)

  #
  # Testing methods.
  #

  def CheckAndReset(self, get_count=0, set_count=0, del_count=0):
    '''Returns a tuple (success, error). Use in tests like:
    self.assertTrue(*object_store.CheckAndReset(...))
    '''
    errors = []
    for desc, expected, actual in (('get_count', get_count, self._get_count),
                                   ('set_count', set_count, self._set_count),
                                   ('del_count', del_count, self._del_count)):
      if actual != expected:
        errors.append('%s: expected %s got %s' % (desc, expected, actual))
    try:
      return (len(errors) == 0, ', '.join(errors))
    finally:
      self.Reset()

  def Reset(self):
    self._get_count = 0
    self._set_count = 0
    self._del_count = 0
