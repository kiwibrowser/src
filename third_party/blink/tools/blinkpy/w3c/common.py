# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions used both when importing and exporting."""

import json
import logging


WPT_GH_ORG = 'w3c'
WPT_GH_REPO_NAME = 'web-platform-tests'
WPT_GH_URL = 'https://github.com/%s/%s/' % (WPT_GH_ORG, WPT_GH_REPO_NAME)
WPT_MIRROR_URL = 'https://chromium.googlesource.com/external/w3c/web-platform-tests.git'
WPT_GH_SSH_URL_TEMPLATE = 'https://{}@github.com/%s/%s.git' % (WPT_GH_ORG, WPT_GH_REPO_NAME)
WPT_REVISION_FOOTER = 'WPT-Export-Revision:'
EXPORT_PR_LABEL = 'chromium-export'
PROVISIONAL_PR_LABEL = 'do not merge yet'

# TODO(qyearsley): Avoid hard-coding third_party/WebKit/LayoutTests.
CHROMIUM_WPT_DIR = 'third_party/WebKit/LayoutTests/external/wpt/'

_log = logging.getLogger(__name__)


def read_credentials(host, credentials_json):
    """Extracts GitHub and Gerrit credentials.

    The data is read from the environment and (optionally) a JSON file. When a
    JSON file is specified, any variables from the environment are discarded
    and the values are all expected to be present in the file.

    Args:
        credentials_json: Path to a JSON file containing an object with the
            keys we want to read, or None.
    """
    env_credentials = {}
    for key in ('GH_USER', 'GH_TOKEN', 'GERRIT_USER', 'GERRIT_TOKEN'):
        if key in host.environ:
            env_credentials[key] = host.environ[key]
    if not credentials_json:
        return env_credentials
    if not host.filesystem.exists(credentials_json):
        _log.warning('Credentials JSON file not found at %s.', credentials_json)
        return {}
    credentials = {}
    contents = json.loads(host.filesystem.read_text_file(credentials_json))
    for key in ('GH_USER', 'GH_TOKEN', 'GERRIT_USER', 'GERRIT_TOKEN'):
        if key in contents:
            credentials[key] = contents[key]
    return credentials


def is_testharness_baseline(filename):
    """Checks whether a given file name appears to be a testharness baseline.

    Args:
        filename: A path (absolute or relative) or a basename.
    """
    return filename.endswith('-expected.txt')


def is_basename_skipped(basename):
    """Checks whether to skip (not sync) a file based on its basename.

    Note: this function is used during both import and export, i.e., files with
    skipped basenames are never imported or exported.
    """
    assert '/' not in basename
    blacklist = [
        'MANIFEST.json',    # MANIFEST.json is automatically regenerated.
        'OWNERS',           # https://crbug.com/584660 https://crbug.com/702283
        'reftest.list',     # https://crbug.com/582838
    ]
    return (basename in blacklist
            or is_testharness_baseline(basename)
            or basename.startswith('.'))


def is_file_exportable(path):
    """Checks whether a file in Chromium WPT should be exported to upstream.

    Args:
        path: A relative path from the root of Chromium repository.
    """
    assert path.startswith(CHROMIUM_WPT_DIR)
    basename = path[path.rfind('/') + 1:]
    return not is_basename_skipped(basename)
