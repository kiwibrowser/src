# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from StringIO import StringIO
import unittest

import emulation
import test_utils


_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))


class EmulationTestCase(unittest.TestCase):
  def testLoadDevices(self):
    devices = emulation.LoadEmulatedDevices(file(os.path.join(
        _SRC_DIR, 'third_party/blink/renderer/devtools/front_end',
        'emulated_devices/module.json')))
    # Just check we have something. We'll assume that if we were able to read
    # the file without dying we must be ok.
    self.assertTrue(devices)

  def testSetUpDevice(self):
    registry = StringIO("""{
      "extensions": [
          {
              "type": "emulated-device",
              "device": {
                  "show-by-default": false,
                  "title": "mattPhone" ,
                  "screen": {
                      "horizontal": {
                          "width": 480,
                          "height": 320
                      },
                      "device-pixel-ratio": 2,
                      "vertical": {
                          "width": 320,
                          "height": 480
                      }
                  },
                  "capabilities": [
                      "touch",
                      "mobile"
                  ],
                  "user-agent": "James Bond"
              }
          } ]}""")
    devices = emulation.LoadEmulatedDevices(registry)
    connection = test_utils.MockConnection(self)
    connection.ExpectSyncRequest({'result': True}, 'Emulation.canEmulate')
    metadata = emulation.SetUpDeviceEmulationAndReturnMetadata(
        connection, devices['mattPhone'])
    self.assertEqual(320, metadata['width'])
    self.assertEqual('James Bond', metadata['userAgent'])
    self.assertTrue(connection.AllExpectationsUsed())
    self.assertEqual('Emulation.setDeviceMetricsOverride',
                     connection.no_response_requests_seen[0][0])

  def testSetUpNetwork(self):
    connection = test_utils.MockConnection(self)
    connection.ExpectSyncRequest({'result': True},
                                 'Network.canEmulateNetworkConditions')
    emulation.SetUpNetworkEmulation(connection, 120, 2048, 1024)
    self.assertTrue(connection.AllExpectationsUsed())
    self.assertEqual('Network.emulateNetworkConditions',
                     connection.no_response_requests_seen[0][0])
    self.assertEqual(
        1024,
        connection.no_response_requests_seen[0][1]['uploadThroughput'])

  def testBandwidthToString(self):
    self.assertEqual('16Kbit/s', emulation.BandwidthToString(2048))
    self.assertEqual('8Mbit/s', emulation.BandwidthToString(1024 * 1024))


if __name__ == '__main__':
  unittest.main()
