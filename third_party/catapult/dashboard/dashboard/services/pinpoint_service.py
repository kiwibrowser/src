# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions for getting commit information from Pinpoint."""

import json
import urllib

from dashboard.common import datastore_hooks
from dashboard.common import utils

_PINPOINT_URL = 'https://pinpoint-dot-chromeperf.appspot.com'


def NewJob(params):
  """Submits a new job request to Pinpoint."""
  return _Request('/api/new', params)


def _Request(endpoint, params):
  """Sends a request to an endpoint and returns JSON data."""
  assert datastore_hooks.IsUnalteredQueryPermitted()

  http_auth = utils.ServiceAccountHttp(timeout=30)
  _, content = http_auth.request(
      _PINPOINT_URL + endpoint,
      method='POST',
      body=urllib.urlencode(params),
      headers={'Content-length': 0})

  return json.loads(content)
