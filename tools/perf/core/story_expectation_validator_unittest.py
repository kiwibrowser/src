# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import unittest

from core import story_expectation_validator

from telemetry import benchmark
from telemetry import story

class FakePage(object):
  def __init__(self, name):
    self._name = name

  @property
  def name(self):
    return self._name


class FakeStorySetOne(story.StorySet):
  def __init__(self): # pylint: disable=super-init-not-called
    self._stories = [
        FakePage('One'),
        FakePage('Two')
    ]

  @property
  def stories(self):
    return self._stories


class FakeBenchmark(benchmark.Benchmark):
  @classmethod
  def Name(cls):
    return 'b1'

  def CreateStorySet(self, options):
    return FakeStorySetOne()


class StoryExpectationValidatorTest(unittest.TestCase):
  def testValidateStoryInValidName(self):
    raw_expectations = '# tags: Mac\ncrbug.com/123 [ Mac ] b1/s1 [ Skip ]'
    benchmarks = [FakeBenchmark]
    with self.assertRaises(AssertionError):
      story_expectation_validator.validate_story_names(
          benchmarks, raw_expectations)

  def testValidateStoryValidName(self):
    raw_expectations = '# tags: Mac\ncrbug.com/123 [ Mac ] b1/One [ Skip ]'
    benchmarks = [FakeBenchmark]
    # If a name is invalid, an exception is thrown. If no exception is thrown
    # all story names are valid. That is why there is no assert here.
    story_expectation_validator.validate_story_names(
        benchmarks, raw_expectations)

  def testGetDisabledStoriesWithExpectationsData(self):
    raw_expectations = '# tags: Mac\ncrbug.com/123 [ Mac ] b1/One [ Skip ]'
    benchmarks = [FakeBenchmark]
    results = story_expectation_validator.GetDisabledStories(
        benchmarks, raw_expectations)
    expected = {'b1': {'One': [(['Mac'], 'crbug.com/123')]}}
    self.assertEqual(expected, results)

  def testGetDisabledStoriesWithoutMatchingExpectationsData(self):
    raw_expectations = '# tags: Mac\ncrbug.com/123 [ Mac ] b2/One [ Skip ]'
    benchmarks = [FakeBenchmark]
    results = story_expectation_validator.GetDisabledStories(
        benchmarks, raw_expectations)
    expected = { 'b1': {}}
    self.assertEqual(expected, results)
