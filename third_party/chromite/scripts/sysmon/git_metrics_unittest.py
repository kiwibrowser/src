# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for git_metrics."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

import mock
import os
import subprocess

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.scripts.sysmon import git_metrics


class TestGitMetricCollector(cros_test_lib.TestCase):
  """Tests for _GitMetricCollector."""

  def setUp(self):
    patcher = mock.patch('infra_libs.ts_mon.common.interface.state.store',
                         autospec=True)
    self.store = patcher.start()
    self.addCleanup(patcher.stop)

  def test_collect(self):
    with mock.patch.object(git_metrics, '_GitRepo', autospec=True) as _GitRepo:
      instance = _GitRepo('dummy')
      instance.get_commit_hash.return_value = \
          '2b1ce059425edc91e013c260e59019195f927a07'
      instance.get_commit_time.return_value = 1483257600
      instance.get_unstaged_changes.return_value = (0, 3)

      collector = git_metrics._GitMetricCollector('~/solciel', 'dummy')
      collector.collect()

    setter = self.store.set
    calls = [
        mock.call('git/hash', ('~/solciel',), None,
                  '2b1ce059425edc91e013c260e59019195f927a07',
                  enforce_ge=mock.ANY),
        mock.call('git/timestamp', ('~/solciel',), None,
                  1483257600, enforce_ge=mock.ANY),
        mock.call('git/unstaged_changes',
                  ('added', '~/solciel'), None,
                  0, enforce_ge=mock.ANY),
        mock.call('git/unstaged_changes',
                  ('deleted', '~/solciel'), None,
                  3, enforce_ge=mock.ANY),
    ]
    setter.assert_has_calls(calls)
    self.assertEqual(len(setter.mock_calls), len(calls))


class TestGitRepoWithTempdir(cros_test_lib.TempDirTestCase):
  """Tests for _GitRepo using a Git fixture."""

  def setUp(self):
    self.git_dir = os.path.join(self.tempdir, '.git')

    devnull = open(os.devnull, 'w')
    self.addCleanup(devnull.close)

    def call(args, **kwargs):
      subprocess.check_call(args, stdout=devnull, stderr=devnull, **kwargs)

    with osutils.ChdirContext(self.tempdir):
      call(['git', 'init'])
      call(['git', 'config', 'user.name', 'John Doe'])
      call(['git', 'config', 'user.email', 'john@example.com'])
      with open('foo', 'w') as f:
        f.write('a\nb\nc\n')
      call(['git', 'add', 'foo'])
      env = os.environ.copy()
      env['GIT_AUTHOR_DATE'] = '2017-01-01T00:00:00Z'
      env['GIT_COMMITTER_DATE'] = '2017-01-01T00:00:00Z'
      call(['git', 'commit', '-m', 'Initial commit'], env=env)

  def test_get_commit_hash(self):
    """Test get_commit_hash()."""
    repo = git_metrics._GitRepo(self.git_dir)
    got = repo.get_commit_hash()
    self.assertEqual(got, '7c88f131e520e8455e2403b88ff4f723758c5dd6')

  def test_get_commit_time(self):
    """Test get_commit_time()."""
    repo = git_metrics._GitRepo(self.git_dir)
    got = repo.get_commit_time()
    self.assertEqual(got, 1483228800)

  def test_get_unstaged_changes(self):
    """Test get_unstaged_changes()."""
    with open(os.path.join(self.tempdir, 'spam'), 'w') as f:
      f.write('a\n')
    os.remove(os.path.join(self.tempdir, 'foo'))
    repo = git_metrics._GitRepo(self.git_dir)
    added, removed = repo.get_unstaged_changes()
    self.assertEqual(added, 0)  # Does not count untracked files
    self.assertEqual(removed, 3)
