#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fake downloader which can be used in place of http_download."""

import os
import shutil
import urllib2

class FakeDownloader(object):
  """Testing replacement for http_download that copies files."""

  def __init__(self, copy_func=None):
    self._urls = {}
    if copy_func is None:
      self._copy_func = shutil.copyfile
    else:
      self._copy_func = copy_func
    self._download_count = 0

  def StoreURL(self, url, filepath):
    """Stores a known url into the downloader to download."""
    self._urls[url] = filepath

  def Download(self, url, target, username=None, verbose=True, password=None,
               logger=None):
    if url not in self._urls:
      raise urllib2.HTTPError(url, 404,
                              'Fake Downloader retrieved invalid URL',
                              [], None)
    self._copy_func(self._urls[url], target)
    self._download_count += 1

  def GetDownloadCount(self):
    return self._download_count
