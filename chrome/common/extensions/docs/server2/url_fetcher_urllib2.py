# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from future import Future
from url_fetcher import UrlFetcher
from urllib2 import urlopen, Request, HTTPError


class UrlFetcherUrllib2(UrlFetcher):
  """UrlFetcher implementation for use outside of AppEngine. Does NOT support
  async fetching, so FetchAsync calls are still blocking.
  """
  class _Response(object):
    def __init__(self, code, content=None, headers={}):
      self.status_code = code
      self.content = content
      self.headers = headers

  def FetchImpl(self, url, headers):
    request = Request(url, headers=headers)
    try:
      urlresponse = urlopen(request)
      response = self._Response(urlresponse.code, urlresponse.read(),
                                urlresponse.headers.dict)
      urlresponse.close()
      return response
    except HTTPError as e:
      return self._Response(e.code, e.reason, e.headers.dict)

  def FetchAsyncImpl(self, url, headers):
    return Future(callback=lambda: self.FetchImpl(url, headers))
