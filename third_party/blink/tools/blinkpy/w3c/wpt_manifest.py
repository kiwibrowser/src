# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""WPTManifest is responsible for handling MANIFEST.json.

The MANIFEST.json file contains metadata about files in web-platform-tests,
such as what tests exist, and extra information about each test, including
test type, options, URLs to use, and reference file paths if applicable.
"""

import json
import logging

from blinkpy.common.memoized import memoized
from blinkpy.common.path_finder import PathFinder

_log = logging.getLogger(__file__)


# TODO(robertma): Use the official wpt.manifest module.

class WPTManifest(object):
    """A simple abstraction of WPT MANIFEST.json.

    The high-level structure of the manifest is as follows:
        {
            "items": {
                "manual": {
                    "file/path": [manifest items],
                    ...
                },
                "reftest": {...},
                "testharness": {...}
            },
            // other info...
        }

    The format of a manifest item depends on:
        https://github.com/w3c/web-platform-tests/blob/master/tools/manifest/item.py
    which can be roughly summarized as follows:
        * testharness test: [url, extras]
        * reftest: [url, references, extras]
    where `extras` is a dict with the following optional items:
        * testharness test: {"timeout": "long", "testdriver": True}
        * reftest: {"timeout": "long", "viewport_size": ..., "dpi": ...}
    and `references` is a list that looks like:
        [[reference_url1, "=="], [reference_url2, "!="], ...]
    """

    def __init__(self, json_content):
        self.raw_dict = json.loads(json_content)
        self.test_types = ('manual', 'reftest', 'testharness')

    def _items_for_file_path(self, path_in_wpt):
        """Finds manifest items for the given WPT path.

        Args:
            path_in_wpt: A file path relative to the root of WPT. Note that this
                is different from a WPT URL; a file path does not have a leading
                slash or a query string.

        Returns:
            A list of manifest items, or None if not found.
        """
        items = self.raw_dict['items']
        for test_type in self.test_types:
            if path_in_wpt in items[test_type]:
                return items[test_type][path_in_wpt]
        return None

    def _item_for_url(self, url):
        """Finds the manifest item for the given WPT URL.

        Args:
            url: A WPT URL (with the leading slash).

        Returns:
            A manifest item, or None if not found.
        """
        return self.all_url_items().get(url)

    @staticmethod
    def _get_url_from_item(item):
        return item[0]

    @staticmethod
    def _get_extras_from_item(item):
        return item[-1]

    @memoized
    def all_url_items(self):
        """Returns a dict mapping every URL in the manifest to its item."""
        url_items = {}
        if 'items' not in self.raw_dict:
            return url_items
        for test_type in self.test_types:
            for records in self.raw_dict['items'][test_type].itervalues():
                for item in records:
                    url_items[self._get_url_from_item(item)] = item
        return url_items

    @memoized
    def all_urls(self):
        """Returns a set of the URLs for all items in the manifest."""
        return frozenset(self.all_url_items().keys())

    def is_test_file(self, path_in_wpt):
        return self._items_for_file_path(path_in_wpt) is not None

    def is_test_url(self, url):
        """Checks if url is a valid test in the manifest.

        The url must be the WPT test name with a leading slash (/).
        """
        if url[0] != '/':
            raise Exception('Test url missing leading /: %s' % url)
        return url in self.all_urls()

    def file_path_to_url_paths(self, path_in_wpt):
        manifest_items = self._items_for_file_path(path_in_wpt)
        assert manifest_items is not None
        # Remove the leading slashes when returning.
        return [self._get_url_from_item(item)[1:] for item in manifest_items]

    def is_slow_test(self, url):
        """Checks if a WPT is slow (long timeout) according to the manifest.

        Args:
            url: A WPT URL (with the leading slash).

        Returns:
            True if the test is found and is slow, False otherwise.
        """
        if not self.is_test_url(url):
            return False

        item = self._item_for_url(url)
        if not item:
            return False
        extras = self._get_extras_from_item(item)
        return extras.get('timeout') == 'long'

    def extract_reference_list(self, path_in_wpt):
        """Extracts reference information of the specified reference test.

        The return value is a list of (match/not-match, reference path in wpt)
        pairs, like:
           [("==", "foo/bar/baz-match.html"),
            ("!=", "foo/bar/baz-mismatch.html")]
        """
        all_items = self.raw_dict['items']
        if path_in_wpt not in all_items['reftest']:
            return []
        reftest_list = []
        for item in all_items['reftest'][path_in_wpt]:
            for ref_path_in_wpt, expectation in item[1]:
                reftest_list.append((expectation, ref_path_in_wpt))
        return reftest_list

    @staticmethod
    def ensure_manifest(host):
        """Generates the MANIFEST.json file if it does not exist."""
        finder = PathFinder(host.filesystem)
        manifest_path = finder.path_from_layout_tests('external', 'wpt', 'MANIFEST.json')
        base_manifest_path = finder.path_from_layout_tests('external', 'WPT_BASE_MANIFEST.json')

        if not host.filesystem.exists(base_manifest_path):
            _log.error('Manifest base not found at "%s".', base_manifest_path)
            host.filesystem.write_text_file(base_manifest_path, '{}')

        if not host.filesystem.exists(manifest_path):
            _log.debug('Manifest not found, copying from base "%s".', base_manifest_path)
            host.filesystem.copyfile(base_manifest_path, manifest_path)

        wpt_path = manifest_path = finder.path_from_layout_tests('external', 'wpt')
        WPTManifest.generate_manifest(host, wpt_path)

        _log.debug('Manifest generation completed.')

    @staticmethod
    def generate_manifest(host, dest_path):
        """Generates MANIFEST.json on the specified directory."""
        finder = PathFinder(host.filesystem)
        wpt_exec_path = finder.path_from_blink_tools('blinkpy', 'third_party', 'wpt', 'wpt', 'wpt')
        cmd = ['python', wpt_exec_path, 'manifest', '--work', '--tests-root', dest_path]

        # ScriptError will be raised if the command fails.
        host.executive.run_command(
            cmd,
            return_stderr=True  # This will also include stderr in the exception message.
        )
