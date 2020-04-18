# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import posixpath

from future import Future
from path_util import AssertIsDirectory, IsDirectory


class _Response(object):
  def __init__(self, content=''):
    self.content = content
    self.headers = {'Content-Type': 'none'}
    self.status_code = 200


class FakeUrlFetcher(object):
  def __init__(self, base_path):
    self._base_path = base_path
    # Mock capabilities. Perhaps this class should be MockUrlFetcher.
    self._sync_count = 0
    self._async_count = 0
    self._async_resolve_count = 0

  def _ReadFile(self, filename):
    # Fake DownloadError, the error that appengine usually raises.
    class DownloadError(Exception): pass
    try:
      with open(os.path.join(self._base_path, filename), 'r') as f:
        return f.read()
    except IOError as e:
      raise DownloadError(e)

  def _ListDir(self, directory):
    # In some tests, we need to test listing a directory from the HTML returned
    # from SVN. This reads an HTML file that has the directories HTML.
    if not os.path.isdir(os.path.join(self._base_path, directory)):
      return self._ReadFile(directory[:-1])
    files = os.listdir(os.path.join(self._base_path, directory))
    html = '<html><title>Revision: 00000</title>\n'
    for filename in files:
      if filename.startswith('.'):
        continue
      if os.path.isdir(os.path.join(self._base_path, directory, filename)):
        html += '<a>' + filename + '/</a>\n'
      else:
        html += '<a>' + filename + '</a>\n'
    html += '</html>'
    return html

  def FetchAsync(self, url):
    self._async_count += 1
    url = url.rsplit('?', 1)[0]
    def resolve():
      self._async_resolve_count += 1
      return self._DoFetch(url)
    return Future(callback=resolve)

  def Fetch(self, url):
    self._sync_count += 1
    return self._DoFetch(url)

  def _DoFetch(self, url):
    url = url.rsplit('?', 1)[0]
    result = _Response()
    if IsDirectory(url):
      result.content = self._ListDir(url)
    else:
      result.content = self._ReadFile(url)
    return result

  def CheckAndReset(self, sync_count=0, async_count=0, async_resolve_count=0):
    '''Returns a tuple (success, error). Use in tests like:
    self.assertTrue(*fetcher.CheckAndReset(...))
    '''
    errors = []
    for desc, expected, actual in (
        ('sync_count', sync_count, self._sync_count),
        ('async_count', async_count, self._async_count),
        ('async_resolve_count', async_resolve_count,
                                self._async_resolve_count)):
      if actual != expected:
        errors.append('%s: expected %s got %s' % (desc, expected, actual))
    try:
      return (len(errors) == 0, ', '.join(errors))
    finally:
      self.Reset()

  def Reset(self):
    self._sync_count = 0
    self._async_count = 0
    self._async_resolve_count = 0


class FakeURLFSFetcher(object):
  '''Use a file_system to resolve fake fetches. Mimics the interface of Google
  Appengine's urlfetch.
  '''

  def __init__(self, file_system, base_path):
    AssertIsDirectory(base_path)
    self._base_path = base_path
    self._file_system = file_system

  def FetchAsync(self, url, **kwargs):
    return Future(value=self.Fetch(url))

  def Fetch(self, url, **kwargs):
    return _Response(self._file_system.ReadSingle(
        posixpath.join(self._base_path, url)).Get())

  def UpdateFS(self, file_system, base_path=None):
    '''Replace the underlying FileSystem used to reslove URLs.
    '''
    self._file_system = file_system
    self._base_path = base_path or self._base_path


class MockURLFetcher(object):
  def __init__(self, fetcher):
    self._fetcher = fetcher
    self.Reset()

  def Fetch(self, url, **kwargs):
    self._fetch_count += 1
    return self._fetcher.Fetch(url, **kwargs)

  def FetchAsync(self, url, **kwargs):
    self._fetch_async_count += 1
    def next(result):
      self._fetch_resolve_count += 1
      return result
    return self._fetcher.FetchAsync(url, **kwargs).Then(next)

  def CheckAndReset(self,
                    fetch_count=0,
                    fetch_async_count=0,
                    fetch_resolve_count=0):
    errors = []
    for desc, expected, actual in (
        ('fetch_count', fetch_count, self._fetch_count),
        ('fetch_async_count', fetch_async_count, self._fetch_async_count),
        ('fetch_resolve_count', fetch_resolve_count,
                                self._fetch_resolve_count)):
      if actual != expected:
        errors.append('%s: expected %s got %s' % (desc, expected, actual))
    try:
      return (len(errors) == 0, ', '.join(errors))
    finally:
      self.Reset()

  def Reset(self):
    self._fetch_count = 0
    self._fetch_async_count = 0
    self._fetch_resolve_count = 0
