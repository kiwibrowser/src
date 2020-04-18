#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import unittest
import sys

from fake_host_file_system_provider import FakeHostFileSystemProvider
from server_instance import ServerInstance
from test_data.canned_data import CANNED_API_FILE_SYSTEM_DATA
from whats_new_data_source import WhatsNewDataSource


class WhatsNewDataSourceTest(unittest.TestCase):
  def testCreateWhatsNewDataSource(self):
    api_fs_creator = FakeHostFileSystemProvider(CANNED_API_FILE_SYSTEM_DATA)
    server_instance = ServerInstance.ForTest(
        file_system_provider=api_fs_creator)

    whats_new_data_source = WhatsNewDataSource(server_instance, None)
    expected_whats_new_changes_list = [
      {
        'version': 22,
        'additionsToExistingApis': [{
          'version': 22,
          'type': 'additionsToExistingApis',
          'id': 'backgroundpages.to-be-non-persistent',
          'description': 'backgrounds to be non persistent'
          }
        ],
      },
      {
        'version': 21,
        'additionsToExistingApis': [{
          'version': 21,
          'type': 'additionsToExistingApis',
          'id': 'chromeSetting.set-regular-only-scope',
          'description': 'ChromeSetting.set now has a regular_only scope.'
          }
        ],
      },
      {
        'version': 20,
        'manifestChanges': [{
          'version': 20,
          'type': 'manifestChanges',
          'id': 'manifest-v1-deprecated',
          'description': 'Manifest version 1 was deprecated in Chrome 18'
          }
        ],
      }
    ]

    expected_new_info_of_apps = [
      {
        'version': 26,
        'apis': [{
          'version': 26, 'type': 'apis',
          'name': u'alarm',
          'description': u'<code>alarm</code>'
          },
          {
          'version': 26, 'type': 'apis',
          'name': u'app.window',
          'description': u'<code>app.window</code>'
          }
        ]
      }
    ]
    expected_new_info_of_apps.extend(expected_whats_new_changes_list)

    expected_new_info_of_extensions = [
      {
        'version': 26,
        'apis': [{
          'version': 26, 'type': 'apis',
          'name': u'alarm',
          'description': u'<code>alarm</code>'
          },
          {
          'version': 26, 'type': 'apis',
          'name': u'browserAction',
          'description': u'<code>browserAction</code>'
          }
        ]
      }
    ]
    expected_new_info_of_extensions.extend(expected_whats_new_changes_list)

    whats_new_for_apps = whats_new_data_source.get('apps')
    whats_new_for_extension = whats_new_data_source.get('extensions')
    self.assertEqual(expected_new_info_of_apps, whats_new_for_apps)
    self.assertEqual(expected_new_info_of_extensions, whats_new_for_extension)

if __name__ == '__main__':
  unittest.main()