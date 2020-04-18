#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import sys
import unittest

import api_schema_graph
from availability_finder import AvailabilityFinder, AvailabilityInfo
from branch_utility import BranchUtility, ChannelInfo
from compiled_file_system import CompiledFileSystem
from fake_host_file_system_provider import FakeHostFileSystemProvider
from fake_url_fetcher import FakeUrlFetcher
from host_file_system_iterator import HostFileSystemIterator
from mock_function import MockFunction
from object_store_creator import ObjectStoreCreator
from platform_util import GetPlatforms
from test_data.canned_data import (CANNED_API_FILE_SYSTEM_DATA, CANNED_BRANCHES)
from test_data.object_level_availability.tabs import TABS_SCHEMA_BRANCHES
from test_util import Server2Path
from schema_processor import SchemaProcessorFactoryForTest


TABS_UNMODIFIED_VERSIONS = (16, 20, 23, 24)

class AvailabilityFinderTest(unittest.TestCase):

  def _create_availability_finder(self,
                                  host_fs_creator,
                                  host_fs_iterator,
                                  platform):
    test_object_store = ObjectStoreCreator.ForTest()
    return AvailabilityFinder(
        self._branch_utility,
        CompiledFileSystem.Factory(test_object_store),
        host_fs_iterator,
        host_fs_creator.GetMaster(),
        test_object_store,
        platform,
        SchemaProcessorFactoryForTest())

  def setUp(self):
    self._branch_utility = BranchUtility(
        os.path.join('branch_utility', 'first.json'),
        os.path.join('branch_utility', 'second.json'),
        FakeUrlFetcher(Server2Path('test_data')),
        ObjectStoreCreator.ForTest())
    self._api_fs_creator = FakeHostFileSystemProvider(
        CANNED_API_FILE_SYSTEM_DATA)
    self._node_fs_creator = FakeHostFileSystemProvider(TABS_SCHEMA_BRANCHES)
    self._api_fs_iterator = HostFileSystemIterator(self._api_fs_creator,
                                                   self._branch_utility)
    self._node_fs_iterator = HostFileSystemIterator(self._node_fs_creator,
                                                    self._branch_utility)

    # Imitate the actual SVN file system by incrementing the stats for paths
    # where an API schema has changed.
    last_stat = type('last_stat', (object,), {'val': 0})

    def stat_paths(file_system, channel_info):
      if channel_info.version not in TABS_UNMODIFIED_VERSIONS:
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

  def testGraphOptimization(self):
    for platform in GetPlatforms():
      # Keep track of how many times the APISchemaGraph constructor is called.
      original_constructor = api_schema_graph.APISchemaGraph
      mock_constructor = MockFunction(original_constructor)
      api_schema_graph.APISchemaGraph = mock_constructor

      node_avail_finder = self._create_availability_finder(
          self._node_fs_creator, self._node_fs_iterator, platform)
      try:
        # The test data includes an extra branch where the API does not exist.
        num_versions = len(TABS_SCHEMA_BRANCHES) - 1
        # We expect an APISchemaGraph to be created only when an API schema file
        # has different stat data from the previous version's schema file.
        num_graphs_created = num_versions - len(TABS_UNMODIFIED_VERSIONS)

        # Run the logic for object-level availability for an API.
        node_avail_finder.GetAPINodeAvailability('tabs')

        self.assertTrue(*api_schema_graph.APISchemaGraph.CheckAndReset(
            num_graphs_created))
      finally:
        # Ensure that the APISchemaGraph constructor is reset to be the original
        # constructor.
        api_schema_graph.APISchemaGraph = original_constructor

  def testGetAPIAvailability(self):
    # Key: Using 'channel' (i.e. 'beta') to represent an availability listing
    # for an API in a _features.json file, and using |channel| (i.e. |dev|) to
    # represent the development channel, or phase of development, where an API's
    # availability is being checked.

    def assertGet(ch_info, api, only_on=None, scheduled=None):
      for platform in GetPlatforms():
        get_availability = self._create_availability_finder(
            self._api_fs_creator,
            self._api_fs_iterator,
            platform if only_on is None else only_on).GetAPIAvailability
        self.assertEqual(AvailabilityInfo(ch_info, scheduled=scheduled),
                         get_availability(api))

    # Testing APIs with predetermined availability.
    assertGet(ChannelInfo('master', 'master', 'master'), 'jsonMasterAPI')
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'jsonDevAPI')
    assertGet(ChannelInfo('beta', CANNED_BRANCHES[30], 30), 'jsonBetaAPI')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[20], 20), 'jsonStableAPI')

    # Testing a whitelisted API.
    assertGet(ChannelInfo('beta', CANNED_BRANCHES[30], 30),
              'declarativeWebRequest')

    # Testing APIs found only by checking file system existence.
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[23], 23), 'windows')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[18], 18), 'tabs')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[18], 18), 'input.ime')

    # Testing API channel existence for _api_features.json.
    # Listed as 'dev' on |beta|, 'dev' on |dev|.
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'systemInfo.stuff')
    # Listed as 'stable' on |beta|.
    assertGet(ChannelInfo('beta', CANNED_BRANCHES[30], 30),
              'systemInfo.cpu',
              scheduled=31)

    # Testing API channel existence for _manifest_features.json.
    # Listed as 'master' on all channels.
    assertGet(ChannelInfo('master', 'master', 'master'), 'sync')
    # No records of API until |master|.
    assertGet(ChannelInfo('master', 'master', 'master'), 'history')
    # Listed as 'dev' on |dev|.
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'storage')
    # Stable in _manifest_features and into pre-18 versions.
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[8], 8), 'pageAction')

    # Testing API channel existence for _permission_features.json.
    # Listed as 'beta' on |master|.
    assertGet(ChannelInfo('master', 'master', 'master'), 'falseBetaAPI')
    # Listed as 'master' on |master|.
    assertGet(ChannelInfo('master', 'master', 'master'), 'masterAPI')
    # Listed as 'master' on all development channels.
    assertGet(ChannelInfo('master', 'master', 'master'), 'declarativeContent')
    # Listed as 'dev' on all development channels.
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'bluetooth')
    # Listed as 'dev' on |dev|.
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'cookies')
    # Treated as 'stable' APIs.
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[24], 24), 'alarms')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[21], 21), 'bookmarks')

    # Testing older API existence using extension_api.json.
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[6], 6), 'menus')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[5], 5), 'idle')

    # Switches between _features.json files across branches.
    # Listed as 'master' on all channels, in _api, _permission, or _manifest.
    assertGet(ChannelInfo('master', 'master', 'master'), 'contextMenus')
    # Moves between _permission and _manifest as file system is traversed.
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[23], 23),
              'systemInfo.display')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[17], 17), 'webRequest')

    # Mid-upgrade cases:
    # Listed as 'dev' on |beta| and 'beta' on |dev|.
    assertGet(ChannelInfo('dev', CANNED_BRANCHES[31], 31), 'notifications')
    # Listed as 'beta' on |stable|, 'dev' on |beta|...until |stable| on master.
    assertGet(ChannelInfo('master', 'master', 'master'), 'events')

    # Check for differing availability across apps|extensions
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[26], 26),
              'appsFirst',
              only_on='extensions')
    assertGet(ChannelInfo('stable', CANNED_BRANCHES[25], 25),
              'appsFirst',
              only_on='apps')

  def testGetAPINodeAvailability(self):
    def assertEquals(found, channel_info, actual, scheduled=None):
      lookup_result = api_schema_graph.LookupResult
      if channel_info is None:
        self.assertEquals(lookup_result(found, None), actual)
      else:
        self.assertEquals(lookup_result(found, AvailabilityInfo(channel_info,
            scheduled=scheduled)), actual)

    for platform in GetPlatforms():
      # Allow the LookupResult constructions below to take just one line.
      avail_finder = self._create_availability_finder(
          self._node_fs_creator,
          self._node_fs_iterator,
          platform)
      tabs_graph = avail_finder.GetAPINodeAvailability('tabs')
      fake_tabs_graph = avail_finder.GetAPINodeAvailability('fakeTabs')

      # Test an API node with predetermined availability.
      assertEquals(True, self._branch_utility.GetStableChannelInfo(27),
          tabs_graph.Lookup('tabs', 'properties', 'fakeTabsProperty4'))

      assertEquals(True, self._branch_utility.GetChannelInfo('master'),
          tabs_graph.Lookup('tabs', 'properties', 'fakeTabsProperty3'))
      assertEquals(True, self._branch_utility.GetChannelInfo('dev'),
          tabs_graph.Lookup('tabs', 'events', 'onActivated', 'parameters',
              'activeInfo', 'properties', 'windowId'), scheduled=31)
      assertEquals(True, self._branch_utility.GetChannelInfo('dev'),
          tabs_graph.Lookup('tabs', 'events', 'onUpdated', 'parameters', 'tab'),
          scheduled=31)
      assertEquals(True, self._branch_utility.GetChannelInfo('beta'),
          tabs_graph.Lookup('tabs', 'events', 'onActivated'), scheduled=30)
      assertEquals(True, self._branch_utility.GetChannelInfo('beta'),
          tabs_graph.Lookup('tabs', 'functions', 'get', 'parameters', 'tabId'),
          scheduled=30)
      assertEquals(True, self._branch_utility.GetChannelInfo('stable'),
          tabs_graph.Lookup('tabs', 'types', 'InjectDetails', 'properties',
              'code'))
      assertEquals(True, self._branch_utility.GetChannelInfo('stable'),
          tabs_graph.Lookup('tabs', 'types', 'InjectDetails', 'properties',
              'file'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(25),
          tabs_graph.Lookup('tabs', 'types', 'InjectDetails'))

      # Test inlined type.
      assertEquals(True, self._branch_utility.GetChannelInfo('master'),
          tabs_graph.Lookup('tabs', 'types', 'InlinedType'))

      # Test implicitly inlined type.
      assertEquals(True, self._branch_utility.GetStableChannelInfo(25),
          fake_tabs_graph.Lookup('fakeTabs', 'types',
              'WasImplicitlyInlinedType'))

      # Test a node that was restricted to dev channel when it was introduced.
      assertEquals(True, self._branch_utility.GetChannelInfo('beta'),
          tabs_graph.Lookup('tabs', 'functions', 'restrictedFunc'),
          scheduled=30)

      # Test an explicitly scheduled node.
      assertEquals(True, self._branch_utility.GetChannelInfo('dev'),
          tabs_graph.Lookup('tabs', 'functions', 'scheduledFunc'),
          scheduled=31)

      # Nothing new in version 24 or 23.

      assertEquals(True, self._branch_utility.GetStableChannelInfo(22),
          tabs_graph.Lookup('tabs', 'types', 'Tab', 'properties', 'windowId'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(21),
          tabs_graph.Lookup('tabs', 'types', 'Tab', 'properties', 'selected'))

      # Nothing new in version 20.

      assertEquals(True, self._branch_utility.GetStableChannelInfo(19),
          tabs_graph.Lookup('tabs', 'functions', 'getCurrent'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(18),
          tabs_graph.Lookup('tabs', 'types', 'Tab', 'properties', 'index'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(17),
          tabs_graph.Lookup('tabs', 'events', 'onUpdated', 'parameters',
              'changeInfo'))

      # Nothing new in version 16.

      assertEquals(True, self._branch_utility.GetStableChannelInfo(15),
          tabs_graph.Lookup('tabs', 'properties', 'fakeTabsProperty2'))

      # Everything else is available at the API's release, version 14 here.
      assertEquals(True, self._branch_utility.GetStableChannelInfo(14),
          tabs_graph.Lookup('tabs', 'types', 'Tab'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(14),
          tabs_graph.Lookup('tabs', 'types', 'Tab', 'properties', 'url'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(14),
          tabs_graph.Lookup('tabs', 'properties', 'fakeTabsProperty1'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(14),
          tabs_graph.Lookup('tabs', 'functions', 'get', 'parameters',
              'callback'))
      assertEquals(True, self._branch_utility.GetStableChannelInfo(14),
          tabs_graph.Lookup('tabs', 'events', 'onUpdated'))

      # Test things that aren't available.
      assertEquals(False, None, tabs_graph.Lookup('tabs', 'types',
          'UpdateInfo'))
      assertEquals(False, None, tabs_graph.Lookup('tabs', 'functions', 'get',
          'parameters', 'callback', 'parameters', 'tab', 'id'))
      assertEquals(False, None, tabs_graph.Lookup('functions'))
      assertEquals(False, None, tabs_graph.Lookup('events', 'onActivated',
          'parameters', 'activeInfo', 'tabId'))


if __name__ == '__main__':
  unittest.main()
