# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from appengine_wrappers import db
from appengine_wrappers import BlobReferenceProperty

BLOB_REFERENCE_BLOBSTORE = 'BlobReferenceBlobstore'

class _Model(db.Model):
  key_ = db.StringProperty()
  value = BlobReferenceProperty()

class BlobReferenceStore(object):
  """A wrapper around the datastore API that can store blob keys.
  """
  def _Query(self, namespace, key):
    return _Model.gql('WHERE key_ = :1', self._MakeKey(namespace, key)).get()

  def _MakeKey(self, namespace, key):
    return '.'.join((namespace, key))

  def Set(self, namespace, key, value):
    _Model(key_=self._MakeKey(namespace, key), value=value).put()

  def Get(self, namespace, key):
    result = self._Query(namespace, key)
    if not result:
      return None
    return result.value

  def Delete(self, namespace, key):
    result = self._Query(namespace, key)
    if not result:
      return None
    blob_key = result.value
    result.delete()
    return blob_key
