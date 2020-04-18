# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time

from environment import IsAppEngine
from future import Future
from url_fetcher import UrlFetcher


FAKE_URL_FETCHER_CONFIGURATION = None


def ConfigureFakeUrlFetch(configuration):
  """|configuration| is a dictionary mapping strings to fake urlfetch classes.
  A fake urlfetch class just needs to have a fetch method. The keys of the
  dictionary are treated as regex, and they are matched with the URL to
  determine which fake urlfetch is used.
  """
  global FAKE_URL_FETCHER_CONFIGURATION
  FAKE_URL_FETCHER_CONFIGURATION = dict(
      (re.compile(k), v) for k, v in configuration.iteritems())


def _GetConfiguration(key):
  if not FAKE_URL_FETCHER_CONFIGURATION:
    raise ValueError('No fake fetch paths have been configured. '
                     'See ConfigureFakeUrlFetch in url_fetcher_fake.py.')
  for k, v in FAKE_URL_FETCHER_CONFIGURATION.iteritems():
    if k.match(key):
      return v
  raise ValueError('No configuration found for %s' % key)


class UrlFetcherFake(UrlFetcher):
  """A fake UrlFetcher implementation which may be configured with manual URL
  overrides for testing. By default this 404s on everything.
  """
  class DownloadError(Exception):
    pass

  class _Response(object):
    def __init__(self, content):
      self.content = content
      self.headers = {'Content-Type': 'none'}
      self.status_code = 200

  def FetchImpl(self, url, headers):
    if IsAppEngine():
      raise ValueError('Attempted to fetch URL from AppEngine: %s' % url)

    url = url.split('?', 1)[0]
    response = self._Response(_GetConfiguration(url).fetch(url))
    if response.content is None:
      response.status_code = 404
    return response

  def FetchAsyncImpl(self, url, headers):
    return Future(callback=lambda: self.FetchImpl(url, headers))
