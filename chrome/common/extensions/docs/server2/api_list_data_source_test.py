#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import json

from api_list_data_source import APIListDataSource
from api_models import ContentScriptAPI
from extensions_paths import CHROME_EXTENSIONS
from server_instance import ServerInstance
from test_file_system import TestFileSystem


def _ToTestData(obj):
  '''Transforms |obj| into test data by turning a list of files into an object
  mapping that file to its contents (derived from its name).
  '''
  return dict((name, name) for name in obj)


def _ToTestFeatures(names):
  '''Transforms a list of strings into a minimal JSON features object.
  '''
  def platforms_to_extension_types(platforms):
    return ['platform_app' if platform == 'apps' else 'extension'
            for platform in platforms]
  features = dict((name, {
                     'name': name,
                     'extension_types': platforms_to_extension_types(platforms),
                     'contexts': context
                   }) for name, platforms, context in names)
  features['sockets.udp']['channel'] = 'dev'
  return features


def _ToTestAPIData(names):
  api_data = dict((name, [{'namespace': name, 'description': description}])
              for name, description in names)
  return api_data


def _ToTestAPISchema(names, apis):
  for name, json_file in names:
    apis['api'][json_file] = json.dumps(_TEST_API_DATA[name])
  return apis


_TEST_API_FEATURES = _ToTestFeatures([
  ('alarms', ['apps', 'extensions'], ['content_script']),
  ('app.window', ['apps'], []),
  ('browserAction', ['extensions'], []),
  ('experimental.bluetooth', ['apps'], []),
  ('experimental.history', ['extensions'], []),
  ('experimental.power', ['apps', 'extensions'], []),
  ('extension', ['extensions'], ['content_script']),
  ('extension.onRequest', ['extensions'], ['content_script']),
  ('extension.sendNativeMessage', ['extensions'], []),
  ('extension.sendRequest', ['extensions'], ['content_script']),
  ('infobars', ['extensions'], []),
  ('something_internal', ['apps'], []),
  ('something_else_internal', ['extensions'], []),
  ('storage', ['apps', 'extensions'], []),
  ('sockets.udp', ['apps', 'extensions'], [])
])


_TEST_API_DATA = _ToTestAPIData([
  ('alarms', u'<code>alarms</code>'),
  ('app.window', u'<code>app.window</code>'),
  ('browserAction', u'<code>browserAction</code>'),
  ('experimental.bluetooth', u'<code>experimental.bluetooth</code>'),
  ('experimental.history', u'<code>experimental.history</code>'),
  ('experimental.power', u'<code>experimental.power</code>'),
  ('extension', u'<code>extension</code>'),
  ('infobars', u'<code>infobars</code>'),
  ('something_internal', u'<code>something_internal</code>'),
  ('something_else_internal', u'<code>something_else_internal</code>'),
  ('storage', u'<code>storage</code>'),
  ('sockets.udp', u'<code>sockets.udp</code>')
])


_TEST_API_SCHEMA = [
  ('alarms', 'alarms.json'),
  ('app.window', 'app_window.json'),
  ('browserAction', 'browser_action.json'),
  ('experimental.bluetooth', 'experimental_bluetooth.json'),
  ('experimental.history', 'experimental_history.json'),
  ('experimental.power', 'experimental_power.json'),
  ('extension', 'extension.json'),
  ('infobars', 'infobars.json'),
  ('something_internal', 'something_internal.json'),
  ('something_else_internal', 'something_else_internal.json'),
  ('storage', 'storage.json'),
  ('sockets.udp', 'sockets_udp.json')
]


_TEST_DATA = _ToTestAPISchema(_TEST_API_SCHEMA, {
  'api': {
    '_api_features.json': json.dumps(_TEST_API_FEATURES),
    '_manifest_features.json': '{}',
    '_permission_features.json': '{}',
  },
  'docs': {
    'templates': {
      'json': {
        'api_availabilities.json': '{}',
        'manifest.json': '{}',
        'permissions.json': '{}',
      },
      'public': {
        'apps': _ToTestData([
          'alarms.html',
          'app_window.html',
          'experimental_bluetooth.html',
          'experimental_power.html',
          'storage.html',
          'sockets_udp.html'
        ]),
        'extensions': _ToTestData([
          'alarms.html',
          'browserAction.html',
          'experimental_history.html',
          'experimental_power.html',
          'extension.html',
          'infobars.html',
          'storage.html',
          'sockets_udp.html'
        ]),
      },
    },
  },
})


class APIListDataSourceTest(unittest.TestCase):
  def setUp(self):
    server_instance = ServerInstance.ForTest(
        TestFileSystem(_TEST_DATA, relative_to=CHROME_EXTENSIONS))
    # APIListDataSource takes a request but doesn't use it,
    # so put None
    self._api_list = APIListDataSource(server_instance, None)
    self.maxDiff = None

  def testApps(self):
    self.assertEqual({
        'stable': [
          {
            'name': 'alarms',
            'version': 5,
            'description': u'<code>alarms</code>'
          },
          {
            'name': 'app.window',
            # Availability logic will look for a camelCase format filename
            # (i.e. 'app.window.html') at version 20 and below, but the
            # unix_name format above won't be found at these versions.
            'version': 21,
            'description': u'<code>app.window</code>'
          },
          {
            'name': 'storage',
            'last': True,
            'version': 5,
            'description': u'<code>storage</code>'
          }],
        'dev': [
          {
            'name': 'sockets.udp',
            'last': True,
            'description': u'<code>sockets.udp</code>'
          }],
        'beta': [],
        'master': []
        }, self._api_list.get('apps').get('chrome'))

  def testExperimentalApps(self):
    self.assertEqual([
        {
          'name': 'experimental.bluetooth',
          'description': u'<code>experimental.bluetooth</code>'
        },
        {
          'name': 'experimental.power',
          'last': True,
          'description': u'<code>experimental.power</code>'
        }], self._api_list.get('apps').get('experimental'))

  def testExtensions(self):
    self.assertEqual({
        'stable': [
          {
            'name': 'alarms',
            'version': 5,
            'description': u'<code>alarms</code>'
          },
          {
            'name': 'browserAction',
            # See comment above for 'app.window'.
            'version': 21,
            'description': u'<code>browserAction</code>'
          },
          {
            'name': 'extension',
            'version': 5,
            'description': u'<code>extension</code>'
          },
          {
            'name': 'infobars',
            'version': 5,
            'description': u'<code>infobars</code>'
          },
          {
            'name': 'storage',
            'last': True,
            'version': 5,
            'description': u'<code>storage</code>'
          }],
        'dev': [
          {
            'name': 'sockets.udp',
            'last': True,
            'description': u'<code>sockets.udp</code>'
          }],
        'beta': [],
        'master': []
        }, self._api_list.get('extensions').get('chrome'))

  def testExperimentalExtensions(self):
    self.assertEqual([
        {
          'name': 'experimental.history',
          'description': u'<code>experimental.history</code>'
        },
        {
          'name': 'experimental.power',
          'description': u'<code>experimental.power</code>',
          'last': True
        }], self._api_list.get('extensions').get('experimental'))

  def testContentScripts(self):
    self.assertEqual([{
      'name': 'alarms',
    },
    {
      'name': 'extension',
      'restrictedTo': [{
        'node': 'onRequest',
        'first': True
      },
      {
        'node': 'sendRequest',
        'last': True
      }]
    }], self._api_list.get('contentScripts'))

if __name__ == '__main__':
  unittest.main()
