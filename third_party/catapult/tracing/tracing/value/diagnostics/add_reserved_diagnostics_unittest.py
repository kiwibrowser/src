# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from tracing.value import histogram
from tracing.value import histogram_set
from tracing.value.diagnostics import add_reserved_diagnostics
from tracing.value.diagnostics import generic_set
from tracing.value.diagnostics import reserved_infos

class AddReservedDiagnosticsUnittest(unittest.TestCase):

  def _CreateHistogram(self, name, stories=None, tags=None, had_failures=False):
    h = histogram.Histogram(name, 'count')
    if stories:
      h.diagnostics[reserved_infos.STORIES.name] = generic_set.GenericSet(
          stories)
    if tags:
      h.diagnostics[reserved_infos.STORY_TAGS.name] = generic_set.GenericSet(
          tags)
    if had_failures:
      h.diagnostics[reserved_infos.HAD_FAILURES.name] = generic_set.GenericSet(
          [True])
    return h

  def testAddReservedDiagnostics_InvalidDiagnostic_Raises(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo')])

    with self.assertRaises(AssertionError):
      add_reserved_diagnostics.AddReservedDiagnostics(
          hs.AsDicts(), {'SOME INVALID DIAGNOSTIC': 'bar'})

  def testAddReservedDiagnostics_DiagnosticsAdded(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('bar', stories=['bar1']),
        self._CreateHistogram('bar', stories=['bar2']),
        self._CreateHistogram('blah')])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    for h in new_hs:
      self.assertIn('benchmarks', h.diagnostics)
      benchmarks = list(h.diagnostics['benchmarks'])
      self.assertEqual(['bar'], benchmarks)

  def testAddReservedDiagnostics_SummaryAddedToMerged(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('bar', stories=['bar1']),
        self._CreateHistogram('bar', stories=['bar2']),
        self._CreateHistogram('blah')])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    expected = [
        [u'foo1', [], [u'foo1']],
        [u'bar', [], [u'bar1']],
        [u'blah', [], []],
        [u'bar', [u'name'], [u'bar1', u'bar2']],
        [u'foo1', [u'name'], [u'foo1']],
        [u'bar', [], [u'bar2']],
    ]

    for h in new_hs:
      is_summary = sorted(
          list(h.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, [])))
      stories = sorted(list(h.diagnostics.get(reserved_infos.STORIES.name, [])))
      self.assertIn([h.name, is_summary, stories], expected)
      expected.remove([h.name, is_summary, stories])

    self.assertEqual(0, len(expected))

  def testAddReservedDiagnostics_Repeats_Merged(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('foo1', stories=['foo1']),
        self._CreateHistogram('foo2', stories=['foo2'])])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    expected = [
        [u'foo2', [u'name']],
        [u'foo1', [u'name']],
        [u'foo2', []],
        [u'foo1', []],
    ]

    for h in new_hs:
      is_summary = sorted(
          list(h.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, [])))
      self.assertIn([h.name, is_summary], expected)
      expected.remove([h.name, is_summary])

    self.assertEqual(0, len(expected))

  def testAddReservedDiagnostics_Stories_Merged(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo', stories=['foo1']),
        self._CreateHistogram('foo', stories=['foo2']),
        self._CreateHistogram('bar', stories=['bar'])])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    expected = [
        [u'foo', [], [u'foo2']],
        [u'foo', [u'name'], [u'foo1', u'foo2']],
        [u'bar', [u'name'], [u'bar']],
        [u'foo', [], [u'foo1']],
        [u'bar', [], [u'bar']],
    ]

    for h in new_hs:
      is_summary = sorted(
          list(h.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, [])))
      stories = sorted(list(h.diagnostics[reserved_infos.STORIES.name]))
      self.assertIn([h.name, is_summary, stories], expected)
      expected.remove([h.name, is_summary, stories])

    self.assertEqual(0, len(expected))

  def testAddReservedDiagnostics_NoStories_Unmerged(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo'),
        self._CreateHistogram('foo'),
        self._CreateHistogram('bar')])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    for h in new_hs:
      self.assertNotIn(reserved_infos.SUMMARY_KEYS.name, h.diagnostics)

    self.assertEqual(2, len(new_hs.GetHistogramsNamed('foo')))
    self.assertEqual(1, len(new_hs.GetHistogramsNamed('bar')))

  def testAddReservedDiagnostics_WithTags(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo', ['bar'], ['t:1']),
        self._CreateHistogram('foo', ['bar'], ['t:2'])
    ])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    expected = [
        [u'foo', [u'name'], [u'bar'], [u't:1', u't:2']],
        [u'foo', [], [u'bar'], [u't:1']],
        [u'foo', [], [u'bar'], [u't:2']],
        [u'foo', [u'name', u'storyTags'], [u'bar'], [u't:1']],
        [u'foo', [u'name', u'storyTags'], [u'bar'], [u't:2']],
    ]

    for h in new_hs:
      is_summary = sorted(
          list(h.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, [])))
      stories = sorted(list(h.diagnostics[reserved_infos.STORIES.name]))
      tags = sorted(list(h.diagnostics[reserved_infos.STORY_TAGS.name]))
      self.assertIn([h.name, is_summary, stories, tags], expected)
      expected.remove([h.name, is_summary, stories, tags])

    self.assertEqual(0, len(expected))

  def testAddReservedDiagnostics_WithTags_SomeIgnored(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram(
            'foo', stories=['story1'], tags=['t:1', 'ignored']),
        self._CreateHistogram(
            'foo', stories=['story1'], tags=['t:1']),
    ])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    expected = [
        [u'foo', [u'name', u'storyTags'], [u'story1'], [u'ignored', u't:1']],
        [u'foo', [], [u'story1'], [u'ignored', u't:1']],
        [u'foo', [u'name'], [u'story1'], [u'ignored', u't:1']],
    ]

    for h in new_hs:
      is_summary = sorted(
          list(h.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, [])))
      stories = sorted(list(h.diagnostics[reserved_infos.STORIES.name]))
      tags = sorted(list(h.diagnostics[reserved_infos.STORY_TAGS.name]))
      self.assertIn([h.name, is_summary, stories, tags], expected)
      expected.remove([h.name, is_summary, stories, tags])

    self.assertEqual(0, len(expected))

  def testAddReservedDiagnostics_OmitsSummariesIfHadFailures(self):
    hs = histogram_set.HistogramSet([
        self._CreateHistogram('foo', ['bar'], had_failures=True)])

    new_hs_json = add_reserved_diagnostics.AddReservedDiagnostics(
        hs.AsDicts(), {'benchmarks': 'bar'})

    new_hs = histogram_set.HistogramSet()
    new_hs.ImportDicts(json.loads(new_hs_json))

    self.assertEqual(len(new_hs), 1)

    h = new_hs.GetFirstHistogram()
    self.assertEqual(h.name, 'foo')
    self.assertNotIn(reserved_infos.SUMMARY_KEYS.name, h.diagnostics)
    self.assertNotIn(reserved_infos.HAD_FAILURES.name, h.diagnostics)
