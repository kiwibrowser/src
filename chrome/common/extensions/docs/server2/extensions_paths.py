# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from posixpath import join


# Extensions-related paths within the Chromium repository.

EXTENSIONS = 'extensions/common/'
CHROME_EXTENSIONS = 'chrome/common/extensions/'

BROWSER_EXTENSIONS = 'extensions/browser/'
BROWSER_CHROME_EXTENSIONS = 'chrome/browser/extensions/'

EXTENSIONS_API = join(EXTENSIONS, 'api/')
CHROME_API = join(CHROME_EXTENSIONS, 'api/')

BROWSER_EXTENSIONS_API = join(BROWSER_EXTENSIONS, 'api/')
BROWSER_CHROME_API = join(BROWSER_CHROME_EXTENSIONS, 'api/')

# Note: This determines search order when APIs are resolved in the filesystem.
API_PATHS = (
  CHROME_API,
  EXTENSIONS_API,
)

BROWSER_API_PATHS = (
  BROWSER_CHROME_API,
  BROWSER_EXTENSIONS_API
)

DOCS = join(CHROME_EXTENSIONS, 'docs/')

EXAMPLES = join(DOCS, 'examples/')
SERVER2 = join(DOCS, 'server2/')
STATIC_DOCS = join(DOCS, 'static/')
TEMPLATES = join(DOCS, 'templates/')

APP_YAML = join(SERVER2, 'app.yaml')

ARTICLES_TEMPLATES = join(TEMPLATES, 'articles/')
INTROS_TEMPLATES = join(TEMPLATES, 'intros/')
JSON_TEMPLATES = join(TEMPLATES, 'json/')
PRIVATE_TEMPLATES = join(TEMPLATES, 'private/')
PUBLIC_TEMPLATES = join(TEMPLATES, 'public/')

CONTENT_PROVIDERS = join(JSON_TEMPLATES, 'content_providers.json')

LOCAL_DEBUG_DIR = join(SERVER2, 'local_debug/')
LOCAL_GCS_DIR = join(LOCAL_DEBUG_DIR, 'gcs/')
LOCAL_GCS_DEBUG_CONF = join(LOCAL_DEBUG_DIR, 'gcs_debug.conf')
