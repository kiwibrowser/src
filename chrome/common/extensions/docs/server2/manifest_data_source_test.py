#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
import json
import unittest

from future import Future
import manifest_data_source
from object_store_creator import ObjectStoreCreator


convert_and_annotate_docs = {
  'name': {
    'example': "My {{platform}}",
    'name': 'name',
    'level': 'required'
  },
  'doc2': {
    'level': 'required',
    'name': 'doc2'
  },
  'doc1': {
    'level': 'required',
    'name': 'doc1',
    'children': {
      'sub1': {
        'annotations': ['not so important'],
        'level': 'optional',
        'name': 'sub1'
      },
      'sub2': {
        'level': 'required',
        'name': 'sub2'
      }
    }
  },
  'doc3': {
    'level':  'only_one',
    'name': 'doc3'
  },
  'doc4': {
    'level': 'recommended',
    'name': 'doc4'
  },
  'doc5': {
    'level': 'only_one',
    'name': 'doc5'
  },
  'doc6': {
    'level': 'optional',
    'name': 'doc6'
  }
}


class ManifestDataSourceTest(unittest.TestCase):
  def testListifyAndSortDocs(self):
    expected_docs = [
      {
        'level': 'required',
        'name': 'doc1',
        'children': [
          {
            'level': 'required',
            'name': 'sub2'
          },
          {
            'annotations': ['not so important'],
            'level': 'optional',
            'name': 'sub1'
          }
        ]
      },
      {
        'level': 'required',
        'name': 'doc2'
      },
      {
        'level': 'required',
        'example': '"My App"',
        'has_example': True,
        'name': 'name'
      },
      {
        'level': 'recommended',
        'name': 'doc4'
      },
      {
        'level': 'only_one',
        'name': 'doc3'
      },
      {
        'level': 'only_one',
        'name': 'doc5'
      },
      {
        'level': 'optional',
        'name': 'doc6'
      }
    ]

    self.assertEqual(expected_docs, manifest_data_source._ListifyAndSortDocs(
        deepcopy(convert_and_annotate_docs), 'App'))

  def testAnnotate(self):
    expected_docs = [
      {
        'level': 'required',
        'name': 'doc1',
        'children': [
          {
            'level': 'required',
            'name': 'sub2'
          },
          {
            'annotations': ['Optional', 'not so important'],
            'level': 'optional',
            'name': 'sub1',
            'is_last': True
          }
        ]
      },
      {
        'level': 'required',
        'name': 'doc2'
      },
      {
        'name': 'name',
        'level': 'required',
        'example': '"My App"',
        'has_example': True
      },
      {
        'annotations': ['Recommended'],
        'level': 'recommended',
        'name': 'doc4'
      },
      {
        'annotations': ['Pick one (or none)'],
        'level': 'only_one',
        'name': 'doc3'
      },
      {
        'level': 'only_one',
        'name': 'doc5'
      },
      {
        'annotations': ['Optional'],
        'level': 'optional',
        'name': 'doc6',
        'is_last': True
      }
    ]

    annotated = manifest_data_source._ListifyAndSortDocs(
        deepcopy(convert_and_annotate_docs), 'App')
    manifest_data_source._AddLevelAnnotations(annotated)
    self.assertEqual(expected_docs, annotated)

  def testExpandedExamples(self):
    docs = {
      'doc1': {
        'name': 'doc1',
        'example': {
          'big': {
            'nested': {
              'json_example': ['with', 'more', 'json']
            }
          }
        }
      }
    }

    expected_docs = [
      {
        'name': 'doc1',
        'children': [
          {
            'name': 'big',
            'children': [
              {
                'name': 'nested',
                'children': [
                  {
                    'name': 'json_example',
                    'example': json.dumps(['with', 'more', 'json']),
                    'has_example': True
                  }
                ]
              }
            ]
          }
        ]
      }
    ]

    self.assertEqual(
        expected_docs, manifest_data_source._ListifyAndSortDocs(docs, 'apps'))

  def testNonExpandedExamples(self):
    docs = {
      'doc1': {
        'name': 'doc1',
        'example': {}
      },
      'doc2': {
        'name': 'doc2',
        'example': []
      },
      'doc3': {
        'name': 'doc3',
        'example': [{}]
      }
    }

    expected_docs = [
      {
        'name': 'doc1',
        'has_example': True,
        'example': '{...}'
      },
      {
        'name': 'doc2',
        'has_example': True,
        'example': '[...]'
      },
      {
        'name': 'doc3',
        'has_example': True,
        'example': '[{...}]'
      }
    ]
    self.assertEqual(
        expected_docs, manifest_data_source._ListifyAndSortDocs(docs, 'apps'))

  def testManifestDataSource(self):
    manifest_features = {
      'doc1': {
        'name': 'doc1',
        'platforms': ['apps', 'extensions'],
        'example': {},
        'level': 'required'
      },
      'doc1.sub1': {
        'name': 'doc1.sub1',
        'platforms': ['apps'],
        'annotations': ['important!'],
        'level': 'recommended'
      }
    }

    expected_app = [
      {
        'example': '{...}',
        'has_example': True,
        'level': 'required',
        'name': 'doc1',
        'platforms': ['apps', 'extensions'],
        'children': [
          {
            'annotations': [
              'Recommended',
              'important!'
            ],
            'level': 'recommended',
            'name': 'sub1',
            'platforms': ['apps'],
            'is_last': True
          }
        ],
        'is_last': True
      }
    ]

    class FakePlatformBundle(object):
      def GetFeaturesBundle(self, platform):
        return FakeFeaturesBundle()

    class FakeFeaturesBundle(object):
      def GetManifestFeatures(self):
        return Future(value=manifest_features)

    class FakeServerInstance(object):
      def __init__(self):
        self.platform_bundle = FakePlatformBundle()
        self.object_store_creator = ObjectStoreCreator.ForTest()

    mds = manifest_data_source.ManifestDataSource(FakeServerInstance(), None)
    self.assertEqual(expected_app, mds.get('apps'))

if __name__ == '__main__':
  unittest.main()
