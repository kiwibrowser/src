# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import google.appengine.api.urlfetch as urlfetch
import logging
import time
import traceback

from future import Future
from url_fetcher import UrlFetcher


_MAX_RETRIES = 5
_RETRY_DELAY_SECONDS = 30


class UrlFetcherAppengine(UrlFetcher):
  """An implementation of UrlFetcher which uses the GAE urlfetch API to
  execute URL fetches.
  """
  def __init__(self):
    self._retries_left = _MAX_RETRIES

  def FetchImpl(self, url, headers):
    return urlfetch.fetch(url, deadline=20, headers=headers)

  def FetchAsyncImpl(self, url, headers):
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
    urlfetch.make_fetch_call(rpc, url, headers=headers)
    return Future(callback=lambda: process_result(rpc.get_result()))
