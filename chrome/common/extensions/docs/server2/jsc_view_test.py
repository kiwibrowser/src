#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import unittest

from jsc_view import GetEventByNameFromEvents
from api_schema_graph import APISchemaGraph
from availability_finder import AvailabilityFinder, AvailabilityInfo
from branch_utility import BranchUtility, ChannelInfo
from compiled_file_system import CompiledFileSystem
from extensions_paths import CHROME_EXTENSIONS
from fake_host_file_system_provider import FakeHostFileSystemProvider
from fake_url_fetcher import FakeUrlFetcher
from features_bundle import FeaturesBundle
from future import Future
from host_file_system_iterator import HostFileSystemIterator
from jsc_view import CreateJSCView, _JSCViewBuilder, _FormatValue
from object_store_creator import ObjectStoreCreator
from schema_processor import SchemaProcessorFactoryForTest
from servlet import Request
from server_instance import ServerInstance
from test_data.api_data_source.canned_master_fs import CANNED_MASTER_FS_DATA
from test_data.canned_data import CANNED_API_FILE_SYSTEM_DATA
from test_data.object_level_availability.tabs import TABS_SCHEMA_BRANCHES
from test_file_system import TestFileSystem
from test_util import Server2Path


class _FakeTemplateCache(object):

  def GetFromFile(self, key):
    return Future(value='motemplate %s' % key)


class _FakeFeaturesBundle(object):
  def GetAPIFeatures(self):
    return Future(value={
      'bluetooth': {'value': True},
      'contextMenus': {'value': True},
      'jsonStableAPI': {'value': True},
      'idle': {'value': True},
      'input.ime': {'value': True},
      'tabs': {'value': True}
    })


class _FakeAvailabilityFinder(object):
  def __init__(self, fake_availability):
    self._fake_availability = fake_availability

  def GetAPIAvailability(self, api_name):
    return self._fake_availability

  def GetAPINodeAvailability(self, api_name):
    schema_graph = APISchemaGraph()
    api_graph = APISchemaGraph(json.loads(
        CANNED_MASTER_FS_DATA['api'][api_name + '.json']))
    # Give the graph fake ChannelInfo; it's not used in tests.
    channel_info = ChannelInfo('stable', '28', 28)
    schema_graph.Update(api_graph, lambda _: channel_info)
    return schema_graph


class JSCViewTest(unittest.TestCase):
  def setUp(self):
    self._base_path = Server2Path('test_data', 'test_json')

    server_instance = ServerInstance.ForTest(
        TestFileSystem(CANNED_MASTER_FS_DATA, relative_to=CHROME_EXTENSIONS))
    file_system = server_instance.host_file_system_provider.GetMaster()
    self._json_cache = server_instance.compiled_fs_factory.ForJson(file_system)
    self._features_bundle = FeaturesBundle(file_system,
                                           server_instance.compiled_fs_factory,
                                           server_instance.object_store_creator,
                                           'extensions')
    self._api_models = server_instance.platform_bundle.GetAPIModels(
        'extensions')
    self._fake_availability = AvailabilityInfo(ChannelInfo('stable', '396', 5))

  def _ReadLocalFile(self, filename):
    with open(os.path.join(self._base_path, filename), 'r') as f:
      return f.read()

  def _LoadJSON(self, filename):
    return json.loads(self._ReadLocalFile(filename))

  def _FakeLoadAddRulesSchema(self):
    events = self._LoadJSON('add_rules_def_test.json')
    return Future(value=GetEventByNameFromEvents(events))

  def testFormatValue(self):
    self.assertEquals('1,234,567', _FormatValue(1234567))
    self.assertEquals('67', _FormatValue(67))
    self.assertEquals('234,567', _FormatValue(234567))

  def testGetEventByNameFromEvents(self):
    events = {}
    # Missing 'types' completely.
    self.assertRaises(AssertionError, GetEventByNameFromEvents, events)

    events['types'] = []
    # No type 'Event' defined.
    self.assertRaises(AssertionError, GetEventByNameFromEvents, events)

    events['types'].append({ 'name': 'Event',
                             'functions': []})
    add_rules = { "name": "addRules" }
    events['types'][0]['functions'].append(add_rules)
    self.assertEqual(add_rules,
                     GetEventByNameFromEvents(events)['addRules'])

    events['types'][0]['functions'].append(add_rules)
    # Duplicates are an error.
    self.assertRaises(AssertionError, GetEventByNameFromEvents, events)

  def testCreateId(self):
    fake_avail_finder = _FakeAvailabilityFinder(self._fake_availability)
    dict_ = CreateJSCView(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('tester').Get(),
        fake_avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        self._features_bundle,
        None,
        'extensions',
        [],
        Request.ForTest(''))
    self.assertEquals('type-TypeA', dict_['types'][0]['id'])
    self.assertEquals('property-TypeA-b',
                      dict_['types'][0]['properties'][0]['id'])
    self.assertEquals('method-get', dict_['functions'][0]['id'])
    self.assertEquals('event-EventA', dict_['events'][0]['id'])

  # TODO(kalman): re-enable this when we have a rebase option.
  def DISABLED_testToDict(self):
    fake_avail_finder = _FakeAvailabilityFinder(self._fake_availability)
    expected_json = self._LoadJSON('expected_tester.json')
    dict_ = CreateJSCView(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('tester').Get(),
        fake_avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        self._features_bundle,
        None,
        'extensions',
        [],
        Request.ForTest(''))
    self.assertEquals(expected_json, dict_)

  def testAddRules(self):
    fake_avail_finder = _FakeAvailabilityFinder(self._fake_availability)
    dict_ = CreateJSCView(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('add_rules_tester').Get(),
        fake_avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        self._features_bundle,
        self._FakeLoadAddRulesSchema(),
        'extensions',
        [],
        Request.ForTest(''))

    # Check that the first event has the addRulesFunction defined.
    self.assertEquals('add_rules_tester', dict_['name'])
    self.assertEquals('rules', dict_['events'][0]['name'])
    self.assertEquals('notable_name_to_check_for',
                      dict_['events'][0]['byName']['addRules'][
                          'parameters'][0]['name'])

    # Check that the second event has addListener defined.
    self.assertEquals('noRules', dict_['events'][1]['name'])
    self.assertEquals('add_rules_tester', dict_['name'])
    self.assertEquals('noRules', dict_['events'][1]['name'])
    self.assertEquals('callback',
                      dict_['events'][0]['byName']['addListener'][
                          'parameters'][0]['name'])

  def testGetIntroList(self):
    fake_avail_finder = _FakeAvailabilityFinder(self._fake_availability)
    model = _JSCViewBuilder(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('tester').Get(),
        fake_avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        self._features_bundle,
        None,
        'extensions',
        [])
    expected_list = [
      { 'title': 'Description',
        'content': [
          { 'text': 'a test api' }
        ]
      },
      { 'title': 'Availability',
        'content': [
          { 'partial': 'motemplate chrome/common/extensions/docs/' +
                       'templates/private/intro_tables/stable_message.html',
            'version': 5,
            'scheduled': None
          }
        ]
      },
      { 'title': 'Permissions',
        'content': [
          { 'class': 'override',
            'text': '"tester"'
          },
          { 'text': 'is an API for testing things.' }
        ]
      },
      { 'title': 'Manifest',
        'content': [
          { 'class': 'code',
            'text': '"tester": {...}'
          }
        ]
      },
      { 'title': 'Content Scripts',
        'content': [
          {
            'partial': 'motemplate chrome/common/extensions/docs' +
                       '/templates/private/intro_tables/content_scripts.html',
            'contentScriptSupport': {
              'name': 'tester',
              'restrictedTo': None
            }
          }
        ]
      },
      { 'title': 'Learn More',
        'content': [
          { 'link': 'https://tester.test.com/welcome.html',
            'text': 'Welcome!'
          }
        ]
      }
    ]
    self.assertEquals(model._GetIntroTableList(), expected_list)

    # Tests the same data with a scheduled availability.
    fake_avail_finder = _FakeAvailabilityFinder(
        AvailabilityInfo(ChannelInfo('beta', '1453', 27), scheduled=28))
    model = _JSCViewBuilder(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('tester').Get(),
        fake_avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        self._features_bundle,
        None,
        'extensions',
        [])
    expected_list[1] = {
      'title': 'Availability',
      'content': [
        { 'partial': 'motemplate chrome/common/extensions/docs/' +
                     'templates/private/intro_tables/beta_message.html',
          'version': 27,
          'scheduled': 28
        }
      ]
    }
    self.assertEquals(model._GetIntroTableList(), expected_list)


class JSCViewWithoutNodeAvailabilityTest(unittest.TestCase):
  def setUp(self):
    server_instance = ServerInstance.ForTest(
        file_system_provider=FakeHostFileSystemProvider(
            CANNED_API_FILE_SYSTEM_DATA))
    self._api_models = server_instance.platform_bundle.GetAPIModels(
        'extensions')
    self._json_cache = server_instance.compiled_fs_factory.ForJson(
        server_instance.host_file_system_provider.GetMaster())
    self._avail_finder = server_instance.platform_bundle.GetAvailabilityFinder(
        'extensions')


  def testGetAPIAvailability(self):
    api_availabilities = {
      'bluetooth': 31,
      'contextMenus': 'master',
      'jsonStableAPI': 20,
      'idle': 5,
      'input.ime': 18,
      'tabs': 18
    }
    for api_name, availability in api_availabilities.iteritems():
      model_dict = CreateJSCView(
          self._api_models.GetContentScriptAPIs().Get(),
          self._api_models.GetModel(api_name).Get(),
          self._avail_finder,
          self._json_cache,
          _FakeTemplateCache(),
          _FakeFeaturesBundle(),
          None,
          'extensions',
          [],
          Request.ForTest(''))
      self.assertEquals(availability,
                        model_dict['introList'][1]['content'][0]['version'])


class JSCViewWithNodeAvailabilityTest(unittest.TestCase):
  def setUp(self):
    tabs_unmodified_versions = (16, 20, 23, 24)
    self._branch_utility = BranchUtility(
        os.path.join('branch_utility', 'first.json'),
        os.path.join('branch_utility', 'second.json'),
        FakeUrlFetcher(Server2Path('test_data')),
        ObjectStoreCreator.ForTest())
    self._node_fs_creator = FakeHostFileSystemProvider(TABS_SCHEMA_BRANCHES)
    self._node_fs_iterator = HostFileSystemIterator(self._node_fs_creator,
                                                    self._branch_utility)
    test_object_store = ObjectStoreCreator.ForTest()
    self._avail_finder = AvailabilityFinder(
        self._branch_utility,
        CompiledFileSystem.Factory(test_object_store),
        self._node_fs_iterator,
        self._node_fs_creator.GetMaster(),
        test_object_store,
        'extensions',
        SchemaProcessorFactoryForTest())

    server_instance = ServerInstance.ForTest(
        file_system_provider=FakeHostFileSystemProvider(
            TABS_SCHEMA_BRANCHES))
    self._api_models = server_instance.platform_bundle.GetAPIModels(
        'extensions')
    self._json_cache = server_instance.compiled_fs_factory.ForJson(
        server_instance.host_file_system_provider.GetMaster())

    # Imitate the actual SVN file system by incrementing the stats for paths
    # where an API schema has changed.
    last_stat = type('last_stat', (object,), {'val': 0})

    def stat_paths(file_system, channel_info):
      if channel_info.version not in tabs_unmodified_versions:
        last_stat.val += 1
      # HACK: |file_system| is a MockFileSystem backed by a TestFileSystem.
      # Increment the TestFileSystem stat count.
      file_system._file_system.IncrementStat(by=last_stat.val)
      # Continue looping. The iterator will stop after 'master' automatically.
      return True

    # Use the HostFileSystemIterator created above to change global stat values
    # for the TestFileSystems that it creates.
    self._node_fs_iterator.Ascending(
        # The earliest version represented with the tabs' test data is 13.
        self._branch_utility.GetStableChannelInfo(13),
        stat_paths)

  def testGetAPINodeAvailability(self):
    def assertEquals(node, actual):
      node_availabilities = {
          'tabs.Tab': None,
          'tabs.fakeTabsProperty1': None,
          'tabs.get': None,
          'tabs.onUpdated': None,
          'tabs.InjectDetails': 25,
          'tabs.fakeTabsProperty2': 15,
          'tabs.getCurrent': 19,
          'tabs.onActivated': 30
      }
      self.assertEquals(node_availabilities[node], actual)

    model_dict = CreateJSCView(
        self._api_models.GetContentScriptAPIs().Get(),
        self._api_models.GetModel('tabs').Get(),
        self._avail_finder,
        self._json_cache,
        _FakeTemplateCache(),
        _FakeFeaturesBundle(),
        None,
        'extensions',
        [],
        Request.ForTest(''))

    # Test nodes that have the same availability as their parent.

    # Test type.
    assertEquals('tabs.Tab', model_dict['types'][0]['availability'])
    # Test property.
    assertEquals('tabs.fakeTabsProperty1',
                 model_dict['properties'][1]['availability'])
    # Test function.
    assertEquals('tabs.get', model_dict['functions'][1]['availability'])
    # Test event.
    assertEquals('tabs.onUpdated', model_dict['events'][1]['availability'])

    # Test nodes with varying availabilities.

    # Test type.
    assertEquals('tabs.InjectDetails',
                 model_dict['types'][1]['availability']['version'])
    # Test property.
    assertEquals('tabs.fakeTabsProperty2',
                 model_dict['properties'][3]['availability']['version'])
    # Test function.
    assertEquals('tabs.getCurrent',
                 model_dict['functions'][0]['availability']['version'])
    # Test event.
    assertEquals('tabs.onActivated',
                 model_dict['events'][0]['availability']['version'])

    # Test a node that became deprecated.
    self.assertEquals({
      'scheduled': None,
      'version': 26,
      'partial': 'motemplate chrome/common/extensions/docs/templates/' +
          'private/intro_tables/deprecated_message.html'
      }, model_dict['types'][2]['availability'])

if __name__ == '__main__':
  unittest.main()
