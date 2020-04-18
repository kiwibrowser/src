#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from extensions_paths import CHROME_API, CHROME_EXTENSIONS, EXTENSIONS_API
from mock_file_system import MockFileSystem
from server_instance import ServerInstance
from test_file_system import TestFileSystem
from test_util import ReadFile


_TEST_DATA = {
  'api': {
    'devtools': {
      'inspected_window.json': ReadFile(
          CHROME_API, 'devtools', 'inspected_window.json'),
    },
    '_api_features.json': json.dumps({
      'alarms': {},
      'app': {'extension_types': ['platform_app']},
      'app.runtime': {'noparent': True},
      'app.runtime.foo': {'extension_types': ['extension']},
      'declarativeWebRequest': {'extension_types': ['extension']},
      'devtools.inspectedWindow': {'extension_types': ['extension']},
      'input': {'extension_types': 'all'},
      'input.ime': {'extension_types': ['extension', 'platform_app']},
      'storage': {'extension_types': ['extension']},
    }),
    '_manifest_features.json': '{}',
    '_permission_features.json': '{}',
    'alarms.idl': ReadFile(EXTENSIONS_API, 'alarms.idl'),
    'input_ime.json': ReadFile(CHROME_API, 'input_ime.json'),
    'page_action.json': ReadFile(CHROME_API, 'page_action.json'),
  },
  'docs': {
    'templates': {
      'json': {
        'manifest.json': '{}',
        'permissions.json': '{}',
      }
    }
  },
}


class PlatformBundleTest(unittest.TestCase):
  def setUp(self):
    mock_file_system = MockFileSystem(
        TestFileSystem(_TEST_DATA, relative_to=CHROME_EXTENSIONS))
    server_instance = ServerInstance.ForTest(file_system=mock_file_system)
    self._platform_bundle = server_instance.platform_bundle

  def testGetters(self):
    self.assertEqual([
      'alarms',
      'app.runtime',
      'declarativeWebRequest',
      'devtools.inspectedWindow',
      'input',
      'storage'
    ], sorted(self._platform_bundle.GetAPIModels('extensions').GetNames()))

    self.assertEqual([
      'alarms',
      'app',
      'app.runtime',
      'input'
    ], sorted(self._platform_bundle.GetAPIModels('apps').GetNames()))

    self.assertEqual({
      'app.runtime': {
        'name': 'app.runtime',
        'noparent': True,
        'channel': 'stable'
      },
      'declarativeWebRequest': {
        'name': 'declarativeWebRequest',
        'channel': 'stable',
        'extension_types': ['extension'],
      },
      'app.runtime.foo': {
        'name': 'app.runtime.foo',
        'channel': 'stable',
        'extension_types': ['extension'],
      },
      'storage': {
        'name': 'storage',
        'channel': 'stable',
        'extension_types': ['extension'],
      },
      'input.ime': {
        'name': 'input.ime',
        'channel': 'stable',
        'extension_types': ['extension', 'platform_app'],
      },
      'alarms': {
        'name': 'alarms',
        'channel': 'stable'
      },
      'input': {
        'name': 'input',
        'channel': 'stable',
        'extension_types': 'all'
      },
      'devtools.inspectedWindow': {
        'name': 'devtools.inspectedWindow',
        'channel': 'stable',
        'extension_types': ['extension'],
      }
    }, self._platform_bundle.GetFeaturesBundle(
        'extensions').GetAPIFeatures().Get())

    self.assertEqual({
      'app.runtime': {
        'name': 'app.runtime',
        'noparent': True,
        'channel': 'stable'
      },
      'input': {
        'name': 'input',
        'channel': 'stable',
        'extension_types': 'all'
      },
      'input.ime': {
        'name': 'input.ime',
        'channel': 'stable',
        'extension_types': ['extension', 'platform_app'],
      },
      'app': {
        'name': 'app',
        'channel': 'stable',
        'extension_types': ['platform_app'],
      },
      'alarms': {
        'name': 'alarms',
        'channel': 'stable'
      }
    }, self._platform_bundle.GetFeaturesBundle('apps').GetAPIFeatures().Get())

    # Check that 'app' is resolved successfully in apps, but is None otherwise.
    self.assertNotEqual(
        None,
        self._platform_bundle.GetReferenceResolver('apps').GetLink('app'))
    self.assertEqual(
        None,
        self._platform_bundle.GetReferenceResolver('extensions').GetLink('app'))

if __name__ == '__main__':
  unittest.main()
