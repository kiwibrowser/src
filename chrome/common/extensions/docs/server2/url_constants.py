# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GITHUB_REPOS = 'https://api.github.com/repos'
GITHUB_BASE = 'https://github.com/GoogleChrome/chrome-app-samples/tree/master/samples'
RAW_GITHUB_BASE = ('https://github.com/GoogleChrome/chrome-app-samples/raw/'
                   'master')
OMAHA_PROXY_URL = 'https://omahaproxy.appspot.com/json'
OMAHA_HISTORY = 'https://omahaproxy.appspot.com/history.json'
OMAHA_DEV_HISTORY = '%s?channel=dev&os=win&json=1' % OMAHA_HISTORY
SVN_URL = 'http://src.chromium.org/chrome'
VIEWVC_URL = 'http://src.chromium.org/viewvc/chrome'
EXTENSIONS_SAMPLES = ('https://chromium.googlesource.com/chromium/src/+/master/chrome/'
                      'common/extensions/docs/examples')
CODEREVIEW_SERVER = 'https://codereview.chromium.org'
