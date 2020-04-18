#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
from operator import itemgetter
import unittest

from extensions_paths import CHROME_EXTENSIONS
from permissions_data_source import PermissionsDataSource
from server_instance import ServerInstance
from third_party.motemplate import Motemplate
from test_file_system import TestFileSystem


_PERMISSION_FEATURES = {
  # This will appear for extensions with a description as defined in the
  # permissions.json file.
  'activeTab': {
    'extension_types': ['extension'],
  },
  # This will appear for apps and extensions with an auto-generated description
  # since the entry appears in _api_features.json.
  'alarms': {
    'extension_types': ['platform_app', 'extension'],
  },
  # This won't appear for anything since there's no entry in permissions.json
  # and it's not an API.
  'audioCapture': {
    'extension_types': ['platform_app'],
  },
  # This won't appear for anything because it's private.
  'commandLinePrivate': {
    'extension_types': ['platform_app', 'extension']
  },
  # This will only appear for apps with an auto-generated description because
  # it's an API.
  'cookies': {
    'extension_types': ['platform_app']
  },
  'host-permissions': {}
}


_PERMISSIONS_JSON = {
  # This will appear for both apps and extensions with a custom description,
  # anchor, etc.
  'host-permissions': {
    'anchor': 'custom-anchor',
    'extension_types': ['platform_app', 'extension'],
    'literal_name': True,
    'name': 'match pattern',
    'partial': 'permissions/host_permissions.html',
  },
  # A custom 'partial' here overrides the default partial.
  'activeTab': {
    'partial': 'permissions/active_tab.html'
  },
}


_PERMISSIONS_PARTIALS = {
  'active_tab.html': 'active tab',
  'host_permissions.html': 'host permissions',
  'generic_description.html': 'generic description',
}


_API_FEATURES = {
  'alarms': {
    'dependencies': ['permission:alarms']
  },
  'cookies': {
    'dependencies': ['permission:cookies']
  },
}


class PermissionsDataSourceTest(unittest.TestCase):
  def testCreatePermissionsDataSource(self):
    expected_extensions = [
      {
        'anchor': 'custom-anchor',
        'description': 'host permissions',
        'extension_types': ['platform_app', 'extension'],
        'literal_name': True,
        'name': 'match pattern',
        'channel': 'stable'
      },
      {
        'anchor': 'activeTab',
        'description': 'active tab',
        'extension_types': ['extension'],
        'name': 'activeTab',
        'channel': 'stable'
      },
      {
        'anchor': 'alarms',
        'description': 'generic description',
        'extension_types': ['platform_app', 'extension'],
        'name': 'alarms',
        'channel': 'stable'
      },
    ]

    expected_apps = [
      {
        'anchor': 'custom-anchor',
        'description': 'host permissions',
        'extension_types': ['platform_app', 'extension'],
        'literal_name': True,
        'name': 'match pattern',
        'channel': 'stable'
      },
      {
        'anchor': 'alarms',
        'description': 'generic description',
        'extension_types': ['platform_app', 'extension'],
        'name': 'alarms',
        'channel': 'stable'
      },
      {
        'anchor': 'cookies',
        'description': 'generic description',
        'extension_types': ['platform_app'],
        'name': 'cookies',
        'channel': 'stable'
      },
    ]

    test_file_system = TestFileSystem({
      'api': {
        '_api_features.json': json.dumps(_API_FEATURES),
        '_manifest_features.json': '{}',
        '_permission_features.json': json.dumps(_PERMISSION_FEATURES),
      },
      'docs': {
        'templates': {
          'json': {
            'manifest.json': '{}',
            'permissions.json': json.dumps(_PERMISSIONS_JSON),
          },
          'private': {
            'permissions': _PERMISSIONS_PARTIALS
          },
        }
      }
    }, relative_to=CHROME_EXTENSIONS)

    permissions_data_source = PermissionsDataSource(
        ServerInstance.ForTest(test_file_system), None)

    actual_extensions = permissions_data_source.get('declare_extensions')
    actual_apps = permissions_data_source.get('declare_apps')

    # Normalise all test data.
    #   - Sort keys. Since the tests don't use OrderedDicts we can't make
    #     assertions about the order, which is unfortunate. Oh well.
    #   - Render all of the Handlerbar instances so that we can use ==.
    #     Motemplates don't implement __eq__, but they probably should.
    for lst in (actual_apps, actual_extensions,
                expected_apps, expected_extensions):
      lst.sort(key=itemgetter('name'))
      for mapping in lst:
        for key, value in mapping.iteritems():
          if isinstance(value, Motemplate):
            mapping[key] = value.Render().text

    self.assertEqual(expected_extensions, actual_extensions)
    self.assertEqual(expected_apps, actual_apps)


if __name__ == '__main__':
  unittest.main()
