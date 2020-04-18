# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# BigQuery constants.

DATASET = 'public'
BUILDS_TABLE = 'builds'
CURRENT_BUILDS_TABLE = 'current_builds'


# `Default` module constants.

DEFAULT_HISTORY_DURATION_SECONDS = 60 * 60 * 12


# `Update` module constants.

BUILDBOT_BASE_URL = 'https://build.chromium.org/p'

CBE_BASE_URL = 'https://chrome-build-extract.appspot.com/p'

MASTER_NAMES = (
    'chromium.perf',
    'chromium.perf.fyi',
    'client.catapult',
    'tryserver.chromium.perf',
    'tryserver.client.catapult',
)


# Code organization / deployment constants.

THIRD_PARTY_LIBRARIES = (
    'apiclient',
    'httplib2',
    'oauth2client',
    'six',
    'uritemplate',
)
