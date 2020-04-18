# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from gpu_tests import gpu_integration_test
from gpu_tests.gpu_test_expectations import GpuTestExpectations

import sys

# There are no expectations for info_collection
class InfoCollectionExpectations(GpuTestExpectations):
  def SetExpectations(self):
    pass

class InfoCollectionTest(gpu_integration_test.GpuIntegrationTest):
  @classmethod
  def Name(cls):
    return 'info_collection'

  @classmethod
  def AddCommandlineArgs(cls, parser):
    parser.add_option('--expected-device-id',
        help='The expected device id')
    parser.add_option('--expected-vendor-id',
        help='The expected vendor id')

  @classmethod
  def GenerateGpuTests(cls, options):
    yield ('_', '_', (options.expected_vendor_id, options.expected_device_id))

  @classmethod
  def SetUpProcess(cls):
    super(cls, InfoCollectionTest).SetUpProcess()
    cls.CustomizeBrowserArgs([])
    cls.StartBrowser()

  def RunActualGpuTest(self, test_path, *args):
    # Make sure the GPU process is started
    self.tab.action_runner.Navigate('chrome:gpu')

    # Gather the IDs detected by the GPU process
    if not self.browser.supports_system_info:
      self.fail("Browser doesn't support GetSystemInfo")

    gpu = self.browser.GetSystemInfo().gpu.devices[0]
    if not gpu:
      self.fail("System Info doesn't have a gpu")

    detected_vendor_id = gpu.vendor_id
    detected_device_id = gpu.device_id

    # Gather the expected IDs passed on the command line
    if args[0] == None or args[1] == None:
      self.fail("Missing --expected-[vendor|device]-id command line args")

    expected_vendor_id = int(args[0], 16)
    expected_device_id = int(args[1], 16)

    # Check expected and detected matches
    if detected_vendor_id != expected_vendor_id:
      self.fail('Vendor ID mismatch, expected %s but got %s.' %
          (expected_vendor_id, detected_vendor_id))

    if detected_device_id != expected_device_id:
      self.fail('device ID mismatch, expected %s but got %s.' %
          (expected_device_id, detected_device_id))

  @classmethod
  def _CreateExpectations(cls):
    return InfoCollectionExpectations()

def load_tests(loader, tests, pattern):
  del loader, tests, pattern  # Unused.
  return gpu_integration_test.LoadAllTestsInModule(sys.modules[__name__])
