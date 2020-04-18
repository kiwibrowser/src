# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cPickle
import google.appengine.ext.db as db
import logging
import traceback


_MAX_ENTITY_SIZE = 1024*1024


# A collection of the data store models used throughout the server.
# These values are global within datastore.

class PersistentObjectStoreItem(db.Model):
  pickled_value = db.BlobProperty()

  @classmethod
  def CreateKey(cls, namespace, key):
    path = '%s/%s' % (namespace, key)
    try:
      return db.Key.from_path(cls.__name__, path)
    except Exception:
      # Probably AppEngine's BadValueError for the name being too long, but
      # it's not documented which errors can actually be thrown here, so catch
      # 'em all.
      raise ValueError(
          'Exception thrown when trying to create db.Key from path %s: %s' % (
              path, traceback.format_exc()))

  @classmethod
  def CreateItem(cls, namespace, key, value):
    pickled_value = cPickle.dumps(value)
    if len(pickled_value) > _MAX_ENTITY_SIZE:
      logging.warn('Refusing to create entity greater than 1 MB in size: %s/%s'
                   % (namespace, key))
      return None
    return PersistentObjectStoreItem(key=cls.CreateKey(namespace, key),
                                     pickled_value=pickled_value)

  def GetValue(self):
    return cPickle.loads(self.pickled_value)
