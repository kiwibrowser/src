# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import fieldtrial_util
import os
import tempfile


class FieldTrialUtilUnittest(unittest.TestCase):

  def runGenerateArgs(self, config, platform):
    result = None
    with tempfile.NamedTemporaryFile('w', delete=False) as base_file:
      try:
        base_file.write(config)
        base_file.close()
        result = fieldtrial_util.GenerateArgs(base_file.name, platform)
      finally:
        os.unlink(base_file.name)
    return result

  def test_GenArgsEmptyPaths(self):
    args = fieldtrial_util.GenerateArgs('', 'linux')
    self.assertEqual([], args)

  def test_GenArgsOneConfig(self):
    config = '''{
      "BrowserBlackList": [
        {
          "platforms": ["win"],
          "experiments": [{"name": "Enabled"}]
        }
      ],
      "SimpleParams": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "Default",
              "params": {"id": "abc"},
              "enable_features": ["a", "b"]
            }
          ]
        }
      ],
      "c": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "d.",
              "params": {"url": "http://www.google.com"},
              "enable_features": ["x"],
              "disable_features": ["y"]
            }
          ]
        }
      ]
    }'''
    result = self.runGenerateArgs(config, 'win')
    self.assertEqual(['--force-fieldtrials='
        'BrowserBlackList/Enabled/SimpleParams/Default/c/d.',
        '--force-fieldtrial-params='
        'SimpleParams.Default:id/abc,'
        'c.d%2E:url/http%3A%2F%2Fwww%2Egoogle%2Ecom',
        '--enable-features=a<SimpleParams,b<SimpleParams,x<c',
        '--disable-features=y<c'], result)

  def test_DuplicateEnableFeatures(self):
    config = '''{
      "X": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "x",
              "enable_features": ["x"]
            }
          ]
        }
      ],
      "Y": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "Default",
              "enable_features": ["x", "y"]
            }
          ]
        }
      ]
    }'''
    with self.assertRaises(Exception) as raised:
      self.runGenerateArgs(config, 'win')
    self.assertEqual('Duplicate feature(s) in enable_features: x',
                     str(raised.exception))

  def test_DuplicateDisableFeatures(self):
    config = '''{
      "X": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "x",
              "enable_features": ["y", "z"]
            }
          ]
        }
      ],
      "Y": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "Default",
              "enable_features": ["z", "x", "y"]
            }
          ]
        }
      ]
    }'''
    with self.assertRaises(Exception) as raised:
      self.runGenerateArgs(config, 'win')
    self.assertEqual('Duplicate feature(s) in enable_features: y, z',
                     str(raised.exception))


  def test_DuplicateEnableDisable(self):
    config = '''{
      "X": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "x",
              "enable_features": ["x"]
            }
          ]
        }
      ],
      "Y": [
        {
          "platforms": ["win"],
          "experiments": [
            {
              "name": "Default",
              "disable_features": ["x", "y"]
            }
          ]
        }
      ]
    }'''
    with self.assertRaises(Exception) as raised:
      self.runGenerateArgs(config, 'win')
    self.assertEqual('Conflicting features set as both enabled and disabled: x',
                     str(raised.exception))

if __name__ == '__main__':
  unittest.main()
