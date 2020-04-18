# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging


class StoryExpectations(object):
  """An object that contains disabling expectations for benchmarks and stories.

  Example Usage:
  class FooBenchmarkExpectations(expectations.StoryExpectations):
    def SetExpectations(self):
      self.DisableBenchmark(
          [expectations.ALL_MOBILE], 'Desktop Benchmark')
      self.DisableStory('story_name1', [expectations.ALL_MAC], 'crbug.com/456')
      self.DisableStory('story_name2', [expectations.ALL], 'crbug.com/789')
      ...
  """
  def __init__(self):
    self._disabled_platforms = []
    self._expectations = {}
    self._frozen = False
    self.SetExpectations()
    self._Freeze()

  # TODO(rnephew): Transform parsed expectation file into StoryExpectations.
  # When working on this it's important to note that StoryExpectations uses
  # logical OR to combine multiple conditions in a single expectation. The
  # expectation files use logical AND when combining multiple conditions.
  # crbug.com/781409
  def GetBenchmarkExpectationsFromParser(self, expectations, benchmark):
    """Transforms parser expections into StoryExpectations

    Args:
      expectations: parser.expectations property from expectations_parser
      benchmark: Name of benchmark to extract expectations for.

    Side effects:
      Adds parser expectations to self._expectations dictionary.
    """
    try:
      self._frozen = False
      for expectation in expectations:
        if not expectation.test.startswith('%s/' % benchmark):
          continue
        # Strip off benchmark name. In format: benchmark/story
        story = expectation.test[len(benchmark)+1:]
        try:
          conditions = (
              [EXPECTATION_NAME_MAP[c] for c in expectation.conditions])
        except KeyError:
          logging.critical(
              'Unable to map expectation in file to TestCondition')
          raise

        # Test Expectations with multiple conditions are treated as logical
        # and and require all conditions to be met for disabling to occur.
        # By design, StoryExpectations treats lists as logical or so we must
        # construct TestConditions using the logical and helper class.
        # TODO(): Consider refactoring TestConditions to be logical or.
        conditions_str = '+'.join(expectation.conditions)
        composite_condition = _TestConditionLogicalAndConditions(
            conditions, conditions_str)

        if story == '*':
          self.DisableBenchmark([composite_condition], expectation.reason)
        else:
          self.DisableStory(story, [composite_condition], expectation.reason)
    finally:
      self._Freeze()

  def AsDict(self):
    """Returns information on disabled stories/benchmarks as a dictionary"""
    return {
        'platforms': self._disabled_platforms,
        'stories': self._expectations
    }

  def GetBrokenExpectations(self, story_set):
    story_set_story_names = [s.name for s in story_set.stories]
    invalid_story_names = []
    for story_name in self._expectations:
      if story_name not in story_set_story_names:
        invalid_story_names.append(story_name)
        logging.error('Story %s is not in the story set.' % story_name)
    return invalid_story_names

  # TODO(rnephew): When TA/DA conversion is complete, remove this method.
  def SetExpectations(self):
    """Sets the Expectations for test disabling

    Override in subclasses to disable tests."""
    pass

  def _Freeze(self):
    self._frozen = True


  @property
  def disabled_platforms(self):
    return self._disabled_platforms

  def DisableBenchmark(self, conditions, reason):
    """Temporarily disable failing benchmarks under the given conditions.

    This means that even if --also-run-disabled-tests is passed, the benchmark
    will not run. Some benchmarks (such as system_health.mobile_* benchmarks)
    contain android specific commands and as such, cannot run on desktop
    platforms under any condition.

    Example:
      DisableBenchmark(
          [expectations.ALL_MOBILE], 'Desktop benchmark')

    Args:
      conditions: List of _TestCondition subclasses.
      reason: Reason for disabling the benchmark.
    """
    assert not self._frozen, ('Cannot disable benchmark on a frozen '
                              'StoryExpectation object.')
    for condition in conditions:
      assert isinstance(condition, _TestCondition)

    self._disabled_platforms.append((conditions, reason))

  def IsBenchmarkDisabled(self, platform, finder_options):
    """Returns the reason the benchmark was disabled, or None if not disabled.

    Args:
      platform: A platform object.
    """
    for conditions, reason in self._disabled_platforms:
      for condition in conditions:
        if condition.ShouldDisable(platform, finder_options):
          logging.info('Benchmark permanently disabled on %s due to %s.',
                       condition, reason)
          return reason if reason is not None else 'No reason given'
    return None

  def DisableStory(self, story_name, conditions, reason):
    """Disable the story under the given conditions.

    Example:
      DisableStory('story_name', [expectations.ALL_WIN], 'crbug.com/123')

    Args:
      story_name: Name of the story to disable passed as a string.
      conditions: List of _TestCondition subclasses.
      reason: Reason for disabling the story.
    """
    # TODO(rnephew): Remove http check when old stories that use urls as names
    # are removed.
    if not (story_name.startswith('http') or '.html' in story_name):
      # Decrease to 50 after we shorten names of existing tests.
      assert len(story_name) < 75, (
          "Story name exceeds limit of 75 characters. This limit is in place to"
          " encourage Telemetry benchmark owners to use short, simple story "
          "names (e.g. 'google_search_images', not "
          "'http://www.google.com/images/1234/abc')."

      )
    assert not self._frozen, ('Cannot disable stories on a frozen '
                              'StoryExpectation object.')
    for condition in conditions:
      assert isinstance(condition, _TestCondition)
    if not self._expectations.get(story_name):
      self._expectations[story_name] = []
    self._expectations[story_name].append((conditions, reason))

  def IsStoryDisabled(self, story, platform, finder_options):
    """Returns the reason the story was disabled, or None if not disabled.

    Args:
      story: Story object that contains a name property.
      platform: A platform object.

    Returns:
      Reason if disabled, None otherwise.
    """
    for conditions, reason in self._expectations.get(story.name, []):
      for condition in conditions:
        if condition.ShouldDisable(platform, finder_options):
          logging.info('%s is disabled on %s due to %s.',
                       story.name, condition, reason)
          return reason if reason is not None else 'No reason given'
    return None


# TODO(rnephew): Since TestConditions are being used for more than
# just story expectations now, this should be decoupled and refactored
# to be clearer.
class _TestCondition(object):
  def ShouldDisable(self, platform, finder_options):
    raise NotImplementedError

  def __str__(self):
    raise NotImplementedError


class _TestConditionByPlatformList(_TestCondition):
  def __init__(self, platforms, name):
    self._platforms = platforms
    self._name = name

  def ShouldDisable(self, platform, finder_options):
    del finder_options  # Unused.
    return platform.GetOSName() in self._platforms

  def __str__(self):
    return self._name


class _AllTestCondition(_TestCondition):
  def ShouldDisable(self, platform, finder_options):
    del platform, finder_options  # Unused.
    return True

  def __str__(self):
    return 'All'


class _TestConditionAndroidSvelte(_TestCondition):
  """Matches android devices with a svelte (low-memory) build."""
  def ShouldDisable(self, platform, finder_options):
    del finder_options  # Unused.
    return platform.GetOSName() == 'android' and platform.IsSvelte()

  def __str__(self):
    return 'Android Svelte'


class _TestConditionByAndroidModel(_TestCondition):
  def __init__(self, model, name=None):
    self._model = model
    self._name = name if name else model

  def ShouldDisable(self, platform, finder_options):
    return (platform.GetOSName() == 'android' and
            self._model in platform.GetDeviceTypeName())

  def __str__(self):
    return self._name

class _TestConditionAndroidWebview(_TestCondition):
  def ShouldDisable(self, platform, finder_options):
    return (platform.GetOSName() == 'android' and
            finder_options.browser_type.startswith('android-webview'))

  def __str__(self):
    return 'Android Webview'

class _TestConditionAndroidNotWebview(_TestCondition):
  def ShouldDisable(self, platform, finder_options):
    return (platform.GetOSName() == 'android' and not
            finder_options.browser_type.startswith('android-webview'))

  def __str__(self):
    return 'Android but not webview'


class _TestConditionByMacVersion(_TestCondition):
  def __init__(self, version, name=None):
    self._version = version
    self._name = name

  def __str__(self):
    return self._name

  def ShouldDisable(self, platform, finder_options):
    if platform.GetOSName() != 'mac':
      return False
    return platform.GetOSVersionDetailString().startswith(self._version)


class _TestConditionLogicalAndConditions(_TestCondition):
  def __init__(self, conditions, name):
    self._conditions = conditions
    self._name = name

  def __str__(self):
    return self._name

  def ShouldDisable(self, platform, finder_options):
    return all(
        c.ShouldDisable(platform, finder_options) for c in self._conditions)


class _TestConditionLogicalOrConditions(_TestCondition):
  def __init__(self, conditions, name):
    self._conditions = conditions
    self._name = name

  def __str__(self):
    return self._name

  def ShouldDisable(self, platform, finder_options):
    return any(
        c.ShouldDisable(platform, finder_options) for c in self._conditions)


ALL = _AllTestCondition()
ALL_MAC = _TestConditionByPlatformList(['mac'], 'Mac')
ALL_WIN = _TestConditionByPlatformList(['win'], 'Win')
ALL_LINUX = _TestConditionByPlatformList(['linux'], 'Linux')
ALL_CHROMEOS = _TestConditionByPlatformList(['chromeos'], 'ChromeOS')
ALL_ANDROID = _TestConditionByPlatformList(['android'], 'Android')
ALL_DESKTOP = _TestConditionByPlatformList(
    ['mac', 'linux', 'win', 'chromeos'], 'Desktop')
ALL_MOBILE = _TestConditionByPlatformList(['android'], 'Mobile')
ANDROID_NEXUS5 = _TestConditionByAndroidModel('Nexus 5')
_ANDROID_NEXUS5X = _TestConditionByAndroidModel('Nexus 5X')
_ANDROID_NEXUS5XAOSP = _TestConditionByAndroidModel('AOSP on BullHead')
ANDROID_NEXUS5X = _TestConditionLogicalOrConditions(
    [_ANDROID_NEXUS5X, _ANDROID_NEXUS5XAOSP], 'Nexus 5X')
_ANDROID_NEXUS6 = _TestConditionByAndroidModel('Nexus 6')
_ANDROID_NEXUS6AOSP = _TestConditionByAndroidModel('AOSP on Shamu')
ANDROID_NEXUS6 = _TestConditionLogicalOrConditions(
    [_ANDROID_NEXUS6, _ANDROID_NEXUS6AOSP], 'Nexus 6')
ANDROID_NEXUS6P = _TestConditionByAndroidModel('Nexus 6P')
ANDROID_NEXUS7 = _TestConditionByAndroidModel('Nexus 7')
ANDROID_GO = _TestConditionByAndroidModel('gobo', 'Android Go')
ANDROID_ONE = _TestConditionByAndroidModel('W6210', 'Android One')
ANDROID_SVELTE = _TestConditionAndroidSvelte()
ANDROID_LOW_END = _TestConditionLogicalOrConditions(
    [ANDROID_GO, ANDROID_SVELTE, ANDROID_ONE], 'Android Low End')
ANDROID_WEBVIEW = _TestConditionAndroidWebview()
ANDROID_NOT_WEBVIEW = _TestConditionAndroidNotWebview()
# MAC_10_11 Includes:
#   Mac 10.11 Perf, Mac Retina Perf, Mac Pro 10.11 Perf, Mac Air 10.11 Perf
MAC_10_11 = _TestConditionByMacVersion('10.11', 'Mac 10.11')
# Mac 10_12 Includes:
#   Mac 10.12 Perf, Mac Mini 8GB 10.12 Perf
MAC_10_12 = _TestConditionByMacVersion('10.12', 'Mac 10.12')
ANDROID_NEXUS6_WEBVIEW = _TestConditionLogicalAndConditions(
    [ANDROID_NEXUS6, ANDROID_WEBVIEW], 'Nexus6 Webview')
ANDROID_NEXUS5X_WEBVIEW = _TestConditionLogicalAndConditions(
    [ANDROID_NEXUS5X, ANDROID_WEBVIEW], 'Nexus5X Webview')

EXPECTATION_NAME_MAP = {
    'All': ALL,
    'Android_Go': ANDROID_GO,
    'Android_One': ANDROID_ONE,
    'Android_Svelte': ANDROID_SVELTE,
    'Android_Low_End': ANDROID_LOW_END,
    'Android_Webview': ANDROID_WEBVIEW,
    'Android_but_not_webview': ANDROID_NOT_WEBVIEW,
    'Mac': ALL_MAC,
    'Win': ALL_WIN,
    'Linux': ALL_LINUX,
    'ChromeOS': ALL_CHROMEOS,
    'Android': ALL_ANDROID,
    'Desktop': ALL_DESKTOP,
    'Mobile': ALL_MOBILE,
    'Nexus_5': ANDROID_NEXUS5,
    'Nexus_5X': ANDROID_NEXUS5X,
    'Nexus_6': ANDROID_NEXUS6,
    'Nexus_6P': ANDROID_NEXUS6P,
    'Nexus_7': ANDROID_NEXUS7,
    'Mac_10.11': MAC_10_11,
    'Mac_10.12': MAC_10_12,
    'Nexus6_Webview': ANDROID_NEXUS6_WEBVIEW,
    'Nexus5X_Webview': ANDROID_NEXUS5X_WEBVIEW
}
