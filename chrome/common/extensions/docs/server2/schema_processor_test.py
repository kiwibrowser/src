#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
from copy import deepcopy

from schema_processor import SchemaProcessor
from future import Future
from object_store_creator import ObjectStoreCreator
from host_file_system_provider import HostFileSystemProvider
from compiled_file_system import CompiledFileSystem

class _FakeReferenceResolver():
  def GetRefModel(self, ref, api_list):
    return None, None

class _FakeAPIModels():
  def GetNames(self):
    return []

class _FakeFeaturesBundle():
  def GetAPIFeatures(self):
    return Future(value={})

class SchemaUtilTest(unittest.TestCase):
  def testRemoveNoDocs(self):
    expected_nodoc = [
      {
        'name': 'B',
        'list': [
          {
            'name': 'B2'
          }
        ]
      },
      {
        'name': 'D',
        'nodoc': False
      },
      {
        'name': 'E',
        'items1': [
          {
            'name': 'E1',
            'items': [
              {
                'name': 'E1.3'
              }
            ]
          },
          {
            'name': 'E2'
          }
        ]
      }
    ]

    nodoc_data = [
      {
        'name': 'A',
        'nodoc': True
      },
      {
        'name': 'B',
        'list': [
          {
            'name': 'B1',
            'nodoc': True
          },
          {
            'name': 'B2'
          },
          {
            'name': 'B3',
            'nodoc': True
          }
        ]
      },
      {
        'name': 'C',
        'nodoc': True
      },
      {
        'name': 'D',
        'nodoc': False
      },
      {
        'name': 'E',
        'dict': {
          'name': 'Ed',
          'nodoc': True
        },
        'items1': [
          {
            'name': 'E1',
            'items': [
              {
                'name': 'E1.1',
                'nodoc': True
              },
              {
                'name': 'E1.2',
                'nodoc': True
              },
              {
                'name': 'E1.3'
              }
            ]
          },
          {
            'name': 'E2'
          },
          {
            'name': 'E3',
            'nodoc': True
          }
        ]
      }
    ]

    object_store_creator = ObjectStoreCreator(start_empty=False)
    host_file_system_provider = HostFileSystemProvider(object_store_creator)
    schema_processor = SchemaProcessor(_FakeReferenceResolver(),
                                       _FakeAPIModels(),
                                       _FakeFeaturesBundle(),
                                       CompiledFileSystem.Factory(
                                           object_store_creator),
                                       host_file_system_provider.GetMaster(),
                                       True)
    schema_processor._RemoveNoDocs(nodoc_data)
    self.assertEquals(expected_nodoc, nodoc_data)

  def testInlineDocs(self):
    schema = {
      'namespace': 'storage',
      'properties': {
        'key2': {
          'description': 'second key',
          '$ref': 'Key'
        },
        'key1': {
          'description': 'first key',
          '$ref': 'Key'
        }
      },
      'types': [
        {
          'inline_doc': True,
          'type': 'string',
          'id': 'Key',  # Should be inlined into both properties and be removed
                        # from types.
          'description': 'This is a key.',  # This description should disappear.
          'marker': True  # This should appear three times in the output.
        },
        {
          'items': {
            '$ref': 'Key'
          },
          'type': 'array',
          'id': 'KeyList',
          'description': 'A list of keys'
        }
      ]
    }

    expected_schema = {
      'namespace': 'storage',
      'properties': {
        'key2': {
          'marker': True,
          'type': 'string',
          'description': 'second key'
        },
        'key1': {
          'marker': True,
          'type': 'string',
          'description': 'first key'
        }
      },
      'types': [
        {
          'items': {
            'marker': True,
            'type': 'string'
          },
          'type': 'array',
          'id': 'KeyList',
          'description': 'A list of keys'
        }
      ]
    }

    object_store_creator = ObjectStoreCreator(start_empty=False)
    host_file_system_provider = HostFileSystemProvider(object_store_creator)
    schema_processor = SchemaProcessor(_FakeReferenceResolver(),
                                       _FakeAPIModels(),
                                       _FakeFeaturesBundle(),
                                       CompiledFileSystem.Factory(
                                           object_store_creator),
                                       host_file_system_provider.GetMaster(),
                                       False)
    inlined_schema = deepcopy(schema)
    schema_processor._InlineDocs(inlined_schema)
    self.assertEqual(expected_schema, inlined_schema)

  def testDetectInline(self):
    schema = {
      'types': [
        {
          'id': 'Key',
          'items': {
            '$ref': 'Value'
          }
        },
        {
          'id': 'Value',
          'marker': True
        }
      ]
    }

    expected_schema = {
      'types': [
        {
          'id': 'Key',
          'items': {
            'marker': True,
          }
        }
      ]
    }

    object_store_creator = ObjectStoreCreator(start_empty=False)
    host_file_system_provider = HostFileSystemProvider(object_store_creator)
    schema_processor = SchemaProcessor(_FakeReferenceResolver(),
                                       _FakeAPIModels(),
                                       _FakeFeaturesBundle(),
                                       CompiledFileSystem.Factory(
                                           object_store_creator),
                                       host_file_system_provider.GetMaster(),
                                       False)
    schema_processor._DetectInlineableTypes(schema)
    schema_processor._InlineDocs(schema)
    self.assertEqual(expected_schema, schema)


if __name__ == '__main__':
  unittest.main()
