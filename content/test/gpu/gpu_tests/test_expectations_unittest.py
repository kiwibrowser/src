# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest

from gpu_tests import test_expectations

class StubPlatform(object):
  def __init__(self, os_name, os_version_name=None):
    self.os_name = os_name
    self.os_version_name = os_version_name

  def GetOSName(self):
    return self.os_name

  def GetOSVersionName(self):
    return self.os_version_name


class StubBrowser(object):
  def __init__(self, platform, browser_type=None):
    self.platform = platform
    self.browser_type = browser_type

  @property
  def supports_system_info(self):
    return False


class SampleExpectationSubclass(test_expectations.Expectation):
  def __init__(self, expectation, pattern, conditions=None, bug=None):
    self.valid_condition_matched = False
    self.valid_condition_unmatched = False
    super(SampleExpectationSubclass, self).__init__(
      expectation, pattern, conditions=conditions, bug=bug)

  def ParseCondition(self, condition):
    if condition == 'valid_condition_matched':
      self.valid_condition_matched = True
    elif condition == 'valid_condition_unmatched':
      self.valid_condition_unmatched = True
    else:
      super(SampleExpectationSubclass, self).ParseCondition(condition)


class SampleTestExpectations(test_expectations.TestExpectations):
  def __init__(self, url_prefixes=None, is_asan=False):
    super(SampleTestExpectations, self).__init__(
      url_prefixes=url_prefixes, is_asan=is_asan)

  def CreateExpectation(self, expectation, url_pattern, conditions=None,
                        bug=None):
    return SampleExpectationSubclass(expectation, url_pattern,
                                     conditions=conditions, bug=bug)

  def SetExpectations(self):
    self.Fail('page1.html', ['win', 'mac'], bug=123)
    self.Fail('page2.html', ['vista'], bug=123)
    self.Fail('page3.html', bug=123)
    self.Fail('page4.*', bug=123)
    self.Fail('Pages.page_6')
    self.Fail('page7.html', ['mountainlion'])
    self.Fail('page8.html', ['mavericks'])
    self.Fail('page9.html', ['yosemite'])
    # Test browser conditions.
    self.Fail('page10.html', ['android', 'android-webview-instrumentation'],
              bug=456)
    # Test user defined conditions.
    self.Fail('page11.html', ['win', 'valid_condition_matched'])
    self.Fail('page12.html', ['win', 'valid_condition_unmatched'])
    # Test file:// scheme.
    self.Fail('conformance/attribs/page13.html')
    # Test file:// scheme on Windows.
    self.Fail('conformance/attribs/page14.html', ['win'])
    # Explicitly matched paths have precedence over wildcards.
    self.Fail('conformance/glsl/*')
    self.Skip('conformance/glsl/page15.html')
    # Test ASAN expectations.
    self.Fail('page16.html', ['mac', 'asan'])
    self.Fail('page17.html', ['mac', 'no_asan'])
    # Explicitly specified ASAN expectations should not collide.
    self.Skip('page18.html', ['mac', 'asan'])
    self.Fail('page18.html', ['mac', 'no_asan'])

  def _ExpectationAppliesToTest(
      self, expectation, browser, test_url, test_name):
    if not super(SampleTestExpectations, self)._ExpectationAppliesToTest(
        expectation, browser, test_url, test_name):
      return False
    # These tests can probably be expressed more tersely, but that
    # would reduce readability.
    if (not expectation.valid_condition_matched and
        not expectation.valid_condition_unmatched):
      return True
    return expectation.valid_condition_matched

class FailingAbsoluteTestExpectations(test_expectations.TestExpectations):
  def CreateExpectation(self, expectation, url_pattern, conditions=None,
                        bug=None):
    return SampleExpectationSubclass(expectation, url_pattern,
                                     conditions=conditions, bug=bug)

  def SetExpectations(self):
    self.Fail('http://test.com/page5.html', bug=123)

class TestExpectationsTest(unittest.TestCase):
  def setUpHelper(self, is_asan=False):
    self.expectations = SampleTestExpectations(url_prefixes=[
      'third_party/webgl/src/sdk/tests/',
      'content/test/data/gpu'], is_asan=is_asan)

  def setUp(self):
    self.setUpHelper()

  def assertExpectationEquals(self, expected, url, platform=StubPlatform(''),
                              browser_type=None):
    self.assertExpectationEqualsForName(
      expected, url, '', platform=platform, browser_type=browser_type)

  def assertExpectationEqualsForName(
      self, expected, url, name, platform=StubPlatform(''), browser_type=None):
    self.expectations.ClearExpectationsCacheForTesting()
    result = self.expectations.GetExpectationForTest(
      StubBrowser(platform, browser_type=browser_type), url, name)
    self.assertEquals(expected, result)

  # Tests with no expectations should always return 'pass'
  def testNoExpectations(self):
    url = 'http://test.com/page0.html'
    self.assertExpectationEquals('pass', url, StubPlatform('win'))

  # Tests with expectations for an OS should only return them when running on
  # that OS
  def testOSExpectations(self):
    url = 'http://test.com/page1.html'
    self.assertExpectationEquals('fail', url, StubPlatform('win'))
    self.assertExpectationEquals('fail', url, StubPlatform('mac'))
    self.assertExpectationEquals('pass', url, StubPlatform('linux'))

  # Tests with expectations for an OS version should only return them when
  # running on that OS version
  def testOSVersionExpectations(self):
    url = 'http://test.com/page2.html'
    self.assertExpectationEquals('fail', url, StubPlatform('win', 'vista'))
    self.assertExpectationEquals('pass', url, StubPlatform('win', 'win7'))

  # Tests with non-conditional expectations should always return that
  # expectation regardless of OS or OS version
  def testConditionlessExpectations(self):
    url = 'http://test.com/page3.html'
    self.assertExpectationEquals('fail', url, StubPlatform('win'))
    self.assertExpectationEquals('fail', url, StubPlatform('mac', 'lion'))
    self.assertExpectationEquals('fail', url, StubPlatform('linux'))

  # Expectations with wildcard characters should return for matching patterns
  def testWildcardExpectations(self):
    url = 'http://test.com/page4.html'
    url_js = 'http://test.com/page4.js'
    self.assertExpectationEquals('fail', url, StubPlatform('win'))
    self.assertExpectationEquals('fail', url_js, StubPlatform('win'))

  # Absolute URLs in expectations are no longer allowed.
  def testAbsoluteExpectationsAreForbidden(self):
    with self.assertRaises(ValueError):
      FailingAbsoluteTestExpectations()

  # Expectations can be set against test names as well as urls
  def testTestNameExpectations(self):
    self.assertExpectationEqualsForName(
      'fail', 'http://test.com/page6.html', 'Pages.page_6')

  # Verify version-specific Mac expectations.
  def testMacVersionExpectations(self):
    url = 'http://test.com/page7.html'
    self.assertExpectationEquals('fail', url,
                                 StubPlatform('mac', 'mountainlion'))
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'mavericks'))
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'yosemite'))
    url = 'http://test.com/page8.html'
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'mountainlion'))
    self.assertExpectationEquals('fail', url,
                                 StubPlatform('mac', 'mavericks'))
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'yosemite'))
    url = 'http://test.com/page9.html'
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'mountainlion'))
    self.assertExpectationEquals('pass', url,
                                 StubPlatform('mac', 'mavericks'))
    self.assertExpectationEquals('fail', url,
                                 StubPlatform('mac', 'yosemite'))

  # Test browser type conditions.
  def testBrowserTypeConditions(self):
    url = 'http://test.com/page10.html'
    self.assertExpectationEquals('pass', url, StubPlatform('android'),
                                 browser_type='android-content-shell')
    self.assertExpectationEquals('fail', url, StubPlatform('android'),
                                 browser_type='android-webview-instrumentation')

  # Tests with user-defined conditions.
  def testUserDefinedConditions(self):
    url = 'http://test.com/page11.html'
    self.assertExpectationEquals('fail', url, StubPlatform('win'))
    url = 'http://test.com/page12.html'
    self.assertExpectationEquals('pass', url, StubPlatform('win'))

  # The file:// scheme is treated specially by Telemetry; it's
  # translated into an HTTP request against localhost. Expectations
  # against it must continue to work.
  def testFileScheme(self):
    url = 'file://conformance/attribs/page13.html'
    self.assertExpectationEquals('fail', url)

  # Telemetry uses backslashes in its file:// URLs on Windows.
  def testFileSchemeOnWindows(self):
    url = 'file://conformance\\attribs\\page14.html'
    self.assertExpectationEquals('pass', url)
    self.assertExpectationEquals('fail', url, StubPlatform('win'))

  def testExplicitPathsHavePrecedenceOverWildcards(self):
    url = 'http://test.com/conformance/glsl/page00.html'
    self.assertExpectationEquals('fail', url)
    url = 'http://test.com/conformance/glsl/page15.html'
    self.assertExpectationEquals('skip', url)

  def testQueryArgsAreStrippedFromFileURLs(self):
    url = 'file://conformance/glsl/page15.html?webglVersion=2'
    self.assertExpectationEquals('skip', url)

  def testURLPrefixesAreStripped(self):
    self.assertExpectationEqualsForName(
      'skip',
      'third_party/webgl/src/sdk/tests/conformance/glsl/page15.html',
      'Page15')
    self.assertExpectationEqualsForName(
      'fail',
      'third_party/webgl/src/sdk/tests/conformance/glsl/foo.html',
      'Foo')
    # Test exact and wildcard matches on Windows.
    self.assertExpectationEqualsForName(
      'skip',
      'third_party\\webgl\\src\\sdk\\tests\\conformance\\glsl\\page15.html',
      'Page15',
      StubPlatform('win'))
    self.assertExpectationEqualsForName(
      'fail',
      'third_party\\webgl\\src\\sdk\\tests\\conformance\\glsl\\foo.html',
      'Foo',
      StubPlatform('win'))

  def testCaseInsensitivity(self):
    url = 'http://test.com/page1.html'
    self.assertExpectationEquals('fail', url, StubPlatform('Win'))
    url = 'http://test.com/page2.html'
    self.assertExpectationEquals('fail', url, StubPlatform('Win', 'Vista'))
    url = 'http://test.com/page10.html'
    self.assertExpectationEquals('fail', url, StubPlatform('android'),
                                 browser_type='Android-Webview-Instrumentation')

  def testASANExpectations(self):
    url16 = 'page16.html'
    url18 = 'page18.html'
    self.assertExpectationEquals('pass', url16, StubPlatform('mac'))
    self.assertExpectationEquals('fail', url18, StubPlatform('mac'))
    self.setUpHelper(is_asan=True)
    self.assertExpectationEquals('fail', url16, StubPlatform('mac'))
    self.assertExpectationEquals('skip', url18, StubPlatform('mac'))
