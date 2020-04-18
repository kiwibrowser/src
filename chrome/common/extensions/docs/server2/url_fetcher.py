# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath

from environment import GetAppVersion
from future import Future
from urllib import urlencode
from urlparse import urlparse, parse_qs

def _MakeHeaders(headers={}):
  headers['User-Agent'] = 'Chromium-Docserver/%s' % GetAppVersion()
  headers['Cache-Control'] = 'max-age=0'
  return headers


def _AddQueryToUrl(url, new_query):
  """Adds query paramters to a URL. This merges the given set of query params
  with any that were already a part of the URL.
  """
  parsed_url = urlparse(url)
  query = parse_qs(parsed_url.query)
  query.update(new_query)
  query_string = urlencode(query, True)
  return '%s://%s%s?%s#%s' % (parsed_url.scheme, parsed_url.netloc,
      parsed_url.path, query_string, parsed_url.fragment)


class UrlFetcher(object):
  def __init__(self):
    self._base_path = None

  def SetBasePath(self, base_path):
    assert base_path is None or not base_path.endswith('/'), base_path
    self._base_path = base_path

  def Fetch(self, url, headers={}, query={}):
    return self.FetchImpl(_AddQueryToUrl(self._FromBasePath(url), query),
        _MakeHeaders(headers))

  def FetchAsync(self, url, headers={}, query={}):
    return self.FetchAsyncImpl(_AddQueryToUrl(self._FromBasePath(url), query),
        _MakeHeaders(headers))

  def FetchImpl(self, url, headers):
    """Fetches a URL synchronously.
    """
    raise NotImplementedError(self.__class__)

  def FetchAsyncImpl(self, url, headers):
    """Fetches a URL asynchronously and returns a Future with the result.
    """
    raise NotImplementedError(self.__class__)

  def _FromBasePath(self, url):
    assert not url.startswith('/'), url
    if self._base_path is not None:
      url = posixpath.join(self._base_path, url) if url else self._base_path
    return url
