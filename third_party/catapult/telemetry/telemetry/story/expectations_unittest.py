# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest


from py_utils import expectations_parser
from telemetry.story import expectations
from telemetry.testing import fakes


class MockState(object):
  def __init__(self):
    self.platform = fakes.FakePlatform()


class MockStory(object):
  def __init__(self, name):
    self._name = name

  @property
  def name(self):
    return self._name


class MockStorySet(object):
  def __init__(self, stories):
    self._stories = stories

  @property
  def stories(self):
    return self._stories

class MockBrowserFinderOptions(object):
  def __init__(self):
    self._browser_type = None

  @property
  def browser_type(self):
    return self._browser_type

  @browser_type.setter
  def browser_type(self, t):
    assert isinstance(t, basestring)
    self._browser_type = t


class TestConditionTest(unittest.TestCase):
  def setUp(self):
    self._platform = fakes.FakePlatform()
    self._finder_options = MockBrowserFinderOptions()

  def testAllAlwaysReturnsTrue(self):
    self.assertTrue(
        expectations.ALL.ShouldDisable(self._platform, self._finder_options))

  def testAllWinReturnsTrueOnWindows(self):
    self._platform.SetOSName('win')
    self.assertTrue(
        expectations.ALL_WIN.ShouldDisable(self._platform,
                                           self._finder_options))

  def testAllWinReturnsFalseOnOthers(self):
    self._platform.SetOSName('not_windows')
    self.assertFalse(
        expectations.ALL_WIN.ShouldDisable(self._platform,
                                           self._finder_options))

  def testAllLinuxReturnsTrueOnLinux(self):
    self._platform.SetOSName('linux')
    self.assertTrue(expectations.ALL_LINUX.ShouldDisable(self._platform,
                                                         self._finder_options))

  def testAllLinuxReturnsFalseOnOthers(self):
    self._platform.SetOSName('not_linux')
    self.assertFalse(expectations.ALL_LINUX.ShouldDisable(self._platform,
                                                          self._finder_options))

  def testAllMacReturnsTrueOnMac(self):
    self._platform.SetOSName('mac')
    self.assertTrue(expectations.ALL_MAC.ShouldDisable(self._platform,
                                                       self._finder_options))

  def testAllMacReturnsFalseOnOthers(self):
    self._platform.SetOSName('not_mac')
    self.assertFalse(expectations.ALL_MAC.ShouldDisable(self._platform,
                                                        self._finder_options))

  def testAllChromeOSReturnsTrueOnChromeOS(self):
    self._platform.SetOSName('chromeos')
    self.assertTrue(expectations.ALL_CHROMEOS.ShouldDisable(
        self._platform, self._finder_options))

  def testAllChromeOSReturnsFalseOnOthers(self):
    self._platform.SetOSName('not_chromeos')
    self.assertFalse(expectations.ALL_CHROMEOS.ShouldDisable(
        self._platform, self._finder_options))

  def testAllAndroidReturnsTrueOnAndroid(self):
    self._platform.SetOSName('android')
    self.assertTrue(
        expectations.ALL_ANDROID.ShouldDisable(self._platform,
                                               self._finder_options))

  def testAllAndroidReturnsFalseOnOthers(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ALL_ANDROID.ShouldDisable(self._platform,
                                               self._finder_options))

  def testAllDesktopReturnsFalseOnNonDesktop(self):
    false_platforms = ['android']
    for plat in false_platforms:
      self._platform.SetOSName(plat)
      self.assertFalse(
          expectations.ALL_DESKTOP.ShouldDisable(self._platform,
                                                 self._finder_options))

  def testAllDesktopReturnsTrueOnDesktop(self):
    true_platforms = ['win', 'mac', 'linux', 'chromeos']
    for plat in true_platforms:
      self._platform.SetOSName(plat)
      self.assertTrue(
          expectations.ALL_DESKTOP.ShouldDisable(self._platform,
                                                 self._finder_options))

  def testAllMobileReturnsFalseOnNonMobile(self):
    false_platforms = ['win', 'mac', 'linux', 'chromeos']
    for plat in false_platforms:
      self._platform.SetOSName(plat)
      self.assertFalse(
          expectations.ALL_MOBILE.ShouldDisable(self._platform,
                                                self._finder_options))

  def testAllMobileReturnsTrueOnMobile(self):
    true_platforms = ['android']
    for plat in true_platforms:
      self._platform.SetOSName(plat)
      self.assertTrue(
          expectations.ALL_MOBILE.ShouldDisable(self._platform,
                                                self._finder_options))

  def testAndroidNexus5ReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NEXUS5.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus5XReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NEXUS5X.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidNexus6ReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus6PReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus7ReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NEXUS7.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidCherryMobileReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_ONE.ShouldDisable(self._platform,
                                               self._finder_options))

  def testAndroidSvelteReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_SVELTE.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus5ReturnsFalseOnAndroidNotNexus5(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_NEXUS5.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus5XReturnsFalseOnAndroidNotNexus5X(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_NEXUS5X.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidNexus6ReturnsFalseOnAndroidNotNexus6(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus6PReturnsFalseOnAndroidNotNexus6P(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus7ReturnsFalseOnAndroidNotNexus7(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_NEXUS7.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidCherryMobileReturnsFalseOnAndroidNotCherryMobile(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_ONE.ShouldDisable(self._platform,
                                               self._finder_options))

  def testAndroidSvelteReturnsFalseOnAndroidNotSvelte(self):
    self._platform.SetOSName('android')
    self.assertFalse(
        expectations.ANDROID_SVELTE.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus5ReturnsTrueOnAndroidNexus5(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('Nexus 5')
    self.assertTrue(
        expectations.ANDROID_NEXUS5.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus5XReturnsTrueOnAndroidNexus5X(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('Nexus 5X')
    self.assertTrue(
        expectations.ANDROID_NEXUS5X.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidNexus6ReturnsTrueOnAndroidNexus6(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('Nexus 6')
    self.assertTrue(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus6PReturnsTrueOnAndroidNexus6P(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('Nexus 6P')
    self.assertTrue(
        expectations.ANDROID_NEXUS6.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidNexus7ReturnsTrueOnAndroidNexus7(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('Nexus 7')
    self.assertTrue(
        expectations.ANDROID_NEXUS7.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidCherryMobileReturnsTrueOnAndroidCherryMobile(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('W6210')
    self.assertTrue(
        expectations.ANDROID_ONE.ShouldDisable(self._platform,
                                               self._finder_options))

  def testAndroidSvelteReturnsTrueOnAndroidSvelte(self):
    self._platform.SetOSName('android')
    self._platform.SetIsSvelte(True)
    self.assertTrue(
        expectations.ANDROID_SVELTE.ShouldDisable(self._platform,
                                                  self._finder_options))

  def testAndroidWebviewReturnsTrueOnAndroidWebview(self):
    self._platform.SetOSName('android')
    self._platform.SetIsAosp(True)
    self._finder_options.browser_type = 'android-webview'
    self.assertTrue(
        expectations.ANDROID_WEBVIEW.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidWebviewReturnsTrueOnAndroidWebviewGoogle(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview-google'
    self.assertTrue(
        expectations.ANDROID_WEBVIEW.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidWebviewReturnsFalseOnAndroidNotWebview(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-chrome'
    self.assertFalse(
        expectations.ANDROID_WEBVIEW.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidWebviewReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_WEBVIEW.ShouldDisable(self._platform,
                                                   self._finder_options))

  def testAndroidNotWebviewReturnsTrueOnAndroidNotWebview(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android'
    self.assertTrue(
        expectations.ANDROID_NOT_WEBVIEW.ShouldDisable(self._platform,
                                                       self._finder_options))

  def testAndroidNotWebviewReturnsFalseOnAndroidWebview(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self.assertFalse(
        expectations.ANDROID_NOT_WEBVIEW.ShouldDisable(self._platform,
                                                       self._finder_options))

  def testAndroidNotWebviewReturnsFalseOnNotAndroid(self):
    self._platform.SetOSName('not_android')
    self.assertFalse(
        expectations.ANDROID_NOT_WEBVIEW.ShouldDisable(self._platform,
                                                       self._finder_options))
  def testMac1011ReturnsTrueOnMac1011(self):
    self._platform.SetOSName('mac')
    self._platform.SetOsVersionDetailString('10.11')
    self.assertTrue(
        expectations.MAC_10_11.ShouldDisable(self._platform,
                                             self._finder_options))

  def testMac1011ReturnsFalseOnNotMac1011(self):
    self._platform.SetOSName('mac')
    self._platform.SetOsVersionDetailString('10.12')
    self.assertFalse(
        expectations.MAC_10_11.ShouldDisable(self._platform,
                                             self._finder_options))

  def testMac1012ReturnsTrueOnMac1012(self):
    self._platform.SetOSName('mac')
    self._platform.SetOsVersionDetailString('10.12')
    self.assertTrue(
        expectations.MAC_10_12.ShouldDisable(self._platform,
                                             self._finder_options))

  def testMac1012ReturnsFalseOnNotMac1012(self):
    self._platform.SetOSName('mac')
    self._platform.SetOsVersionDetailString('10.11')
    self.assertFalse(
        expectations.MAC_10_12.ShouldDisable(self._platform,
                                             self._finder_options))

  def testNexus5XWebviewFalseOnNotWebview(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android'
    self._platform.SetDeviceTypeName('Nexus 5X')
    self.assertFalse(
        expectations.ANDROID_NEXUS5X_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testNexus5XWebviewFalseOnNotNexus5X(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('Nexus 5')
    self.assertFalse(
        expectations.ANDROID_NEXUS5X_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testNexus5XWebviewReturnsTrue(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('Nexus 5X')
    self.assertTrue(
        expectations.ANDROID_NEXUS5X_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testNexus6WebviewFalseOnNotWebview(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android'
    self._platform.SetDeviceTypeName('Nexus 6')
    self.assertFalse(
        expectations.ANDROID_NEXUS6_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testNexus6WebviewFalseOnNotNexus6(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('Nexus 5X')
    self.assertFalse(
        expectations.ANDROID_NEXUS6_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testNexus6WebviewReturnsTrue(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('Nexus 6')
    self.assertTrue(
        expectations.ANDROID_NEXUS6_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testAndroidNexus6AOSP(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('AOSP on Shamu')
    self.assertTrue(
        expectations.ANDROID_NEXUS6.ShouldDisable(
            self._platform, self._finder_options))

  def testAndroidNexus5XAOSP(self):
    self._platform.SetOSName('android')
    self._platform.SetDeviceTypeName('AOSP on BullHead')
    self.assertTrue(
        expectations.ANDROID_NEXUS5X.ShouldDisable(
            self._platform, self._finder_options))

  def testAndroidNexus6WebviewAOSP(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('AOSP on Shamu')
    self.assertTrue(
        expectations.ANDROID_NEXUS6_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))

  def testAndroidNexus5XWebviewAOSP(self):
    self._platform.SetOSName('android')
    self._finder_options.browser_type = 'android-webview'
    self._platform.SetDeviceTypeName('AOSP on BullHead')
    self.assertTrue(
        expectations.ANDROID_NEXUS5X_WEBVIEW.ShouldDisable(
            self._platform, self._finder_options))


class StoryExpectationsTest(unittest.TestCase):
  def setUp(self):
    self.platform = fakes.FakePlatform()
    self.finder_options = MockBrowserFinderOptions()

  def testCantDisableAfterInit(self):
    e = expectations.StoryExpectations()
    with self.assertRaises(AssertionError):
      e.DisableBenchmark(['test'], 'test')
    with self.assertRaises(AssertionError):
      e.DisableStory('story', ['platform'], 'reason')

  def testDisableBenchmark(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableBenchmark([expectations.ALL_WIN], 'crbug.com/123')

    e = FooExpectations()
    self.platform.SetOSName('win')

    reason = e.IsBenchmarkDisabled(self.platform, self.finder_options)
    self.assertEqual(reason, 'crbug.com/123')

    self.platform.SetOSName('android')
    reason = e.IsBenchmarkDisabled(self.platform, self.finder_options)
    self.assertIsNone(reason)

  def testDisableStoryMultipleConditions(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            'multi', [expectations.ALL_WIN], 'crbug.com/123')
        self.DisableStory(
            'multi', [expectations.ALL_MAC], 'crbug.com/456')

    e = FooExpectations()

    self.platform.SetOSName('mac')
    reason = e.IsStoryDisabled(
        MockStory('multi'), self.platform, self.finder_options)
    self.assertEqual(reason, 'crbug.com/456')

  def testDisableStoryOneCondition(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            'disable', [expectations.ALL_WIN], 'crbug.com/123')

    e = FooExpectations()

    self.platform.SetOSName('win')
    reason = e.IsStoryDisabled(
        MockStory('disable'), self.platform, self.finder_options)
    self.assertEqual(reason, 'crbug.com/123')
    self.platform.SetOSName('mac')
    reason = e.IsStoryDisabled(
        MockStory('disabled'), self.platform, self.finder_options)
    self.assertFalse(reason)
    self.assertIsNone(reason)

  def testDisableStoryWithLongName(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            '123456789012345678901234567890123456789012345678901234567890123456'
            '789012345',
            [expectations.ALL], 'Too Long')

    with self.assertRaises(AssertionError):
      FooExpectations()

  def testDisableStoryWithLongNameStartsWithHttp(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            'http12345678901234567890123456789012345678901234567890123456789012'
            '3456789012345',
            [expectations.ALL], 'Too Long')
    FooExpectations()

  def testDisableStoryWithLongNameContainsHtml(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            '123456789012345678901234567890123456789012345678901234567890123456'
            '789012345.html?and_some_get_parameters',
            [expectations.ALL], 'Too Long')
    FooExpectations()

  def testDisableBenchmarkWithNoReason(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableBenchmark([expectations.ALL_WIN], None)

    e = FooExpectations()

    self.platform.SetOSName('win')
    reason = e.IsBenchmarkDisabled(self.platform, self.finder_options)
    self.assertEqual(reason, 'No reason given')

  def testDisableStoryWithNoReason(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            'story', [expectations.ALL_WIN], None)

    e = FooExpectations()

    self.platform.SetOSName('win')
    reason = e.IsStoryDisabled(
        MockStory('story'), self.platform, self.finder_options)
    self.assertEqual(reason, 'No reason given')

  def testGetBrokenExpectationsNotMatching(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory('bad_name', [expectations.ALL], 'crbug.com/123')

    e = FooExpectations()
    s = MockStorySet([MockStory('good_name')])
    self.assertEqual(e.GetBrokenExpectations(s), ['bad_name'])

  def testGetBrokenExpectationsMatching(self):
    class FooExpectations(expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory('good_name', [expectations.ALL], 'crbug.com/123')

    e = FooExpectations()
    s = MockStorySet([MockStory('good_name')])
    self.assertEqual(e.GetBrokenExpectations(s), [])

  def testGetBenchmarkExpectationsFromParserNoBenchmarkMatch(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/12345', 'benchmark2/story', ['All'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = e.AsDict()
    expected = {'platforms': [], 'stories': {}}
    self.assertEqual(actual, expected)

  @staticmethod
  def _ConvertTestConditionsToStrings(e):
    for story in e['stories']:
      for index in range(0, len(e['stories'][story])):
        conditions, reason = e['stories'][story][index]
        conditions = [str(c) for c in conditions]
        e['stories'][story][index] = (conditions, reason)
    new_platform = []
    for conditions, reason in e['platforms']:
      conditions = '+'.join([str(c) for c in conditions])

      new_platform.append((conditions, reason))
    e['platforms'] = new_platform
    return e

  def testGetBenchmarkExpectationsFromParserOneCondition(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/12345', 'benchmark1/story', ['All'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [],
        'stories': {
            'story': [(['All'], 'crbug.com/12345')],
        }
    }
    self.assertEqual(actual, expected)

  def testGetBenchmarkExpectationsFromParserMultipleConditions(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/23456', 'benchmark1/story', ['Win', 'Mac'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [],
        'stories': {
            'story': [(['Win+Mac'], 'crbug.com/23456')],
        }
    }
    self.assertEqual(actual, expected)

  def testGetBenchmarkExpectationsFromParserMultipleDisablesDifBenchmarks(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/123', 'benchmark1/story', ['Win', 'Mac'], ['Skip']),
        expectations_parser.Expectation(
            'crbug.com/234', 'benchmark1/story2', ['Win'], ['Skip']),
        expectations_parser.Expectation(
            'crbug.com/345', 'benchmark2/story', ['Mac'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')

    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [],
        'stories': {
            'story': [(['Win+Mac'], 'crbug.com/123')],
            'story2': [(['Win'], 'crbug.com/234')]
        }
    }
    self.assertEqual(actual, expected)

  def testGetBenchmarkExpectationsFromParserMultipleDisablesSameBenchmark(
      self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/123', 'benchmark1/story', ['Win'], ['Skip']),
        expectations_parser.Expectation(
            'crbug.com/234', 'benchmark2/story2', ['Win'], ['Skip']),
        expectations_parser.Expectation(
            'crbug.com/345', 'benchmark1/story', ['Mac'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [],
        'stories': {
            'story': [
                (['Win'], 'crbug.com/123'),
                (['Mac'], 'crbug.com/345'),
            ],
        }
    }
    self.assertEqual(actual, expected)

  def testGetBenchmarkExpectationsFromParserUnmappedTag(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/23456', 'benchmark1/story', ['Unmapped_Tag'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    with self.assertRaises(KeyError):
      e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')

  def testGetBenchmarkExpectationsFromParserRefreeze(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/23456', 'benchmark1/story', ['All'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    with self.assertRaises(AssertionError):
      e.DisableStory('story', [], 'reason')

  def testGetBenchmarkExpectationsFromParserDisableBenchmark(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/123', 'benchmark1/*', ['Desktop', 'Linux'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [('Desktop+Linux', 'crbug.com/123')],
        'stories': {},
    }
    self.assertEqual(actual, expected)

  def testGetBenchmarkExpectationsFromParserDisableBenchmarkAndStory(self):
    raw_data = [
        expectations_parser.Expectation(
            'crbug.com/123', 'benchmark1/*', ['Desktop'], ['Skip']),
        expectations_parser.Expectation(
            'crbug.com/234', 'benchmark1/story', ['Mac'], ['Skip']),
    ]
    e = expectations.StoryExpectations()
    e.GetBenchmarkExpectationsFromParser(raw_data, 'benchmark1')
    actual = self._ConvertTestConditionsToStrings(e.AsDict())
    expected = {
        'platforms': [('Desktop', 'crbug.com/123')],
        'stories': {
            'story': [
                (['Mac'], 'crbug.com/234'),
            ],
        },
    }
    self.assertEqual(actual, expected)
