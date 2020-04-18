# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest

from telemetry.internal.platform import system_info

from gpu_tests import gpu_test_expectations

VENDOR_NVIDIA = 0x10DE
VENDOR_AMD = 0x1002
VENDOR_INTEL = 0x8086

VENDOR_STRING_IMAGINATION = 'Imagination Technologies'
DEVICE_STRING_SGX = 'PowerVR SGX 554'

class StubPlatform(object):
  def __init__(self, os_name, os_version_name=None):
    self.os_name = os_name
    self.os_version_name = os_version_name

  def GetOSName(self):
    return self.os_name

  def GetOSVersionName(self):
    return self.os_version_name

class StubBrowser(object):
  def __init__(self, platform, gpu, device, vendor_string, device_string,
               browser_type=None, gl_renderer=None):
    self.platform = platform
    self.browser_type = browser_type
    sys_info = {
      'model_name': '',
      'gpu': {
        'devices': [
          {'vendor_id': gpu, 'device_id': device,
           'vendor_string': vendor_string, 'device_string': device_string},
        ]
      }
    }
    if gl_renderer:
      sys_info['gpu']['aux_attributes'] = {
        'gl_renderer': gl_renderer
      }
    self.system_info = system_info.SystemInfo.FromDict(sys_info)

  @property
  def supports_system_info(self):
    return False if not self.system_info else True

  def GetSystemInfo(self):
    return self.system_info

class SampleTestExpectations(gpu_test_expectations.GpuTestExpectations):
  def SetExpectations(self):
    # Test GPU conditions.
    self.Fail('test1.html', ['nvidia', 'intel'], bug=123)
    self.Fail('test2.html', [('nvidia', 0x1001), ('nvidia', 0x1002)], bug=123)
    self.Fail('test3.html', ['win', 'intel', ('amd', 0x1001)], bug=123)
    self.Fail('test4.html', ['imagination'])
    self.Fail('test5.html', [('imagination', 'PowerVR SGX 554')])
    # Test ANGLE conditions.
    self.Fail('test6-1.html', ['win', 'd3d9'], bug=345)
    self.Fail('test6-2.html', ['opengl'], bug=345)
    self.Fail('test6-3.html', ['no_angle'], bug=345)
    # Test flaky expectations.
    self.Flaky('test7.html', bug=123, max_num_retries=5)
    self.Flaky('test8.html', ['win'], bug=123, max_num_retries=6)
    self.Flaky('wildcardtest*.html', ['win'], bug=123, max_num_retries=7)

class InvalidDeviceIDExpectation(gpu_test_expectations.GpuTestExpectations):
  def SetExpectations(self):
    self.Fail('test1.html', [('amd', '0x6613')], bug=123)

class GpuTestExpectationsTest(unittest.TestCase):
  def setUp(self):
    self.expectations = SampleTestExpectations()

  def assertExpectationEquals(self, expected, url, platform=StubPlatform(''),
                              gpu=0, device=0, vendor_string='',
                              device_string='', browser_type=None,
                              gl_renderer=None):
    self.expectations.ClearExpectationsCacheForTesting()
    result = self.expectations.GetExpectationForTest(StubBrowser(
      platform, gpu, device, vendor_string, device_string,
      browser_type=browser_type, gl_renderer=gl_renderer), url, '')
    self.assertEquals(expected, result)

  def getRetriesForTest(self, url, platform=StubPlatform(''), gpu=0,
      device=0, vendor_string='', device_string=''):
    self.expectations.ClearExpectationsCacheForTesting()
    return self.expectations.GetFlakyRetriesForTest(StubBrowser(
      platform, gpu, device, vendor_string, device_string), url, '')

  # Tests with expectations for a GPU should only return them when running with
  # that GPU
  def testGpuExpectations(self):
    url = 'http://test.com/test1.html'
    self.assertExpectationEquals('fail', url, gpu=VENDOR_NVIDIA)
    self.assertExpectationEquals('fail', url, gpu=VENDOR_INTEL)
    self.assertExpectationEquals('pass', url, gpu=VENDOR_AMD)

  # Tests with expectations for a GPU should only return them when running with
  # that GPU
  def testGpuDeviceIdExpectations(self):
    url = 'http://test.com/test2.html'
    self.assertExpectationEquals('fail', url, gpu=VENDOR_NVIDIA, device=0x1001)
    self.assertExpectationEquals('fail', url, gpu=VENDOR_NVIDIA, device=0x1002)
    self.assertExpectationEquals('pass', url, gpu=VENDOR_NVIDIA, device=0x1003)
    self.assertExpectationEquals('pass', url, gpu=VENDOR_AMD, device=0x1001)

  # Tests with multiple expectations should only return them when all criteria
  # are met
  def testMultipleExpectations(self):
    url = 'http://test.com/test3.html'
    self.assertExpectationEquals('fail', url,
        StubPlatform('win'), VENDOR_AMD, 0x1001)
    self.assertExpectationEquals('fail', url,
        StubPlatform('win'), VENDOR_INTEL, 0x1002)
    self.assertExpectationEquals('pass', url,
        StubPlatform('win'), VENDOR_NVIDIA, 0x1001)
    self.assertExpectationEquals('pass', url,
        StubPlatform('mac'), VENDOR_AMD, 0x1001)
    self.assertExpectationEquals('pass', url,
        StubPlatform('win'), VENDOR_AMD, 0x1002)

  # Tests with expectations based on GPU vendor string.
  def testGpuVendorStringExpectations(self):
    url = 'http://test.com/test4.html'
    self.assertExpectationEquals('fail', url,
                                 vendor_string=VENDOR_STRING_IMAGINATION,
                                 device_string=DEVICE_STRING_SGX)
    self.assertExpectationEquals('fail', url,
                                 vendor_string=VENDOR_STRING_IMAGINATION,
                                 device_string='Triangle Monster 3000')
    self.assertExpectationEquals('pass', url,
                                 vendor_string='Acme',
                                 device_string=DEVICE_STRING_SGX)

  # Tests with expectations based on GPU vendor and renderer string pairs.
  def testGpuDeviceStringExpectations(self):
    url = 'http://test.com/test5.html'
    self.assertExpectationEquals('fail', url,
                                 vendor_string=VENDOR_STRING_IMAGINATION,
                                 device_string=DEVICE_STRING_SGX)
    self.assertExpectationEquals('pass', url,
                                 vendor_string=VENDOR_STRING_IMAGINATION,
                                 device_string='Triangle Monster 3000')
    self.assertExpectationEquals('pass', url,
                                 vendor_string='Acme',
                                 device_string=DEVICE_STRING_SGX)

  # Test ANGLE conditions.
  def testANGLEConditions(self):
    url = 'http://test.com/test6-1.html'
    self.assertExpectationEquals('pass', url, StubPlatform('win'),
                                 gl_renderer='ANGLE Direct3D11')
    self.assertExpectationEquals('fail', url, StubPlatform('win'),
                                 gl_renderer='ANGLE Direct3D9')

    # Regression test for a native mac GL_RENDERER string matching
    # an ANGLE expectation.
    url = 'http://test.com/test6-2.html'
    self.assertExpectationEquals('pass', url, StubPlatform('mac'),
                                 gl_renderer='Mac Something OpenGL')
    self.assertExpectationEquals('fail', url, StubPlatform('win'),
                                 gl_renderer='ANGLE OpenGL')

    # Tests for the no_angle keyword
    url = 'http://test.com/test6-3.html'
    self.assertExpectationEquals('fail', url, StubPlatform('mac'),
                                 gl_renderer='Mac Something OpenGL')
    self.assertExpectationEquals('pass', url, StubPlatform('win'),
                                 gl_renderer='ANGLE OpenGL')

  # Ensure retry mechanism is working.
  def testFlakyExpectation(self):
    url = 'http://test.com/test7.html'
    self.assertExpectationEquals('flaky', url)
    self.assertEquals(5, self.getRetriesForTest(url))

  # Ensure the filtering from the TestExpectations superclass still works.
  def testFlakyPerPlatformExpectation(self):
    url1 = 'http://test.com/test8.html'
    self.assertExpectationEquals('flaky', url1, StubPlatform('win'))
    self.assertEquals(6, self.getRetriesForTest(url1, StubPlatform('win')))
    self.assertExpectationEquals('pass', url1, StubPlatform('mac'))
    self.assertEquals(0, self.getRetriesForTest(url1, StubPlatform('mac')))
    url2 = 'http://test.com/wildcardtest1.html'
    self.assertExpectationEquals('flaky', url2, StubPlatform('win'))
    self.assertEquals(7, self.getRetriesForTest(url2, StubPlatform('win')))
    self.assertExpectationEquals('pass', url2, StubPlatform('mac'))
    self.assertEquals(0, self.getRetriesForTest(url2, StubPlatform('mac')))

  # Test that device IDs are checked to be integers.
  def testDeviceIDIsInteger(self):
    with self.assertRaises(ValueError):
      InvalidDeviceIDExpectation()
