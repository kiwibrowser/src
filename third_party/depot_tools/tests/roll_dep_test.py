#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys
import subprocess
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)

from testing_support import fake_repos

ROLL_DEP = os.path.join(ROOT_DIR, 'roll-dep')
GCLIENT = os.path.join(ROOT_DIR, 'gclient')


class FakeRepos(fake_repos.FakeReposBase):
  NB_GIT_REPOS = 2

  def populateGit(self):
    self._commit_git('repo_2', {
        'origin': 'git/repo_2@1',
    })
    self._commit_git('repo_2', {
        'origin': 'git/repo_2@2',
    })
    self._commit_git('repo_2', {
        'origin': 'git/repo_2@3',
    })

    self._commit_git('repo_1', {
        'DEPS': '\n'.join([
            'deps = {',
            ' "src/foo": "%(git_base)srepo_2@%(repo_2_revision)s",',
            '}',
        ]) % {
            'git_base': self.git_base,
            'repo_2_revision': self.git_hashes['repo_2'][1][0],
        },
    })


class RollDepTest(fake_repos.FakeReposTestBase):
  FAKE_REPOS_CLASS = FakeRepos

  def setUp(self):
    super(RollDepTest, self).setUp()
    # Make sure it doesn't try to auto update when testing!
    self.env = os.environ.copy()
    self.env['DEPOT_TOOLS_UPDATE'] = '0'

    self.enabled = self.FAKE_REPOS.set_up_git()
    self.src_dir = os.path.join(self.root_dir, 'src')
    self.foo_dir = os.path.join(self.src_dir, 'foo')
    if self.enabled:
      self.call([GCLIENT, 'config', self.git_base + 'repo_1', '--name', 'src'],
                cwd=self.root_dir)
      self.call([GCLIENT, 'sync'], cwd=self.root_dir)

  def call(self, cmd, cwd=None):
    cwd = cwd or self.src_dir
    process = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               shell=sys.platform.startswith('win'))
    stdout, stderr = process.communicate()
    logging.debug("XXX: %s\n%s\nXXX" % (' '.join(cmd), stdout))
    logging.debug("YYY: %s\n%s\nYYY" % (' '.join(cmd), stderr))
    return (stdout.replace('\r\n', '\n'), stderr.replace('\r\n', '\n'),
            process.returncode)

  def testRollsDep(self):
    if not self.enabled:
      return
    stdout = self.call([ROLL_DEP, 'src/foo'])[0]
    expected_revision = self.githash('repo_2', 3)

    with open(os.path.join(self.src_dir, 'DEPS')) as f:
      contents = f.read()

    self.assertEqual(self.gitrevparse(self.foo_dir), expected_revision)
    self.assertEqual([
        'deps = {',
        ' "src/foo": "' + self.git_base + 'repo_2@' + expected_revision + '",',
        '}',
    ], contents.splitlines())

    commit_message = self.call(['git', 'log', '-n', '1'])[0]

    expected_message = 'Roll src/foo/ %s..%s (2 commits)' % (
        self.githash('repo_2', 1)[:9], self.githash('repo_2', 3)[:9])

    self.assertIn(expected_message, stdout)
    self.assertIn(expected_message, commit_message)

  def testRollsDepToSpecificRevision(self):
    if not self.enabled:
      return
    stdout = self.call([ROLL_DEP, 'src/foo',
                        '--roll-to', self.githash('repo_2', 2)])[0]
    expected_revision = self.githash('repo_2', 2)

    with open(os.path.join(self.src_dir, 'DEPS')) as f:
      contents = f.read()

    self.assertEqual(self.gitrevparse(self.foo_dir), expected_revision)
    self.assertEqual([
        'deps = {',
        ' "src/foo": "' + self.git_base + 'repo_2@' + expected_revision + '",',
        '}',
    ], contents.splitlines())

    commit_message = self.call(['git', 'log', '-n', '1'])[0]

    expected_message = 'Roll src/foo/ %s..%s (1 commit)' % (
        self.githash('repo_2', 1)[:9], self.githash('repo_2', 2)[:9])

    self.assertIn(expected_message, stdout)
    self.assertIn(expected_message, commit_message)


if __name__ == '__main__':
  level = logging.DEBUG if '-v' in sys.argv else logging.FATAL
  logging.basicConfig(
      level=level,
      format='%(asctime).19s %(levelname)s %(filename)s:'
             '%(lineno)s %(message)s')
  unittest.main()
