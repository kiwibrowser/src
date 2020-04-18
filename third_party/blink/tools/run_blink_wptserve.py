#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an instance of wptserve to allow manual testing of web-platform-tests.

The main HTTP server is run on 8001, while the main HTTPS server is run on 8444.

URL paths are relative to the web-platform-tests root, e.g. the test:
    LayoutTests/external/wpt/referrer-policy/origin/http-rp/same-origin/http-http/img-tag/generic.no-redirect.http.html
Could be tried by running this scrip then navigating to:
    http://localhost:8001/referrer-policy/origin/http-rp/same-origin/http-http/img-tag/generic.no-redirect.http.html
"""

from blinkpy.common import version_check  # pylint: disable=unused-import
from blinkpy.web_tests.servers import cli_wrapper
from blinkpy.web_tests.servers import wptserve

cli_wrapper.main(wptserve.WPTServe, description=__doc__)
