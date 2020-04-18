#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest
import copy

from extensions_paths import JSON_TEMPLATES
from mock_file_system import MockFileSystem
from server_instance import ServerInstance
from servlet import Request
from sidenav_data_source import SidenavDataSource, _AddLevels, _AddAnnotations
from test_file_system import TestFileSystem
from test_util import CaptureLogging


class SamplesDataSourceTest(unittest.TestCase):
  def testAddLevels(self):
    sidenav_json = [{
      'title': 'H2',
      'items': [{
        'title': 'H3',
        'items': [{ 'title': 'X1' }]
      }]
    }]

    expected = [{
      'level': 1,
      'title': 'H2',
      'items': [{
        'level': 2,
        'title': 'H3',
        'items': [{ 'level': 3, 'title': 'X1' }]
      }]
    }]

    _AddLevels(sidenav_json, 1)
    self.assertEqual(expected, sidenav_json)

  def testAddAnnotations(self):
    item1 = { 'href': '/H1.html' }
    item2_1 = { 'href': '/H2_1.html' }
    item2_2 = { 'href': '/H2_2.html' }
    item2 = { 'href': '/H2.html', 'items': [item2_1, item2_2] }

    expected = [ item1, item2 ]

    sidenav_json = copy.deepcopy(expected)

    item2['child_selected'] = True
    item2_1['selected'] = True
    item2_1['related'] = True
    item2_1['parent'] = { 'title': item2.get('title', None),
                          'href': item2.get('href', None) }

    item2_2['related'] = True

    self.assertTrue(_AddAnnotations(sidenav_json, item2_1['href']))
    self.assertEqual(expected, sidenav_json)

  def testWithDifferentBasePath(self):
    file_system = TestFileSystem({
      'chrome_sidenav.json': json.dumps([
        { 'href': '/H1.html' },
        { 'href': '/H2.html' },
        { 'href': '/base/path/H2.html' },
        { 'href': 'https://qualified/X1.html' },
        {
          'href': 'H3.html',
          'items': [{
            'href': 'H4.html'
          }]
        },
      ])
    }, relative_to=JSON_TEMPLATES)

    expected = [
      {'href': '/base/path/H1.html', 'level': 2, 'related': True},
      {'href': '/base/path/H2.html', 'level': 2, 'selected': True, 'related': True},
      {'href': '/base/path/base/path/H2.html', 'level': 2, 'related': True},
      {'href': 'https://qualified/X1.html', 'level': 2, 'related': True},
      {'items': [
        {'href': '/base/path/H4.html', 'level': 3}
      ],
      'href': '/base/path/H3.html', 'level': 2, 'related': True}
    ]

    server_instance = ServerInstance.ForTest(file_system,
                                             base_path='/base/path/')
    sidenav_data_source = SidenavDataSource(server_instance,
                                            Request.ForTest('/H2.html'))

    log_output = CaptureLogging(
        lambda: self.assertEqual(expected, sidenav_data_source.get('chrome')))
    self.assertEqual(2, len(log_output))

  def testSidenavDataSource(self):
    file_system = MockFileSystem(TestFileSystem({
      'chrome_sidenav.json': json.dumps([{
        'title': 'H1',
        'href': 'H1.html',
        'items': [{
          'title': 'H2',
          'href': '/H2.html'
        }]
      }])
    }, relative_to=JSON_TEMPLATES))

    expected = [{
      'level': 2,
      'child_selected': True,
      'title': 'H1',
      'href': '/H1.html',
      'items': [{
        'level': 3,
        'selected': True,
        'related': True,
        'title': 'H2',
        'href': '/H2.html',
        'parent': { 'href': '/H1.html', 'title': 'H1'}
      }]
    }]

    sidenav_data_source = SidenavDataSource(
        ServerInstance.ForTest(file_system), Request.ForTest('/H2.html'))
    self.assertTrue(*file_system.CheckAndReset())

    log_output = CaptureLogging(
        lambda: self.assertEqual(expected, sidenav_data_source.get('chrome')))

    self.assertEqual(1, len(log_output))
    self.assertTrue(
        log_output[0].msg.startswith('Paths in sidenav must be qualified.'))

    # Test that only a single file is read when creating the sidenav, so that
    # we can be confident in the compiled_file_system.SingleFile annotation.
    self.assertTrue(*file_system.CheckAndReset(
        read_count=1, stat_count=1, read_resolve_count=1))

  def testRefresh(self):
    file_system = TestFileSystem({
      'chrome_sidenav.json': '[{ "title": "H1" }]'
    }, relative_to=JSON_TEMPLATES)

    # Ensure Refresh doesn't rely on request.
    sidenav_data_source = SidenavDataSource(
        ServerInstance.ForTest(file_system), request=None)
    sidenav_data_source.Refresh().Get()

    # If Refresh fails, chrome_sidenav.json will not be cached, and the
    # cache_data access will fail.
    # TODO(jshumway): Make a non hack version of this check.
    sidenav_data_source._cache._file_object_store.Get(
        '%schrome_sidenav.json' % JSON_TEMPLATES).Get().cache_data


if __name__ == '__main__':
  unittest.main()
