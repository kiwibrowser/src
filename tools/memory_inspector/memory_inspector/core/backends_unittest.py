# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from memory_inspector.core import backends


class MockDevice(backends.Device):  # pylint: disable=W0223
  def __init__(self, backend, device_id):
    super(MockDevice, self).__init__(backend)
    self.device_id = device_id

  @property
  def name(self):
    return "Mock Device %s" % self.device_id

  @property
  def id(self):
    return self.device_id


class MockBackend(backends.Backend):
  _SETTINGS = {'key_1': 'key descritpion 1'}

  def __init__(self, backend_name):
    super(MockBackend, self).__init__(MockBackend._SETTINGS)
    self.backend_name = backend_name

  def EnumerateDevices(self):
    yield MockDevice(self, 'device-1')
    yield MockDevice(self, 'device-2')

  def ExtractSymbols(self, native_heaps, sym_paths):
    raise NotImplementedError()

  @property
  def name(self):
    return self.backend_name


class BackendRegisterTest(unittest.TestCase):
  def runTest(self):
    mock_backend_1 = MockBackend('mock-backend-1')
    mock_backend_2 = MockBackend('mock-backend-2')
    self.assertEqual(mock_backend_1.settings['key_1'], 'key descritpion 1')
    backends.Register(mock_backend_1)
    backends.Register(mock_backend_2)
    devices = list(backends.ListDevices())
    self.assertEqual(len(devices), 4)
    self.assertIsNotNone(backends.GetDevice('mock-backend-1', 'device-1'))
    self.assertIsNotNone(backends.GetDevice('mock-backend-1', 'device-2'))
    self.assertIsNotNone(backends.GetDevice('mock-backend-2', 'device-1'))
    self.assertIsNotNone(backends.GetDevice('mock-backend-2', 'device-1'))
    self.assertTrue('key_1' in mock_backend_1.settings)
