# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import webapp2

from handlers import blank
from handlers import builds


_URL_MAPPING = [
    ('/_ah/health', blank.Blank),
    ('/_ah/start', blank.Blank),
    ('/builds', builds.Builds),
]
app = webapp2.WSGIApplication(_URL_MAPPING)  # pylint: disable=invalid-name
