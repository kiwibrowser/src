# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import unittest

from gpu_tests import gpu_test_expectations

from py_utils import discover


class StubPlatform(object):
  def __init__(self, os_name, os_version_name=None):
    self.os_name = os_name
    self.os_version_name = os_version_name

  def GetOSName(self):
    return self.os_name

  def GetOSVersionName(self):
    return self.os_version_name


class StubDevice(object):
  def __init__(self, gpu_vendor_id, gpu_vendor_string, gpu_device_id,
               gpu_device_string):
    self.vendor_id = gpu_vendor_id
    self.vendor_string = gpu_vendor_string
    self.device_id = gpu_device_id
    self.device_string = gpu_device_string


class StubGpuInfo(object):
  def __init__(self, gpu_vendor_id, gpu_vendor_string, gpu_device_id,
               gpu_device_string, gl_renderer, passthrough_cmd_decoder):
    devices = [StubDevice(gpu_vendor_id, gpu_vendor_string, gpu_device_id,
                          gpu_device_string)]
    aux_attributes = {
      'gl_renderer': gl_renderer,
    }
    self.devices = devices
    self.aux_attributes = aux_attributes


class StubSystemInfo(object):
  def __init__(self, gpu_vendor_id, gpu_vendor_string, gpu_device_id,
               gpu_device_string, gl_renderer, passthrough_cmd_decoder):
    self.gpu = StubGpuInfo(gpu_vendor_id, gpu_vendor_string, gpu_device_id,
               gpu_device_string, gl_renderer, passthrough_cmd_decoder)


class StubBrowser(object):
  def __init__(self, os_name, os_version_name, gpu_vendor_id, gpu_vendor_string,
               gpu_device_id, gpu_device_string, gl_renderer,
               passthrough_cmd_decoder, browser_type=None):
    self.platform = StubPlatform(os_name, os_version_name)
    self._system_info = StubSystemInfo(
      gpu_vendor_id, gpu_vendor_string, gpu_device_id, gpu_device_string,
      gl_renderer, passthrough_cmd_decoder)
    self.browser_type = browser_type

  @property
  def supports_system_info(self):
    return True

  def GetSystemInfo(self):
    return self._system_info


class VerifyTestExpectations(unittest.TestCase):
  def setUp(self):
    self._base_dir = os.path.dirname(__file__)
    # Emulate several of the configurations on Chromium's waterfall to try to
    # catch test expectation collisions before they reach either the CQ or
    # waterfall.
    d3d11 = 'ANGLE (Direct3D11)'
    d3d9 = 'ANGLE (Direct3D9)'
    angleogl = 'ANGLE (OpenGL 4.5 core)'
    es30 = 'OpenGL ES 3.0'
    ogl41 = 'OpenGL 4.1'
    ogl45 = 'OpenGL 4.5'
    qcom = 'Qualcomm'
    nvda = 'nvidia'
    ad418 = 'Adreno (TM) 418'
    ad420 = 'Adreno (TM) 420'
    ad430 = 'Adreno (TM) 430'
    tegra = 'NVIDIA Tegra'
    self._browsers = [
      # Windows 10, NVIDIA Quadro P400, D3D11, passthrough cmd decoder
      StubBrowser('win', 'win10', 0x10de, '', 0x1cb3, '', d3d11, True),
      # Windows 10, NVIDIA Quadro P400, D3D9, passthrough cmd decoder
      StubBrowser('win', 'win10', 0x10de, '', 0x1cb3, '', d3d9, True),
      # Windows 10, Intel HD 630, D3D11, passthrough cmd decoder
      StubBrowser('win', 'win10', 0x8086, '', 0x5912, '', d3d11, True),
      # Windows 10, Intel HD 630, D3D9, passthrough cmd decoder
      StubBrowser('win', 'win10', 0x8086, '', 0x5912, '', d3d9, True),
      # Windows 7, AMD R7 240, D3D11, passthrough cmd decoder
      StubBrowser('win', 'win7', 0x1002, '', 0x6613, '', d3d11, True),
      # Windows 7, AMD R7 240, D3D9, passthrough cmd decoder
      StubBrowser('win', 'win7', 0x1002, '', 0x6613, '', d3d9, True),
      # Windows 10, NVIDIA Quadro P400, GL, validating cmd decoder
      StubBrowser('win', 'win10', 0x10de, '', 0x1cb3, '', angleogl, False),
      # macOS Sierra, Intel GPU, validating cmd decoder
      StubBrowser('mac', 'sierra', 0x8086, '', 0x0a2e, '', ogl41, False),
      # macOS Sierra, AMD Retina MacBook Pro, validating cmd decoder
      StubBrowser('mac', 'sierra', 0x1002, '', 0x6821, '', ogl41, False),
      # macOS Sierra, NVIDIA Retina MacBook Pro, validating cmd decoder
      StubBrowser('mac', 'sierra', 0x10de, '', 0x0fe9, '', ogl41, False),
      # macOS Sierra, AMD Mac Pro, validating cmd decoder
      StubBrowser('mac', 'sierra', 0x1002, '', 0x679e, '', ogl41, False),
      # macOS High Sierra, Intel GPU, validating cmd decoder
      StubBrowser('mac', 'highsierra', 0x8086, '', 0x0a2e, '', ogl41, False),
      # macOS High Sierra, AMD Retina MacBook Pro, validating cmd decoder
      StubBrowser('mac', 'highsierra', 0x1002, '', 0x6821, '', ogl41, False),
      # macOS High Sierra, NVIDIA Retina MacBook Pro, validating cmd decoder
      StubBrowser('mac', 'highsierra', 0x10de, '', 0x0fe9, '', ogl41, False),
      # macOS High Sierra, AMD Mac Pro, validating cmd decoder
      StubBrowser('mac', 'highsierra', 0x1002, '', 0x679e, '', ogl41, False),
      # Linux, NVIDIA Quadro P400, validating cmd decoder
      StubBrowser('linux', 'trusty', 0x10de, '', 0x1cb3, '', ogl45, False),
      # Linux, Intel HD 630, validating cmd decoder
      StubBrowser('linux', 'trusty', 0x8086, '', 0x5912, '', ogl45, False),
      # Linux, AMD R7 240, validating cmd decoder
      StubBrowser('linux', 'trusty', 0x1002, '', 0x6613, '', ogl45, False),
      # Android, Nexus 5, validating cmd decoder
      StubBrowser('android', 'L', '', qcom, '', ad418, es30, False),
      # Android, Nexus 5X, validating cmd decoder
      StubBrowser('android', 'M', '', qcom, '', ad418, es30, False),
      # Android, Nexus 6, validating cmd decoder
      StubBrowser('android', 'L', '', qcom, '', ad420, es30, False),
      # Android, Nexus 6P, validating cmd decoder
      StubBrowser('android', 'L', '', qcom, '', ad430, es30, False),
      # Android, Shield TV, validating cmd decoder
      StubBrowser('android', 'N', '', nvda, '', tegra, es30, False),
      # ChromeOS Intel Pinetrail
      StubBrowser('chromeos', '?', 0x8086, '', 0xa011, '', es30, False),
    ]
    # Do not validate the following expectations subclasses, since
    # they're only used for tests.
    self._skipped_expectations = [
      'gpu_test_expectations_unittest.InvalidDeviceIDExpectation',
      'gpu_test_expectations_unittest.SampleTestExpectations',
    ]
    self._known_expectations = set([
      'context_lost_expectations.ContextLostExpectations',
      'depth_capture_expectations.DepthCaptureExpectations',
      'gpu_process_expectations.GpuProcessExpectations',
      'hardware_accelerated_feature_expectations.' + \
      'HardwareAcceleratedFeatureExpectations',
      'info_collection_test.InfoCollectionExpectations',
      'maps_expectations.MapsExpectations',
      'pixel_expectations.PixelExpectations',
      'screenshot_sync_expectations.ScreenshotSyncExpectations',
      'trace_test_expectations.TraceTestExpectations',
      'webgl_conformance_expectations.WebGLConformanceExpectations',
      'webgl2_conformance_expectations.WebGL2ConformanceExpectations',
    ])

  def getFullClassName(self, clz):
    return clz.__module__ + '.' + clz.__name__

  def getAllExpectationsInDirectory(self):
    all_expectations = discover.DiscoverClasses(
      self._base_dir, self._base_dir, gpu_test_expectations.GpuTestExpectations)
    return [
      e for e in all_expectations.itervalues()
      if not self.getFullClassName(e) in self._skipped_expectations]

  def testKnowsAboutAllExpectationsInDirectory(self):
    expectations = self.getAllExpectationsInDirectory()
    for exp in expectations:
      full_name = exp.__module__ + '.' + exp.__name__
      if full_name not in self._known_expectations:
        self.fail('Unknown expectation class ' + exp.__module__ + '.' +
                  exp.__name__)
      if not discover.IsDirectlyConstructable(exp):
        self.fail('Expectation class ' + exp.__module__ + '.' +
                  exp.__name__ +
                  ' must be directly constructible (no required args)')

  def testNoCollisionsInExpectations(self):
    for clz in self.getAllExpectationsInDirectory():
      for b in self._browsers:
        # Contract: all of the GpuTestExpectations subclasses in this
        # directory must be directly constructible (verified above).
        clz()._BuildExpectationsCache(b)
