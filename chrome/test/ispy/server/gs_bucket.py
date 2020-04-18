# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implementation of CloudBucket using Google Cloud Storage as the backend."""
import os
import sys

import cloudstorage

from common import cloud_bucket


class GoogleCloudStorageBucket(cloud_bucket.BaseCloudBucket):
  """Subclass of cloud_bucket.CloudBucket with actual GS commands."""

  def __init__(self, bucket):
    """Initializes the bucket.

    Args:
      bucket: the name of the bucket to connect to.
    """
    self.bucket = '/' + bucket

  def _full_path(self, path):
    return self.bucket + '/' + path.lstrip('/')

  # override
  def UploadFile(self, path, contents, content_type):
    gs_file = cloudstorage.open(
        self._full_path(path), 'w', content_type=content_type)
    gs_file.write(contents)
    gs_file.close()

  # override
  def DownloadFile(self, path):
    try:
      gs_file = cloudstorage.open(self._full_path(path), 'r')
      r = gs_file.read()
      gs_file.close()
    except Exception as e:
      raise Exception('%s: %s' % (self._full_path(path), str(e)))
    return r

  # override
  def UpdateFile(self, path, contents):
    if not self.FileExists(path):
      raise cloud_bucket.FileNotFoundError
    gs_file = cloudstorage.open(self._full_path(path), 'w')
    gs_file.write(contents)
    gs_file.close()

  # override
  def RemoveFile(self, path):
    cloudstorage.delete(self._full_path(path))

  # override
  def FileExists(self, path):
    try:
      cloudstorage.stat(self._full_path(path))
    except cloudstorage.NotFoundError:
      return False
    return True

  # override
  def GetImageURL(self, path):
    return '/image?file_path=%s' % path

  # override
  def GetAllPaths(self, prefix, max_keys=None, marker=None, delimiter=None):
    return (f.filename[len(self.bucket) + 1:] for f in
            cloudstorage.listbucket(self.bucket, prefix=prefix,
                max_keys=max_keys, marker=marker, delimiter=delimiter))
