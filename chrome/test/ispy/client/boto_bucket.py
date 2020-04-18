# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implementation of CloudBucket using Google Cloud Storage as the backend."""
import os
import sys

# boto requires depot_tools/third_party be in the path. Use
# src/build/find_depot_tools.py to add this directory.
sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                             os.pardir, os.pardir, 'build'))
import find_depot_tools
DEPOT_TOOLS_PATH = find_depot_tools.add_depot_tools_to_path()
sys.path.append(os.path.join(os.path.abspath(DEPOT_TOOLS_PATH), 'third_party'))
import boto

from ispy.common import cloud_bucket


class BotoCloudBucket(cloud_bucket.BaseCloudBucket):
  """Interfaces with GS using the boto library."""

  def __init__(self, key, secret, bucket_name):
    """Initializes the bucket with a key, secret, and bucket_name.

    Args:
      key: the API key to access GS.
      secret: the API secret to access GS.
      bucket_name: the name of the bucket to connect to.
    """
    uri = boto.storage_uri('', 'gs')
    conn = uri.connect(key, secret)
    self.bucket = conn.get_bucket(bucket_name)

  def _GetKey(self, path):
    key = boto.gs.key.Key(self.bucket)
    key.key = path
    return key

  # override
  def UploadFile(self, path, contents, content_type):
    key = self._GetKey(path)
    key.set_metadata('Content-Type', content_type)
    key.set_contents_from_string(contents)
    # Open permissions for the appengine account to read/write.
    key.add_email_grant('FULL_CONTROL',
        'ispy.google.com@appspot.gserviceaccount.com')

  # override
  def DownloadFile(self, path):
    key = self._GetKey(path)
    if key.exists():
      return key.get_contents_as_string()
    else:
      raise cloud_bucket.FileNotFoundError

  # override
  def UpdateFile(self, path, contents):
    key = self._GetKey(path)
    if key.exists():
      key.set_contents_from_string(contents)
    else:
      raise cloud_bucket.FileNotFoundError

  # override
  def RemoveFile(self, path):
    key = self._GetKey(path)
    key.delete()

  # override
  def FileExists(self, path):
    key = self._GetKey(path)
    return key.exists()

  # override
  def GetImageURL(self, path):
    key = self._GetKey(path)
    if key.exists():
      # Corrects a bug in boto that incorrectly generates a url
      #  to a resource in Google Cloud Storage.
      return key.generate_url(3600).replace('AWSAccessKeyId', 'GoogleAccessId')
    else:
      raise cloud_bucket.FileNotFoundError(path)

  # override
  def GetAllPaths(self, prefix):
    return (key.key for key in self.bucket.get_all_keys(prefix=prefix))
