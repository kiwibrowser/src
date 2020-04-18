# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Ensures that all depot_tools talks directly to appengine to avoid SNI."""

import urlparse


mapping = {
  'codereview.chromium.org': 'chromiumcodereview.appspot.com',
  'crashpad.chromium.org': 'crashpad-home.appspot.com',
  'bugs.chromium.org': 'monorail-prod.appspot.com',
  'bugs-staging.chromium.org': 'monorail-staging.appspot.com',
}


def MapUrl(url):
  parts = list(urlparse.urlsplit(url))
  new_netloc = mapping.get(parts[1])
  if new_netloc:
    parts[1] = new_netloc
  return urlparse.urlunsplit(parts)
