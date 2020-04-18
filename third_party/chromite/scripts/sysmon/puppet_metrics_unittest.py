# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for puppet_metrics."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

from cStringIO import StringIO
import os

import mock

from chromite.lib import cros_test_lib
from chromite.scripts.sysmon import puppet_metrics


_SUMMARY = '''\
---
  version:
    config: 1499979608
    puppet: "3.4.3"
  resources:
    changed: 7
    failed: 0
    failed_to_restart: 0
    out_of_sync: 7
    restarted: 0
    scheduled: 0
    skipped: 1
    total: 218
  time:
    config_retrieval: 2.862796974
    cron: 0.004638468
    exec: 11.494792536
    file: 0.618018423
    file_line: 0.003589435
    filebucket: 0.000341392
    group: 0.017957332
    ini_subsetting: 0.001235189
    mount: 0.001416499
    package: 4.315027644000001
    schedule: 0.001541641
    service: 10.242378408
    total: 52.958788377
    user: 0.001673407
    vcsrepo: 23.393381029
    last_run: 1499979671
  changes:
    total: 7
  events:
    failure: 0
    success: 7
    total: 7%
'''


class TestPuppetRunSummary(cros_test_lib.TestCase):
  """Tests for _PuppetRunSummary."""

  def test_config_version(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.config_version, 1499979608)

  def test_puppet_version(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.puppet_version, '3.4.3')

  def test_events(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.events, {
        'failure': 0,
        'success': 7
    })

  def test_resources(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.resources, {
        'changed': 7,
        'failed': 0,
        'failed_to_restart': 0,
        'out_of_sync': 7,
        'restarted': 0,
        'scheduled': 0,
        'skipped': 1,
        'other': 203,
    })

  def test_times(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.times, {
        'config_retrieval': 2.862796974,
        'cron': 0.004638468,
        'exec': 11.494792536,
        'file': 0.618018423,
        'file_line': 0.003589435,
        'filebucket': 0.000341392,
        'group': 0.017957332,
        'ini_subsetting': 0.001235189,
        'mount': 0.001416499,
        'other': 0,
        'package': 4.315027644000001,
        'schedule': 0.001541641,
        'service': 10.242378408,
        'user': 0.001673407,
        'vcsrepo': 23.393381029,
    })

  def test_last_run_time(self):
    summary = puppet_metrics._PuppetRunSummary(StringIO(_SUMMARY))
    self.assertEqual(summary.last_run_time, 1499979671)


class TestPuppetMetrics(cros_test_lib.TempDirTestCase):
  """Tests for puppet_metrics."""

  def setUp(self):
    patcher = mock.patch('infra_libs.ts_mon.common.interface.state.store',
                         autospec=True)
    self.store = patcher.start()
    self.addCleanup(patcher.stop)
    self.tempfile = os.path.join(self.tempdir, 'last_run_summary.yaml')

  def test_collect(self):
    with open(self.tempfile, 'w') as f:
      f.write(_SUMMARY)
    with mock.patch('time.time', return_value=1500000000):
      with mock.patch.object(puppet_metrics, 'LAST_RUN_FILE', self.tempfile):
        puppet_metrics.collect_puppet_summary()

    setter = self.store.set
    calls = [
        mock.call('puppet/version/config', (), None,
                  1499979608, enforce_ge=mock.ANY),
        mock.call('puppet/version/puppet', (), None,
                  '3.4.3', enforce_ge=mock.ANY),
        mock.call('puppet/events', ('failure',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('puppet/events', ('success',), None,
                  7, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('scheduled',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('skipped',), None,
                  1, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('restarted',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('changed',), None,
                  7, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('failed',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('other',), None,
                  203, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('failed_to_restart',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('puppet/resources', ('out_of_sync',), None,
                  7, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('vcsrepo',), None,
                  23.393381029, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('exec',), None,
                  11.494792536, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('cron',), None,
                  0.004638468, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('file_line',), None,
                  0.003589435, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('config_retrieval',), None,
                  2.862796974, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('user',), None,
                  0.001673407, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('file',), None,
                  0.618018423, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('group',), None,
                  0.017957332, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('service',), None,
                  10.242378408, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('package',), None,
                  4.315027644000001, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('mount',), None,
                  0.001416499, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('schedule',), None,
                  0.001541641, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('other',), None,
                  0.0, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('ini_subsetting',), None,
                  0.001235189, enforce_ge=mock.ANY),
        mock.call('puppet/times', ('filebucket',), None,
                  0.000341392, enforce_ge=mock.ANY),
        mock.call('puppet/age', (), None,
                  20329.0, enforce_ge=mock.ANY),
    ]
    setter.assert_has_calls(calls)
    self.assertEqual(len(setter.mock_calls), len(calls))
