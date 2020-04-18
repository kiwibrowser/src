# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging

from google.appengine.api import urlfetch
from google.appengine.runtime import apiproxy_errors

from base import constants


def BuildUrl(master_name, url, use_cbe=False):
  base = constants.BUILDBOT_BASE_URL
  if use_cbe:
    base = constants.CBE_BASE_URL
    url += '?json=1'

  return '%s/%s/%s' % (base, master_name, url)


def FetchData(url):
  try:
    return json.loads(FetchText(url))
  except ValueError:
    logging.warning('Data is corrupt: %s', url)
    raise


def FetchText(url):
  logging.debug('Retrieving %s', url)
  try:
    return urlfetch.fetch(url).content
  except (apiproxy_errors.DeadlineExceededError, urlfetch.DownloadError,
          urlfetch.InternalTransientError):
    # Could be intermittent; try again.
    try:
      return urlfetch.fetch(url).content
    except:
      logging.error('Error retrieving URL: %s', url)
      raise
  except:
    logging.error('Error retrieving URL: %s', url)
    raise
