# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Git repo metrics."""

from __future__ import absolute_import
from __future__ import print_function

import os
import subprocess

from chromite.lib import cros_logging as logging
from chromite.lib import metrics

logger = logging.getLogger(__name__)


class _GitRepo(object):
  """Helper class for running git commands."""

  def __init__(self, gitdir):
    self._gitdir = gitdir

  def _get_git_command(self):
    return ['git', '--git-dir', self._gitdir]

  def _check_output(self, args, **kwargs):
    return subprocess.check_output(
        self._get_git_command() + list(args), **kwargs)

  def get_commit_hash(self):
    """Return commit hash string."""
    return self._check_output(['rev-parse', 'HEAD']).strip()

  def get_commit_time(self):
    """Return commit time as UNIX timestamp int."""
    return int(self._check_output(['show', '-s', '--format=%ct', 'HEAD'])
               .strip())

  def get_unstaged_changes(self):
    """Return number of unstaged changes as (added, deleted)."""
    added_total, deleted_total = 0, 0
    # output looks like:
    # '1\t2\tfoo\n3\t4\tbar\n'
    # '-\t-\tbinary_file\n'
    output = self._check_output(['diff-index', '--numstat', 'HEAD'])
    stats_strings = (line.split() for line in output.splitlines())
    for added, deleted, _path in stats_strings:
      if added != '-':
        added_total += int(added)
      if deleted != '-':
        deleted_total += int(deleted)
    return added_total, deleted_total


class _GitMetricCollector(object):
  """Class for collecting metrics about a git repository.

  The constructor takes the arguments: `gitdir`, `metric_path`.
  `gitdir` is the path to the Git directory to collect metrics for and
  may start with a tilde (expanded to a user's home directory).
  `metric_path` is the Monarch metric path to report to.
  """

  _commit_hash_metric = metrics.StringMetric(
      'git/hash',
      description='Current Git commit hash.')

  _timestamp_metric = metrics.GaugeMetric(
      'git/timestamp',
      description='Current Git commit time as seconds since Unix Epoch.')

  _unstaged_changes_metric = metrics.GaugeMetric(
      'git/unstaged_changes',
      description='Unstaged Git changes.')

  def __init__(self, gitdir, metric_path):
    self._gitdir = gitdir
    self._gitrepo = _GitRepo(os.path.expanduser(gitdir))
    self._fields = {'repo': gitdir}
    self._metric_path = metric_path

  def collect(self):
    """Collect metrics."""
    try:
      self._collect_commit_hash_metric()
      self._collect_timestamp_metric()
      self._collect_unstaged_changes_metric()
    except subprocess.CalledProcessError as e:
      logger.warning(u'Error collecting git metrics for %s: %s',
                     self._gitdir, e)

  def _collect_commit_hash_metric(self):
    commit_hash = self._gitrepo.get_commit_hash()
    logger.debug(u'Collecting Git hash %r for %r', commit_hash, self._gitdir)
    self._commit_hash_metric.set(commit_hash, self._fields)

  def _collect_timestamp_metric(self):
    commit_time = self._gitrepo.get_commit_time()
    logger.debug(u'Collecting Git timestamp %r for %r',
                 commit_time, self._gitdir)
    self._timestamp_metric.set(commit_time, self._fields)

  def _collect_unstaged_changes_metric(self):
    added, deleted = self._gitrepo.get_unstaged_changes()
    self._unstaged_changes_metric.set(
        added, fields=dict(change_type='added', **self._fields))
    self._unstaged_changes_metric.set(
        deleted, fields=dict(change_type='deleted', **self._fields))


_CHROMIUMOS_DIR = '~chromeos-test/chromiumos/'

_repo_collectors = (
    # TODO(ayatane): We cannot access chromeos-admin because we are
    # running as non-root.
    _GitMetricCollector(gitdir='/root/chromeos-admin/.git',
                        metric_path='chromeos-admin'),
    _GitMetricCollector(gitdir=_CHROMIUMOS_DIR + 'chromite/.git',
                        metric_path='chromite'),
    _GitMetricCollector(gitdir='/usr/local/autotest/.git',
                        metric_path='installed_autotest'),
)


def collect_git_metrics():
  """Collect metrics for Git repository state."""
  for collector in _repo_collectors:
    collector.collect()
