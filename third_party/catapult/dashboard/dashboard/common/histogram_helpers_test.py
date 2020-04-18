# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dashboard.common import histogram_helpers
from dashboard.common import testing_common
from tracing.value import histogram as histogram_module
from tracing.value import histogram_set
from tracing.value.diagnostics import generic_set
from tracing.value.diagnostics import reserved_infos


class HistogramHelpersTest(testing_common.TestCase):

  def setUp(self):
    super(HistogramHelpersTest, self).setUp()

  def testGetTIRLabelFromHistogram_NoTags_ReturnsEmpty(self):
    hist = histogram_module.Histogram('hist', 'count')
    self.assertEqual('', histogram_helpers.GetTIRLabelFromHistogram(hist))

  def testGetTIRLabelFromHistogram_NoValidTags_ReturnsEmpty(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.STORY_TAGS.name,
        generic_set.GenericSet(['foo', 'bar']))
    self.assertEqual('', histogram_helpers.GetTIRLabelFromHistogram(hist))

  def testGetTIRLabelFromHistogram_ValidTags_SortsByKey(self):
    hist = histogram_module.Histogram('hist', 'count')
    histograms = histogram_set.HistogramSet([hist])
    histograms.AddSharedDiagnostic(
        reserved_infos.STORY_TAGS.name,
        generic_set.GenericSet(
            ['z:last', 'ignore', 'a:first', 'me', 'm:middle']))
    self.assertEqual(
        'first_middle_last', histogram_helpers.GetTIRLabelFromHistogram(hist))


