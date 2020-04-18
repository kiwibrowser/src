# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import environment
import logging

from caching_file_system import CachingFileSystem
from empty_dir_file_system import EmptyDirFileSystem
from environment import IsTest
from extensions_paths import LOCAL_GCS_DIR, LOCAL_GCS_DEBUG_CONF
from gcs_file_system import CloudStorageFileSystem
from local_file_system import LocalFileSystem
from path_util import ToDirectory


class CloudStorageFileSystemProvider(object):
  '''Provides CloudStorageFileSystem bound to a GCS bucket.
  '''
  def __init__(self, object_store_creator):
    self._object_store_creator = object_store_creator

  def Create(self, bucket):
    '''Creates a CloudStorageFileSystemProvider.

    |bucket| is the name of GCS bucket, eg devtools-docs. It is expected
             that this bucket has Read permission for this app in its ACLs.

    Optional configuration can be set in a local_debug/gcs_debug.conf file:
      use_local_fs=True|False
      remote_bucket_prefix=<prefix>

    If running in Preview mode or in Development mode with use_local_fs set to
    True, buckets and files are looked for inside the local_debug folder instead
    of in the real GCS server.
    '''
    if IsTest():
      return EmptyDirFileSystem()

    debug_bucket_prefix = None
    use_local_fs = False
    if os.path.exists(LOCAL_GCS_DEBUG_CONF):
      with open(LOCAL_GCS_DEBUG_CONF, "r") as token_file:
        properties = dict(line.strip().split('=', 1) for line in token_file)
      use_local_fs = properties.get('use_local_fs', 'False')=='True'
      debug_bucket_prefix = properties.get('remote_bucket_prefix', None)
      logging.debug('gcs: prefixing all bucket names with %s' %
                    debug_bucket_prefix)

    if use_local_fs:
      return LocalFileSystem(ToDirectory(os.path.join(LOCAL_GCS_DIR, bucket)))

    if debug_bucket_prefix:
      bucket = debug_bucket_prefix + bucket

    return CachingFileSystem(CloudStorageFileSystem(bucket),
                             self._object_store_creator)

  @staticmethod
  def ForEmpty():
    class EmptyImpl(object):
      def Create(self, bucket):
        return EmptyDirFileSystem()
    return EmptyImpl()
