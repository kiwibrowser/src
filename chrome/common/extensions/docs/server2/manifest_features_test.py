#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from manifest_features import ConvertDottedKeysToNested

class ManifestFeaturesTest(unittest.TestCase):
  def testConvertDottedKeysToNested(self):
    docs = {
      'doc1.sub2': {
        'name': 'doc1.sub2'
      },
      'doc1': {
        'name': 'doc1'
      },
      'doc2': {
        'name': 'doc2'
      },
      'doc1.sub1.subsub1': {
        'name': 'doc1.sub1.subsub1'
      },
      'doc1.sub1': {
        'name': 'doc1.sub1'
      }
    }

    expected_docs = {
      'doc1': {
        'name': 'doc1',
        'children': {
          'sub1': {
            'name': 'sub1',
            'children': {
              'subsub1': {
                'name' :'subsub1'
              }
            }
          },
          'sub2': {
            'name': 'sub2'
          }
        }
      },
      'doc2': {
        'name': 'doc2'
      }
    }

    self.assertEqual(expected_docs, ConvertDottedKeysToNested(docs))

if __name__ == '__main__':
  unittest.main()
