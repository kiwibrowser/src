#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import cygprofile_utils


class TestCygprofileUtils(unittest.TestCase):
  def testInvertMapping(self):
    inputMap = {'1': ['2', '3'],
                '4': ['2', '5']}
    self.assertEqual(cygprofile_utils.InvertMapping(inputMap),
        {'2': ['1', '4'],
         '3': ['1'],
         '5': ['4']})


if __name__ == '__main__':
  unittest.main()
