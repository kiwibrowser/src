# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import collections
import itertools
import unittest

from telemetry.testing import fakes

from gpu_tests import fake_win_amd_gpu_info
from gpu_tests import test_expectations
from gpu_tests import webgl_conformance_expectations
from gpu_tests import webgl2_conformance_expectations

class FakeWindowsPlatform(fakes.FakePlatform):
  @property
  def is_host_platform(self):
    return True

  def GetDeviceTypeName(self):
    return 'Desktop'

  def GetArchName(self):
    return 'x86_64'

  def GetOSName(self):
    return 'win'

  def GetOSVersionName(self):
    return 'win8'

  def GetOSVersionDetailString(self):
    # Not sure whether this is accurate.
    return 'Windows 8.1'


class WebGLTestInfo(object):
  def __init__(self, url):
    self.name = ('WebglConformance_%s' %
                 url.replace('/', '_').replace('-', '_').
                 replace('\\', '_').rpartition('.')[0].replace('.', '_'))
    self.url = 'file://' + url

Conditions = collections.\
  namedtuple('Conditions', ['non_gpu', 'vendors', 'devices'])

class WebGLConformanceExpectationsTest(unittest.TestCase):
  def testGlslConstructVecMatIndexExpectationOnWin(self):
    possible_browser = fakes.FakePossibleBrowser()
    browser = possible_browser.Create()
    browser.platform = FakeWindowsPlatform()
    browser.returned_system_info = fakes.FakeSystemInfo(
      gpu_dict=fake_win_amd_gpu_info.FAKE_GPU_INFO)
    exps = webgl_conformance_expectations.WebGLConformanceExpectations()
    test_info = WebGLTestInfo(
      'conformance/glsl/constructors/glsl-construct-vec-mat-index.html')
    expectation = exps.GetExpectationForTest(
      browser, test_info.url, test_info.name)
    # TODO(kbr): change this expectation back to "flaky". crbug.com/534697
    self.assertEquals(expectation, 'fail')

  def testWebGLExpectationsHaveNoCollisions(self):
    exps = webgl_conformance_expectations.WebGLConformanceExpectations()
    self.checkConformanceHasNoCollisions(exps)

  def testWebGL2ExpectationsHaveNoCollisions(self):
    exps = webgl2_conformance_expectations.WebGL2ConformanceExpectations()
    self.checkConformanceHasNoCollisions(exps)

  def checkConformanceHasNoCollisions(self, conformance):
    # Checks that no two expectations for the same page can match the
    # same configuration (wildcards expectations are ok though).
    # See webgl2_conformance_expectations.py that contains commented out
    # conflicting expectations that can be used to test this function.
    expectations = conformance.GetAllNonWildcardExpectations()

    # Extract the conditions of the expectations as a tuple of sets, one
    # set for each aspect (OS, GPU...) of the expectations.
    conditions_by_pattern = dict()
    for e in expectations:
      conditions_by_pattern[e.pattern] = []

    for e in expectations:
      # Specifiying 'mac' or 'win' is equivalent to specifying all the
      # OS's versions
      os_conditions = e.os_conditions
      if 'win' in os_conditions:
        os_conditions += test_expectations.WIN_CONDITIONS
      if 'mac' in os_conditions:
        os_conditions += test_expectations.MAC_CONDITIONS

      conditions_by_pattern[e.pattern].append(Conditions(
        (
          set(e.os_conditions),
          set(e.browser_conditions),
          set(e.asan_conditions),
          set(e.cmd_decoder_conditions),
          set(e.angle_conditions),
        ),
        set(e.gpu_conditions),
        set(e.device_id_conditions),
      ))

    for (pattern, conditions) in conditions_by_pattern.iteritems():
      for (c1, c2) in itertools.combinations(conditions, 2):
        # Two conditions for the same page conflict iff we can find a
        # configuration satisfying both conditions, that is iff for each
        # aspect, one of the following is true:
        #  - One of the conditions doesn't specify a requirement for that
        # aspect, which means we can get a value valid for the other condition
        #  - Both conditions have requirements but they intersect
        non_gpu_conflicts = all(
          [len(aspect1.intersection(aspect2)) != 0 or \
            len(aspect1) == 0 or len(aspect2) == 0 \
            for (aspect1, aspect2) in zip(c1.non_gpu, c2.non_gpu)])

        # A GPU configuration matches an expectation if it matches either the
        # GPU vendor condition or the GPU device condition. This means that
        # we can have a conflicting configuration if one of the following
        # is true:
        # - There are no conditions
        # - The GPU vendors or the GPU devices intersect
        # - There is a device of a condition that matches the vendors of
        # the other condition
        gpu_conflicts = \
          len(c1.vendors) + len(c1.devices) == 0 or \
          len(c2.vendors) + len(c2.devices) == 0 or \
          len(c1.vendors.intersection(c2.vendors)) != 0 or \
          len(c1.devices.intersection(c2.devices)) != 0 or \
          any([vendor in c1.vendors for (vendor, _) in c2.devices]) or \
          any([vendor in c2.vendors for (vendor, _) in c1.devices])

        conflicts = non_gpu_conflicts and gpu_conflicts

        if conflicts:
          print "WARNING: Found a conflict for", pattern, " :"
          print "  ", c1
          print "  ", c2

          print "  Type:" + (" (non-gpu)" if non_gpu_conflicts else "") + \
            (" (gpu)" if gpu_conflicts else "")
        self.assertEquals(conflicts, False)
