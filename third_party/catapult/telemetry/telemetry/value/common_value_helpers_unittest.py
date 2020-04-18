# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import os

from tracing.mre import function_handle

from telemetry import page
from telemetry import story
from telemetry.value import improvement_direction
from telemetry.value import scalar
from telemetry.value import common_value_helpers


def _SingleFileFunctionHandle(filename, function_name, guid):
  return function_handle.FunctionHandle(
      modules_to_load=[function_handle.ModuleToLoad(filename=filename)],
      function_name=function_name, guid=guid)


class TranslateCommonValuesTest(unittest.TestCase):

  def testTranslateScalarValue(self):
    story_set = story.StorySet(base_dir=os.path.dirname(__file__))
    p = page.Page('http://www.foo.com/', story_set, story_set.base_dir, name='foo')

    scalar_value = {
        'type': 'numeric',
        'numeric': {
            'type': 'scalar',
            'unit': 'timeInMs_smallerIsBetter',
            'value': 42
        },
        'name': 'foo',
        'description': 'desc'
    }

    v = common_value_helpers.TranslateScalarValue(scalar_value, p)

    self.assertIsInstance(v, scalar.ScalarValue)
    self.assertEquals('foo', v.name)
    self.assertEquals(p, v.page)
    self.assertEquals('timeInMs', v.units)
    self.assertEquals(42, v.value)
    self.assertEquals(improvement_direction.DOWN, v.improvement_direction)
    self.assertEquals('desc', v.description)

  def testTranslateScalarNoneValue(self):
    story_set = story.StorySet(base_dir=os.path.dirname(__file__))
    p = page.Page('http://www.foo.com/', story_set, story_set.base_dir, name='foo')

    scalar_value = {
        'type': 'numeric',
        'numeric': {
            'type': 'scalar',
            'unit': 'timeInMs_smallerIsBetter',
            'value': None
        },
        'name': 'foo'
    }

    v = common_value_helpers.TranslateScalarValue(scalar_value, p)

    self.assertIsNone(v.value)
    self.assertEquals('Common scalar contained None', v.none_value_reason)
