#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from api_models import APIModels
from compiled_file_system import CompiledFileSystem
from extensions_paths import (API_PATHS, CHROME_API, CHROME_EXTENSIONS,
    EXTENSIONS_API)
from features_bundle import FeaturesBundle
from file_system import FileNotFoundError
from mock_file_system import MockFileSystem
from object_store_creator import ObjectStoreCreator
from test_file_system import TestFileSystem
from test_util import ReadFile
from future import Future
from schema_processor import SchemaProcessorFactoryForTest


_TEST_DATA = {
  'api': {
    'devtools': {
      'inspected_window.json': ReadFile(
          CHROME_API, 'devtools', 'inspected_window.json'),
    },
    '_api_features.json': json.dumps({
      'alarms': {},
      'app': {},
      'app.runtime': {'noparent': True},
      'app.runtime.foo': {},
      'declarativeWebRequest': {},
      'devtools.inspectedWindow': {},
      'input': {},
      'input.ime': {},
      'storage': {},
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


class APIModelsTest(unittest.TestCase):
  def setUp(self):
    object_store_creator = ObjectStoreCreator.ForTest()
    compiled_fs_factory = CompiledFileSystem.Factory(object_store_creator)
    self._mock_file_system = MockFileSystem(
        TestFileSystem(_TEST_DATA, relative_to=CHROME_EXTENSIONS))
    features_bundle = FeaturesBundle(self._mock_file_system,
                                     compiled_fs_factory,
                                     object_store_creator,
                                     'extensions')
    self._api_models = APIModels(features_bundle,
                                 compiled_fs_factory,
                                 self._mock_file_system,
                                 object_store_creator,
                                 'extensions',
                                 SchemaProcessorFactoryForTest())

  def testGetNames(self):
    # Both 'app' and 'app.runtime' appear here because 'app.runtime' has
    # noparent:true, but 'app.runtime.foo' etc doesn't so it's a sub-feature of
    # 'app.runtime' not a separate API. 'devtools.inspectedWindow' is an API
    # because there is no 'devtools'.
    self.assertEqual(
        ['alarms', 'app', 'app.runtime', 'declarativeWebRequest',
         'devtools.inspectedWindow', 'input', 'storage'],
        sorted(self._api_models.GetNames()))

  def testGetModel(self):
    def get_model_name(api_name):
      return self._api_models.GetModel(api_name).Get().name
    self.assertEqual('devtools.inspectedWindow',
                     get_model_name('devtools.inspectedWindow'))
    self.assertEqual('devtools.inspectedWindow',
                     get_model_name('devtools/inspected_window.json'))
    self.assertEqual('devtools.inspectedWindow',
                     get_model_name(CHROME_API +
                                    'devtools/inspected_window.json'))
    self.assertEqual('alarms', get_model_name('alarms'))
    self.assertEqual('alarms', get_model_name('alarms.idl'))
    self.assertEqual('alarms', get_model_name(CHROME_API + 'alarms.idl'))
    self.assertEqual('input.ime', get_model_name('input.ime'))
    self.assertEqual('input.ime', get_model_name('input_ime.json'))
    self.assertEqual('input.ime',
                     get_model_name(CHROME_API + 'input_ime.json'))
    self.assertEqual('pageAction', get_model_name('pageAction'))
    self.assertEqual('pageAction', get_model_name('page_action.json'))
    self.assertEqual('pageAction', get_model_name(CHROME_API +
                                                  'page_action.json'))

  def testGetNonexistentModel(self):
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel('declarativeWebRequest').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(
                          'declarative_web_request.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(
                          CHROME_API + 'declarative_web_request.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel('notfound').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel('notfound.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(CHROME_API +
                                                'notfound.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(CHROME_API +
                                                'alarms.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel('storage').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(CHROME_API +
                                                'storage.json').Get)
    self.assertRaises(FileNotFoundError,
                      self._api_models.GetModel(CHROME_API +
                                                'storage.idl').Get)

  def testSingleFile(self):
    # 2 stats (1 for JSON and 1 for IDL) for each available API path.
    # 1 read (for IDL file which existed).
    future = self._api_models.GetModel('alarms')
    self.assertTrue(*self._mock_file_system.CheckAndReset(
        read_count=1, stat_count=len(API_PATHS)*2))

    # 1 read-resolve (for the IDL file).
    #
    # The important part here and above is that it's only doing a single read;
    # any more would break the contract that only a single file is accessed -
    # see the SingleFile annotation in api_models._CreateAPIModel.
    future.Get()
    self.assertTrue(*self._mock_file_system.CheckAndReset(
        read_resolve_count=1))

    # 2 stats (1 for JSON and 1 for IDL) for each available API path.
    # No reads (still cached).
    future = self._api_models.GetModel('alarms')
    self.assertTrue(*self._mock_file_system.CheckAndReset(
        stat_count=len(API_PATHS)*2))
    future.Get()
    self.assertTrue(*self._mock_file_system.CheckAndReset())


if __name__ == '__main__':
  unittest.main()
