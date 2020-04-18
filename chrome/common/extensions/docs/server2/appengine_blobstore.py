# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import blob_reference_store as datastore
from blob_reference_store import BlobReferenceStore
from appengine_wrappers import blobstore
from appengine_wrappers import files

BLOBSTORE_GITHUB = 'BlobstoreGithub'

# TODO(kalman): Re-write this class.
#   - It uses BlobReader which is a synchronous method. We should be creating
#     multiple async fetches, one for each partition, then exposing a Future
#     interface which stitches them together.
#   - It's very hard to reuse.

class AppEngineBlobstore(object):
  """A wrapper around the blobstore API, which stores the blob keys in
  datastore.
  """
  def __init__(self):
    self._datastore = BlobReferenceStore()

  def Set(self, key, blob, namespace):
    """Add a blob to the blobstore. |version| is used as part of the key so
    multiple blobs with the same name can be differentiated.
    """
    key = namespace + '.' + key
    filename = files.blobstore.create()
    with files.open(filename, 'a') as f:
      f.write(blob)
    files.finalize(filename)
    blob_key = files.blobstore.get_blob_key(filename)
    self._datastore.Set(datastore.BLOB_REFERENCE_BLOBSTORE, key, blob_key)

  def Get(self, key, namespace):
    """Get a blob with version |version|.
    """
    key = namespace + '.' + key
    blob_key = self._datastore.Get(datastore.BLOB_REFERENCE_BLOBSTORE, key)
    if blob_key is None:
      return None
    blob_reader = blobstore.BlobReader(blob_key)
    return blob_reader.read()

  def Delete(self, key, namespace):
    """Delete the blob with version |version| if it is found.
    """
    key = namespace + '.' + key
    blob_key = self._datastore.Delete(datastore.BLOB_REFERENCE_BLOBSTORE, key)
    if blob_key is None:
      return
    blob_key.delete()
