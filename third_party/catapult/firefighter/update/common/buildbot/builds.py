# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urllib

from google.appengine.api import urlfetch

from common.buildbot import build
from common.buildbot import network


class Builds(object):

  def __init__(self, master_name, builder_name, url):
    self._master_name = master_name
    self._builder_name = builder_name
    self._url = url

  def __getitem__(self, key):
    """Fetches a Build object containing build details.

    Args:
      key: A nonnegative build number.

    Returns:
      A Build object.

    Raises:
      TypeError: key is not an int.
      ValueError: key is negative.
    """
    # We can't take slices because we don't have a defined length.
    if not isinstance(key, int):
      raise TypeError('build numbers must be integers, not %s' %
                      type(key).__name__)

    return self.Fetch((key,))

  def Fetch(self, build_numbers):
    """Downloads and returns build details.

    If a build has corrupt data, it is not included in the result. If you
    strictly need all the builds requested, be sure to check the result length.

    Args:
      build_numbers: An iterable of build numbers to download.

    Yields:
      Build objects, in the order requested. Some may be missing.

    Raises:
      ValueError: A build number is invalid.
    """
    if not build_numbers:
      return

    for build_number in build_numbers:
      if build_number < 0:
        raise ValueError('Invalid build number: %d' % build_number)

    builds = []
    for build_number in build_numbers:
      url = 'builders/%s/builds/%d' % (
          urllib.quote(self._builder_name), build_number)
      url = network.BuildUrl(self._master_name, url, use_cbe=True)
      try:
        builds.append(network.FetchData(url))
      except (ValueError, urlfetch.ResponseTooLargeError):
        logging.warning('Unable to fetch %s/%s build %d',
                        self._master_name, self._builder_name, build_number)
        continue

    for build_data in builds:
      if 'error' in build_data:
        continue
      yield build.Build(build_data, self._url)
