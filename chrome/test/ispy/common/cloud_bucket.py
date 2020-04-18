# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Abstract injector class for GS requests."""


class FileNotFoundError(Exception):
  """Thrown by a subclass of CloudBucket when a file is not found."""
  pass


class BaseCloudBucket(object):
  """An abstract base class for working with GS."""

  def UploadFile(self, path, contents, content_type):
    """Uploads a file to GS.

    Args:
      path: where in GS to upload the file.
      contents: the contents of the file to be uploaded.
      content_type: the MIME Content-Type of the file.
    """
    raise NotImplementedError

  def DownloadFile(self, path):
    """Downsloads a file from GS.

    Args:
      path: the location in GS to download the file from.

    Returns:
      String contents of the file downloaded.

    Raises:
      bucket_injector.NotFoundException: if the file is not found.
    """
    raise NotImplementedError

  def UpdateFile(self, path, contents):
    """Uploads a file to GS.

    Args:
      path: location of the file in GS to update.
      contents: the contents of the file to be updated.
    """
    raise NotImplementedError

  def RemoveFile(self, path):
    """Removes a file from GS.

    Args:
      path: the location in GS to download the file from.
    """
    raise NotImplementedError

  def FileExists(self, path):
    """Checks if a file exists in GS.

    Args:
      path: the location in GS of the file.

    Returns:
      boolean representing whether the file exists in GS.
    """
    raise NotImplementedError

  def GetImageURL(self, path):
    """Gets a URL to an item in GS from its path.

    Args:
      path: the location in GS of a file.

    Returns:
      an url to a file in GS.

    Raises:
      bucket_injector.NotFoundException: if the file is not found.
    """
    raise NotImplementedError

  def GetAllPaths(self, prefix):
    """Gets paths to files in GS that start with a prefix.

    Args:
      prefix: the prefix to filter files in GS.

    Returns:
      a generator of paths to files in GS.
    """
    raise NotImplementedError
