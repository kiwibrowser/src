#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from api_data_source import APIDataSource
from extensions_paths import CHROME_EXTENSIONS
from servlet import Request
from server_instance import ServerInstance
from test_data.api_data_source.canned_master_fs import CANNED_MASTER_FS_DATA
from test_file_system import TestFileSystem


class APIDataSourceTest(unittest.TestCase):
  def setUp(self):
    self.server_instance = ServerInstance.ForTest(
        TestFileSystem(CANNED_MASTER_FS_DATA, relative_to=CHROME_EXTENSIONS))

  def testGet(self):
    api_ds = APIDataSource(self.server_instance, Request.ForTest('/'))
    jsc_view = api_ds.get('extensions').get('tester')
    funcs_arr = [{
      'availability': None,
      'callback': {
        'name': 'callback',
        'optional': False,
        'parameters': [{
          'array': {
            'availability': None,
            'description': None,
            'events': [],
            'functions': [],
            'id': 'type-results-resultsType',
            'is_object': False,
            'link': {
              'name': 'TypeA',
              'ref': 'tester.TypeA',
              'text': 'TypeA'
            },
            'name': 'resultsType',
            'properties': []
          },
          'availability': None,
          'description': None,
          'functions': [],
          'id': 'property-callback-results',
          'is_object': False,
          'last': True,
          'name': 'results',
          'optional': None,
          'parameters': [],
          'parentName': 'callback',
          'properties': [],
          'returns': None
        }],
        'simple_type': {
          'simple_type': 'function'
        }
      },
      'description': 'Gets stuff.',
      'id': 'method-get',
      'name': 'get',
      'parameters': [{
        'availability': None,
        'choices': [{
          'availability': None,
          'description': None,
          'events': [],
          'functions': [],
          'id': 'type-a-string',
          'is_object': False,
          'name': 'string',
          'properties': [],
          'simple_type': 'string'
        },
        {
          'array': {
            'availability': None,
            'description': None,
            'events': [],
            'functions': [],
            'id': 'type-strings-stringsType',
            'is_object': False,
            'name': 'stringsType',
            'properties': [],
            'simple_type': 'string'
          },
          'availability': None,
          'description': None,
          'events': [],
          'functions': [],
          'id': 'type-a-strings',
          'is_object': False,
          'last': True,
          'name': 'strings',
          'properties': []
        }],
        'description': 'a param',
        'functions': [],
        'id': 'property-get-a',
        'is_object': False,
        'name': 'a',
        'optional': None,
        'parameters': [],
        'parentName': 'get',
        'properties': [],
        'returns': None
      },
      {
        'asFunction': {
          'name': 'callback',
          'optional': False,
          'parameters': [{
            'array': {
              'availability': None,
              'description': None,
              'events': [],
              'functions': [],
              'id': 'type-results-resultsType',
              'is_object': False,
              'link': {
                'name': 'TypeA',
                'ref': 'tester.TypeA',
                'text': 'TypeA'
              },
              'name': 'resultsType',
              'properties': []
            },
            'availability': None,
            'description': None,
            'functions': [],
            'id': 'property-callback-results',
            'is_object': False,
            'last': True,
            'name': 'results',
            'optional': None,
            'parameters': [],
            'parentName': 'callback',
            'properties': [],
            'returns': None
          }],
          'simple_type': {
            'simple_type': 'function'
          }
        },
        'description': None,
        'id': 'property-get-callback',
        'isCallback': True,
        'last': True,
        'name': 'callback',
        'optional': False,
        'parentName': 'get',
        'simple_type': 'function'
      }],
      'returns': None
    }]
    self.assertEquals(funcs_arr, jsc_view['functions'])
    types_arr = [{
      'availability': None,
      'description': 'A cool thing.',
      'events': [],
      'functions': [],
      'id': 'type-TypeA',
      'is_object': True,
      'name': 'TypeA',
      'properties': [{
        'array': {
          'availability': None,
          'description': None,
          'events': [],
          'functions': [],
          'id': 'type-b-bType',
          'is_object': False,
          'link': {
            'name': 'TypeA',
            'ref': 'tester.TypeA',
            'text': 'TypeA'
          },
          'name': 'bType',
          'properties': []
        },
        'availability': None,
        'description': 'List of TypeA.',
        'functions': [],
        'id': 'property-TypeA-b',
        'is_object': False,
        'name': 'b',
        'optional': True,
        'parameters': [],
        'parentName': 'TypeA',
        'properties': [],
        'returns': None
      }],
      'simple_type': 'object'
    }]
    self.assertEquals(types_arr, jsc_view['types'])

if __name__ == '__main__':
  unittest.main()
