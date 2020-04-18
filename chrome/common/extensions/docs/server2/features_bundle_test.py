#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from extensions_paths import CHROME_EXTENSIONS
from server_instance import ServerInstance
from test_file_system import TestFileSystem


_TEST_FILESYSTEM = {
  'api': {
    '_api_features.json': json.dumps({
      'audioCapture': {
        'channel': 'stable',
        'extension_types': ['platform_app']
      },
      'background': [
        {
          'channel': 'stable',
          'extension_types': ['extension']
        },
        {
          'channel': 'stable',
          'extension_types': ['platform_app'],
          'whitelist': ['im not here']
        }
      ],
      'inheritsFromDifferentDependencyName': {
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency']
      },
      'omnibox': {
        'dependencies': ['manifest:omnibox'],
        'contexts': ['blessed_extension']
      },
      'syncFileSystem': {
        'dependencies': ['permission:syncFileSystem'],
        'contexts': ['blessed_extension']
      },
      'tabs': {
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app'],
        'contexts': ['blessed_extension']
      },
      'test': {
        'channel': 'stable',
        'extension_types': 'all',
        'contexts': [
            'blessed_extension', 'unblessed_extension', 'content_script']
      },
      'overridesPlatformAndChannelFromDependency': {
        'channel': 'beta',
        'dependencies': [
          'permission:overridesPlatformAndChannelFromDependency'
        ],
        'extension_types': ['platform_app']
      },
      'windows': {
        'dependencies': ['api:tabs'],
        'contexts': ['blessed_extension']
      },
      'testDeep1': {
        'dependencies': ['api:testDeep2']
      },
      'testDeep2': {
        'dependencies': ['api:testDeep3']
      },
      'testDeep3': {
        'dependencies': ['manifest:testDeep4']
      },
      'testDeep1.child': {},
      'multipleAmbiguous': [{
        'value': 1,
        'extension_types': ['platform_app']
      }, {
        'value': 2,
        'dependencies': ['manifest:multipleAmbiguous']
      }],
      'mergingDependencies1': {
        'dependencies': [
          'permission:mergingDependencies1',
          'permission:mergingDependencies2'
        ]
      },
      'mergingDependencies2': {
        'dependencies': [
          'permission:mergingDependencies1',
          'permission:mergingDependencies3'
        ]
      },
      'mergingDependencies3': {
        'dependencies': [
          'permission:mergingDependencies2',
          'permission:mergingDependencies3'
        ]
      },
      'implicitNoParent.child': {
        'extension_types': ['extension'],
        'channel': 'stable'
      },
      'parent': {
        'extension_types': ['extension'],
        'channel': 'beta'
      },
      'parent.explicitNoParent': {
        'extension_types': ['extension'],
        'noparent': True
      },
      'parent.inheritAndOverride': {
        'channel': 'dev'
      },
      'overridePlatform': {
        'dependencies': ['permission:tabs'],
        'extension_types': 'platform_app'
      },
      'mergeMostStableChannel': [{
        'channel': 'dev',
        'extension_types': ['extension'],
        'value1': 1
      }, {
        'channel': 'stable',
        'extension_types': ['extension'],
        'value2': 2
      }, {
        'channel': 'beta',
        'extension_types': ['extension'],
        'value3': 3
      }, {
        'channel': 'stable',
        'extension_types': ['extension'],
        'value4': 4
      }, {
        'extension_types': ['extension'],
        'value5': 5
      }]
    }),
    '_manifest_features.json': json.dumps({
      'app.content_security_policy': {
        'channel': 'stable',
        'extension_types': ['platform_app'],
        'min_manifest_version': 2,
        'whitelist': ['this isnt happening']
      },
      'background': {
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app', 'hosted_app']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'channel': 'dev',
        'extension_types': ['extension']
      },
      'manifest_version': {
        'channel': 'stable',
        'extension_types': 'all'
      },
      'omnibox': {
        'channel': 'stable',
        'extension_types': ['extension'],
        'platforms': ['win']
      },
      'page_action': {
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'sockets': {
        'channel': 'dev',
        'extension_types': ['platform_app']
      },
      'testDeep4': {
        'extension_types': ['extension']
      },
      'multipleAmbiguous': {
        'extension_types': ['extension']
      }
    }),
    '_permission_features.json': json.dumps({
      'bluetooth': {
        'channel': 'dev',
        'extension_types': ['platform_app']
      },
      'overridesPlatformAndChannelFromDependency': {
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'power': {
        'channel': 'stable',
        'extension_types': [
          'extension', 'legacy_packaged_app', 'platform_app'
        ]
      },
      'syncFileSystem': {
        'channel': 'stable',
        'extension_types': ['platform_app']
      },
      'tabs': {
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'mergingDependencies1': {
        'channel': 'stable',
        'extension_types': 'all'
      },
      'mergingDependencies2': {
        'channel': 'beta',
        'extension_types': ['platform_app']
      },
      'mergingDependencies3': {
        'extension_types': ['extension']
      },
      'defaults': {}
    })
  },
  'docs': {
    'templates': {
      'json': {
        'manifest.json': json.dumps({
          'background': {
            'documentation': 'background_pages.html'
          },
          'manifest_version': {
            'documentation': 'manifest/manifest_version.html',
            'example': 2,
            'level': 'required'
          },
          'page_action': {
            'documentation': 'pageAction.html',
            'example': {},
            'level': 'only_one'
          }
        }),
        'permissions.json': json.dumps({
          'fakeUnsupportedFeature': {},
          'syncFileSystem': {
            'partial': 'permissions/sync_file_system.html'
          },
          'tabs': {
            'partial': 'permissions/tabs.html'
          }
        })
      }
    }
  }
}


class FeaturesBundleTest(unittest.TestCase):
  def setUp(self):
    self._server = ServerInstance.ForTest(
        TestFileSystem(_TEST_FILESYSTEM, relative_to=CHROME_EXTENSIONS))

  def testManifestFeatures(self):
    expected_features = {
      'background': {
        'name': 'background',
        'channel': 'stable',
        'documentation': 'background_pages.html',
        'extension_types': ['extension', 'legacy_packaged_app', 'hosted_app']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'name': 'inheritsPlatformAndChannelFromDependency',
        'channel': 'dev',
        'extension_types': ['extension']
      },
      'manifest_version': {
        'name': 'manifest_version',
        'channel': 'stable',
        'documentation': 'manifest/manifest_version.html',
        'extension_types': 'all',
        'level': 'required',
        'example': 2,
      },
      'omnibox': {
        'name': 'omnibox',
        'channel': 'stable',
        'extension_types': ['extension'],
        'platforms': ['win'],
      },
      'page_action': {
        'name': 'page_action',
        'channel': 'stable',
        'documentation': 'pageAction.html',
        'extension_types': ['extension'],
        'level': 'only_one',
        'example': {},
      },
      'testDeep4': {
        'name': 'testDeep4',
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'multipleAmbiguous': {
        'name': 'multipleAmbiguous',
        'channel': 'stable',
        'extension_types': ['extension']
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'extensions').GetManifestFeatures().Get())
    expected_features = {
      'manifest_version': {
        'name': 'manifest_version',
        'channel': 'stable',
        'documentation': 'manifest/manifest_version.html',
        'extension_types': 'all',
        'level': 'required',
        'example': 2,
      },
      'sockets': {
        'name': 'sockets',
        'channel': 'dev',
        'extension_types': ['platform_app']
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'apps').GetManifestFeatures().Get())

  def testPermissionFeatures(self):
    expected_features = {
      'power': {
        'name': 'power',
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app', 'platform_app']
      },
      'overridesPlatformAndChannelFromDependency': {
        'name': 'overridesPlatformAndChannelFromDependency',
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'tabs': {
        'name': 'tabs',
        'channel': 'stable',
        'extension_types': ['extension'],
        'partial': 'permissions/tabs.html'
      },
      'mergingDependencies1': {
        'name': 'mergingDependencies1',
        'channel': 'stable',
        'extension_types': 'all'
      },
      'mergingDependencies3': {
        'name': 'mergingDependencies3',
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'defaults': {
        'name': 'defaults',
        'channel': 'stable'
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'extensions').GetPermissionFeatures().Get())
    expected_features = {
      'bluetooth': {
        'name': 'bluetooth',
        'channel': 'dev',
        'extension_types': ['platform_app']
      },
      'power': {
        'name': 'power',
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app', 'platform_app']
      },
      'syncFileSystem': {
        'name': 'syncFileSystem',
        'channel': 'stable',
        'extension_types': ['platform_app'],
        'partial': 'permissions/sync_file_system.html'
      },
      'mergingDependencies1': {
        'name': 'mergingDependencies1',
        'channel': 'stable',
        'extension_types': 'all'
      },
      'mergingDependencies2': {
        'name': 'mergingDependencies2',
        'channel': 'beta',
        'extension_types': ['platform_app']
      },
      'defaults': {
        'name': 'defaults',
        'channel': 'stable'
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'apps').GetPermissionFeatures().Get())

  def testAPIFeatures(self):
    expected_features = {
      'background': {
        'name': 'background',
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'omnibox': {
        'name': 'omnibox',
        'contexts': ['blessed_extension'],
        'dependencies': ['manifest:omnibox'],
        'channel': 'stable'
      },
      'tabs': {
        'name': 'tabs',
        'channel': 'stable',
        'contexts': ['blessed_extension'],
        'extension_types': ['extension', 'legacy_packaged_app'],
      },
      'test': {
        'name': 'test',
        'channel': 'stable',
        'contexts': [
            'blessed_extension', 'unblessed_extension', 'content_script'],
        'extension_types': 'all',
      },
      'windows': {
        'name': 'windows',
        'contexts': ['blessed_extension'],
        'dependencies': ['api:tabs'],
        'channel': 'stable'
      },
      'testDeep1': {
        'name': 'testDeep1',
        'dependencies': ['api:testDeep2'],
        'channel': 'stable'
      },
      'testDeep2': {
        'name': 'testDeep2',
        'dependencies': ['api:testDeep3'],
        'channel': 'stable'
      },
      'testDeep3': {
        'name': 'testDeep3',
        'dependencies': ['manifest:testDeep4'],
        'channel': 'stable'
      },
      'testDeep1.child': {
        'name': 'testDeep1.child',
        'channel': 'stable',
        'dependencies': ['api:testDeep2']
      },
      'multipleAmbiguous': {
        'name': 'multipleAmbiguous',
        'value': 2,
        'dependencies': ['manifest:multipleAmbiguous'],
        'channel': 'stable'
      },
      'mergingDependencies2': {
        'name': 'mergingDependencies2',
        'dependencies': [
          'permission:mergingDependencies1',
          'permission:mergingDependencies3'
        ],
        'channel': 'stable'
      },
      'inheritsFromDifferentDependencyName': {
        'channel': 'dev',
        'name': 'inheritsFromDifferentDependencyName',
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency'],
      },
      'inheritsPlatformAndChannelFromDependency': {
        'channel': 'dev',
        'name': 'inheritsPlatformAndChannelFromDependency',
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency'],
      },
      'implicitNoParent.child': {
        'name': 'implicitNoParent.child',
        'channel': 'stable',
        'extension_types': ['extension'],
      },
      'parent': {
        'name': 'parent',
        'channel': 'beta',
        'extension_types': ['extension'],
      },
      'parent.explicitNoParent': {
        'name': 'parent.explicitNoParent',
        'channel': 'stable',
        'extension_types': ['extension'],
        'noparent': True
      },
      'parent.inheritAndOverride': {
        'name': 'parent.inheritAndOverride',
        'channel': 'dev',
        'extension_types': ['extension']
      },
      'mergeMostStableChannel': {
        'name': 'mergeMostStableChannel',
        'channel': 'stable',
        'extension_types': ['extension'],
        'value2': 2,
        'value4': 4,
        'value5': 5
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'extensions').GetAPIFeatures().Get())
    expected_features = {
      'audioCapture': {
        'name': 'audioCapture',
        'channel': 'stable',
        'extension_types': ['platform_app']
      },
      'syncFileSystem': {
        'name': 'syncFileSystem',
        'contexts': ['blessed_extension'],
        'dependencies': ['permission:syncFileSystem'],
        'channel': 'stable'
      },
      'test': {
        'name': 'test',
        'channel': 'stable',
        'contexts': [
            'blessed_extension', 'unblessed_extension', 'content_script'],
        'extension_types': 'all',
      },
      'multipleAmbiguous': {
        'name': 'multipleAmbiguous',
        'channel': 'stable',
        'extension_types': ['platform_app'],
        'value': 1,
      },
      'mergingDependencies1': {
        'name': 'mergingDependencies1',
        'channel': 'beta',
        'dependencies': [
          'permission:mergingDependencies1',
          'permission:mergingDependencies2'
        ],
      },
      'overridesPlatformAndChannelFromDependency': {
        'name': 'overridesPlatformAndChannelFromDependency',
        'channel': 'beta',
        'dependencies': [
          'permission:overridesPlatformAndChannelFromDependency'
        ],
        'extension_types': ['platform_app']
      },
      'overridePlatform': {
        'name': 'overridePlatform',
        'channel': 'stable',
        'dependencies': ['permission:tabs'],
        'extension_types': 'platform_app'
      }
    }
    self.assertEqual(
        expected_features,
        self._server.platform_bundle.GetFeaturesBundle(
            'apps').GetAPIFeatures().Get())


if __name__ == '__main__':
  unittest.main()
