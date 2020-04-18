# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import fieldtrial_to_struct
import os


class FieldTrialToStruct(unittest.TestCase):

  def test_FieldTrialToDescription(self):
    config = {
      'Trial1': [
        {
          'platforms': ['win'],
          'experiments': [
            {
              'name': 'Group1',
              'params': {
                'x': '1',
                'y': '2'
              },
              'enable_features': ['A', 'B'],
              'disable_features': ['C']
            },
            {
              'name': 'Group2',
              'params': {
                'x': '3',
                'y': '4'
              },
              'enable_features': ['D', 'E'],
              'disable_features': ['F']
            },
          ]
        }
      ],
      'Trial2': [
        {
          'platforms': ['win'],
          'experiments': [{'name': 'OtherGroup'}]
        }
      ],
      'TrialWithForcingFlag':  [
        {
          'platforms': ['win'],
          'experiments': [
            {
              'name': 'ForcedGroup',
              'forcing_flag': "my-forcing-flag"
            }
          ]
        }
      ]
    }
    result = fieldtrial_to_struct._FieldTrialConfigToDescription(config, 'win')
    expected = {
      'elements': {
        'kFieldTrialConfig': {
          'studies': [
            {
              'name': 'Trial1',
              'experiments': [
                {
                  'name': 'Group1',
                  'params': [
                    {'key': 'x', 'value': '1'},
                    {'key': 'y', 'value': '2'}
                  ],
                  'enable_features': ['A', 'B'],
                  'disable_features': ['C']
                },
                {
                  'name': 'Group2',
                  'params': [
                    {'key': 'x', 'value': '3'},
                    {'key': 'y', 'value': '4'}
                  ],
                  'enable_features': ['D', 'E'],
                  'disable_features': ['F']
                },
              ],
            },
            {
              'name': 'Trial2',
              'experiments': [{'name': 'OtherGroup'}]
            },
            {
              'name': 'TrialWithForcingFlag',
              'experiments': [
                  {
                    'name': 'ForcedGroup',
                    'forcing_flag': "my-forcing-flag"
                  }
              ]
            },
          ]
        }
      }
    }
    self.maxDiff = None
    self.assertEqual(expected, result)

  _MULTIPLE_PLATFORM_CONFIG = {
    'Trial1': [
      {
        'platforms': ['win', 'ios'],
        'experiments': [
          {
            'name': 'Group1',
            'params': {
              'x': '1',
              'y': '2'
            },
            'enable_features': ['A', 'B'],
            'disable_features': ['C']
          },
          {
            'name': 'Group2',
            'params': {
              'x': '3',
              'y': '4'
            },
            'enable_features': ['D', 'E'],
            'disable_features': ['F']
          }
        ]
      },
      {
        'platforms': ['ios'],
        'experiments': [
          {
            'name': 'IOSOnly'
          }
        ]
      },
    ],
    'Trial2': [
      {
        'platforms': ['win', 'mac'],
        'experiments': [{'name': 'OtherGroup'}]
      }
    ]
  }

  def test_FieldTrialToDescriptionMultipleSinglePlatformMultipleTrial(self):
    result = fieldtrial_to_struct._FieldTrialConfigToDescription(
        self._MULTIPLE_PLATFORM_CONFIG, 'ios')
    expected = {
      'elements': {
        'kFieldTrialConfig': {
          'studies': [
            {
              'name': 'Trial1',
              'experiments': [
                {
                  'name': 'Group1',
                  'params': [
                    {'key': 'x', 'value': '1'},
                    {'key': 'y', 'value': '2'}
                  ],
                  'enable_features': ['A', 'B'],
                  'disable_features': ['C']
                },
                {
                  'name': 'Group2',
                  'params': [
                    {'key': 'x', 'value': '3'},
                    {'key': 'y', 'value': '4'}
                  ],
                  'enable_features': ['D', 'E'],
                  'disable_features': ['F']
                },
                {
                  'name': 'IOSOnly'
                },
              ],
            },
          ]
        }
      }
    }
    self.maxDiff = None
    self.assertEqual(expected, result)

  def test_FieldTrialToDescriptionMultipleSinglePlatformSingleTrial(self):
    result = fieldtrial_to_struct._FieldTrialConfigToDescription(
        self._MULTIPLE_PLATFORM_CONFIG, 'mac')
    expected = {
      'elements': {
        'kFieldTrialConfig': {
          'studies': [
            {
              'name': 'Trial2',
              'experiments': [
                {
                  'name': 'OtherGroup',
                },
              ],
            },
          ]
        }
      }
    }
    self.maxDiff = None
    self.assertEqual(expected, result)

  def test_FieldTrialToStructMain(self):
    schema = (os.path.dirname(__file__) +
              '/../../components/variations/field_trial_config/'
              'field_trial_testing_config_schema.json')
    unittest_data_dir = os.path.dirname(__file__) + '/unittest_data/'
    test_output_filename = 'test_output'
    fieldtrial_to_struct.main([
      '--schema=' + schema,
      '--output=' + test_output_filename,
      '--platform=win',
      '--year=2015',
      unittest_data_dir + 'test_config.json'
    ])
    header_filename = test_output_filename + '.h'
    with open(header_filename, 'r') as header:
      test_header = header.read()
      with open(unittest_data_dir + 'expected_output.h', 'r') as expected:
        expected_header = expected.read()
        self.assertEqual(expected_header, test_header)
    os.unlink(header_filename)

    cc_filename = test_output_filename + '.cc'
    with open(cc_filename, 'r') as cc:
      test_cc = cc.read()
      with open(unittest_data_dir + 'expected_output.cc', 'r') as expected:
        expected_cc = expected.read()
        self.assertEqual(expected_cc, test_cc)
    os.unlink(cc_filename)

if __name__ == '__main__':
  unittest.main()
