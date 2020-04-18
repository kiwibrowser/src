# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import logging
import posixpath
import time

from appengine_wrappers import urlfetch
from environment import GetAppVersion
from future import Future


_MAX_RETRIES = 5
_RETRY_DELAY_SECONDS = 30


def _MakeHeaders(username, password, access_token):
  headers = {
    'User-Agent': 'Chromium docserver %s' % GetAppVersion(),
    'Cache-Control': 'max-age=0',
  }
  if username is not None and password is not None:
    headers['Authorization'] = 'Basic %s' % base64.b64encode(
        '%s:%s' % (username, password))
  if access_token is not None:
    headers['Authorization'] = 'OAuth %s' % access_token
  return headers


class AppEngineUrlFetcher(object):
  """A wrapper around the App Engine urlfetch module that allows for easy
  async fetches.
  """
  def __init__(self, base_path=None):
    assert base_path is None or not base_path.endswith('/'), base_path
    self._base_path = base_path
    self._retries_left = _MAX_RETRIES

  def Fetch(self, url, username=None, password=None, access_token=None):
    """Fetches a file synchronously.
    """
    return urlfetch.fetch(self._FromBasePath(url),
                          deadline=20,
                          headers=_MakeHeaders(username,
                                               password,
                                               access_token))

  def FetchAsync(self, url, username=None, password=None, access_token=None):
    """Fetches a file asynchronously, and returns a Future with the result.
    """
    def process_result(result):
      if result.status_code == 429:
        if self._retries_left == 0:
          logging.error('Still throttled. Giving up.')
          return result
        self._retries_left -= 1
        logging.info('Throttled. Trying again in %s seconds.' %
                     _RETRY_DELAY_SECONDS)
        time.sleep(_RETRY_DELAY_SECONDS)
        return self.FetchAsync(url, username, password, access_token).Get()
      return result

    rpc = urlfetch.create_rpc(deadline=20)
    urlfetch.make_fetch_call(rpc,
                             self._FromBasePath(url),
                             headers=_MakeHeaders(username,
                                                  password,
                                                  access_token))
    return Future(callback=lambda: process_result(rpc.get_result()))

  def _FromBasePath(self, url):
    assert not url.startswith('/'), url
    if self._base_path is not None:
      url = posixpath.join(self._base_path, url) if url else self._base_path
    return url
