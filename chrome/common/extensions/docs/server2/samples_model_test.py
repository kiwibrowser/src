#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import unittest

from future import Future
from server_instance import ServerInstance
from test_file_system import TestFileSystem
from test_util import Server2Path


def _ReadLocalFile(filename):
  base_path = Server2Path('test_data', 'samples_data_source')
  with open(os.path.join(base_path, filename), 'r') as f:
    return f.read()


class _FakeCache(object):
  def __init__(self, obj):
    self._cache = obj

  def GetFromFileListing(self, _):
    return Future(value=self._cache)


class SamplesModelSourceTest(unittest.TestCase):
  def setUp(self):
    server_instance = ServerInstance.ForTest(file_system=TestFileSystem({}))
    self._samples_model = server_instance.platform_bundle.GetSamplesModel(
        'apps')
    self._samples_model._samples_cache = _FakeCache(json.loads(_ReadLocalFile(
        'samples.json')))

  def testFilterSamples(self):
    self.assertEquals(json.loads(_ReadLocalFile('expected.json')),
                      self._samples_model.FilterSamples('bobaloo').Get())

if __name__ == '__main__':
  unittest.main()
