#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from owners_data_source import ParseOwnersFile, OwnersDataSource
from server_instance import ServerInstance
from servlet import Request
from test_file_system import TestFileSystem


_TEST_FS = {
  'chrome': {
    'browser': {
      'extensions': {
        'OWNERS': '\n'.join([
          '# Core owners.',
          'satsuki@revocs.tld'
        ]),
        'api': {
          'some_api': {
            'OWNERS': '\n'.join([
              'matoi@owner.tld'
            ]),
            'some_api.cc': ''
          },
          'another_api': {
            'another_api.cc': '',
            'another_api.h': ''
          },
          'moar_apis': {
            'OWNERS': '\n'.join([
              '# For editing moar_apis.',
              'satsuki@revocs.tld'
            ])
          }
        }
      }
    }
  },
  'extensions': {
    'browser': {
      'api': {
        'a_different_api': {
          'OWNERS': '\n'.join([
            '# Hallo!',
            'nonon@owner.tld',
            'matoi@owner.tld'
          ])
        }
      }
    }
  }
}


class OwnersDataSourceTest(unittest.TestCase):
  def setUp(self):
    server_instance = ServerInstance.ForTest(
        file_system=TestFileSystem(_TEST_FS))
    # Don't randomize the owners to avoid testing issues.
    self._owners_ds = OwnersDataSource(server_instance,
                                       Request.ForTest('/'),
                                       randomize=False)

  def testParseOwnersFile(self):
    owners_content = '\n'.join([
      'satsuki@revocs.tld',
      'mankanshoku@owner.tld',
      '',
      'matoi@owner.tld'
    ])
    owners, notes = ParseOwnersFile(owners_content, randomize=False)
    # The order of the owners list should reflect the order of the owners file.
    self.assertEqual(owners, [
      {
        'email': 'satsuki@revocs.tld',
        'username': 'satsuki'
      },
      {
        'email': 'mankanshoku@owner.tld',
        'username': 'mankanshoku'
      },
      {
        'email': 'matoi@owner.tld',
        'username': 'matoi',
        'last': True
      }
    ])
    self.assertEqual(notes, '')

    owners_content_with_comments = '\n'.join([
      '# This is a comment concerning this file',
      '# that should not be ignored.',
      'matoi@owner.tld',
      'mankanshoku@owner.tld',
      '',
      '# Only bug satsuki if matoi or mankanshoku are unavailable.',
      'satsuki@revocs.tld'
    ])
    owners, notes = ParseOwnersFile(owners_content_with_comments,
                                    randomize=False)
    self.assertEqual(owners, [
      {
        'email': 'matoi@owner.tld',
        'username': 'matoi'
      },
      {
        'email': 'mankanshoku@owner.tld',
        'username': 'mankanshoku'
      },
      {
        'email': 'satsuki@revocs.tld',
        'username': 'satsuki',
        'last': True
      }
    ])
    self.assertEqual(notes, '\n'.join([
      'This is a comment concerning this file',
      'that should not be ignored.',
      'Only bug satsuki if matoi or mankanshoku are unavailable.'
    ]))


  def testCollectOwners(self):
    # NOTE: Order matters. The list should be sorted by 'apiName'.
    self.assertEqual(self._owners_ds.get('apis'), [{
      'apiName': 'Core Extensions/Apps Owners',
      'owners': [
        {
          'email': 'satsuki@revocs.tld',
          'username': 'satsuki',
          'last': True
        }
      ],
      'notes': 'Core owners.',
      'id': 'core'
    },
    {
      'apiName': 'a_different_api',
      'owners': [
        {
          'email': 'nonon@owner.tld',
          'username': 'nonon'
        },
        {
          'email': 'matoi@owner.tld',
          'username': 'matoi',
          'last': True
        }
      ],
      'notes': 'Hallo!',
      'id': 'a_different_api'
    },
    {
      'apiName': 'another_api',
      'owners': [],
      'notes': 'Use one of the Core Extensions/Apps Owners.',
      'id': 'another_api'
    },
    {
      'apiName': 'moar_apis',
      'owners': [
        {
          'email': 'satsuki@revocs.tld',
          'username': 'satsuki',
          'last': True
        }
      ],
      'notes': 'For editing moar_apis.',
      'id': 'moar_apis'
    },
    {
      'apiName': 'some_api',
      'owners': [
        {
          'email': 'matoi@owner.tld',
          'username': 'matoi',
          'last': True
        }
      ],
      'notes': '',
      'id': 'some_api'
    }])

if __name__ == '__main__':
  unittest.main()
