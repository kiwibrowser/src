#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for git_drover."""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testing_support import auto_stub
import git_drover


class GitDroverTest(auto_stub.TestCase):

  def setUp(self):
    super(GitDroverTest, self).setUp()
    self._temp_directory = tempfile.mkdtemp()
    self._parent_repo = os.path.join(self._temp_directory, 'parent_repo')
    self._target_repo = os.path.join(self._temp_directory, 'drover_branch_123')
    os.makedirs(os.path.join(self._parent_repo, '.git'))
    with open(os.path.join(self._parent_repo, '.git', 'config'), 'w') as f:
      f.write('config')
    with open(os.path.join(self._parent_repo, '.git', 'HEAD'), 'w') as f:
      f.write('HEAD')
    os.mkdir(os.path.join(self._parent_repo, '.git', 'info'))
    with open(
        os.path.join(self._parent_repo, '.git', 'info', 'refs'), 'w') as f:
      f.write('refs')
    self.mock(tempfile, 'mkdtemp', self._mkdtemp)
    self.mock(__builtins__, 'raw_input', self._get_input)
    self.mock(subprocess, 'check_call', self._check_call)
    self.mock(subprocess, 'check_output', self._check_call)
    self.real_popen = subprocess.Popen
    self.mock(subprocess, 'Popen', self._Popen)
    self._commands = []
    self._input = []
    self._fail_on_command = None
    self._reviewers = ''

    self.REPO_CHECK_COMMANDS = [
        (['git', '--help'], self._parent_repo),
        (['git', 'status'], self._parent_repo),
        (['git', 'fetch', 'origin'], self._parent_repo),
        (['git', 'rev-parse', 'refs/remotes/branch-heads/branch^{commit}'],
         self._parent_repo),
        (['git', 'rev-parse', 'cl^{commit}'], self._parent_repo),
        (['git', 'show', '-s', 'cl'], self._parent_repo),
    ]
    self.LOCAL_REPO_COMMANDS = [
        (['git', 'rev-parse', '--git-dir'], self._parent_repo),
        (['git', 'config', 'core.sparsecheckout', 'true'], self._target_repo),
        (['git', 'checkout', '-b', 'drover_branch_123',
          'refs/remotes/branch-heads/branch'], self._target_repo),
        (['git', 'cherry-pick', '-x', 'cl'], self._target_repo),
    ]
    self.UPLOAD_COMMANDS = [
        (['git', 'reset', '--hard'], self._target_repo),
        (['git', 'log', '-1', '--format=%ae'], self._target_repo),
        (['git', 'cl', 'upload', '--send-mail', '--tbrs', 'author@domain.org'],
         self._target_repo),
    ]
    self.LAND_COMMAND = [
        (['git', 'cl', 'land', '--bypass-hooks'], self._target_repo),
    ]
    if os.name == 'nt':
      self.BRANCH_CLEANUP_COMMANDS = [
          (['rmdir', '/s', '/q', self._target_repo], None),
          (['git', 'branch', '-D', 'drover_branch_123'], self._parent_repo),
      ]
    else:
      self.BRANCH_CLEANUP_COMMANDS = [
          (['git', 'branch', '-D', 'drover_branch_123'], self._parent_repo),
      ]
    self.MANUAL_RESOLVE_PREPARATION_COMMANDS = [
        (['git', '-c', 'core.quotePath=false', 'status', '--porcelain'],
         self._target_repo),
        (['git', 'update-index', '--skip-worktree', '--stdin'],
         self._target_repo),
    ]
    self.FINISH_MANUAL_RESOLVE_COMMANDS = [
        (['git', 'commit', '--no-edit'], self._target_repo),
    ]

  def tearDown(self):
    shutil.rmtree(self._temp_directory)
    super(GitDroverTest, self).tearDown()

  def _mkdtemp(self, prefix='tmp'):
    self.assertEqual('drover_branch_', prefix)
    os.mkdir(self._target_repo)
    return self._target_repo

  def _get_input(self, message):
    result = self._input.pop(0)
    if result == 'EOF':
      raise EOFError
    return result

  def _check_call(self, args, stderr=None, stdout=None, shell='', cwd=None):
    if args[0] == 'rmdir':
      subprocess.call(args, shell=shell)
    else:
      self.assertFalse(shell)
    self._commands.append((args, cwd))
    if (self._fail_on_command is not None and
        self._fail_on_command == len(self._commands)):
      self._fail_on_command = None
      raise subprocess.CalledProcessError(1, args[0])
    if args == ['git', 'rev-parse', '--git-dir']:
      return os.path.join(self._parent_repo, '.git')
    if args == ['git', '-c', 'core.quotePath=false', 'status', '--porcelain']:
      return ' D foo\nUU baz\n D bar\n'
    if args == ['git', 'log', '-1', '--format=%ae']:
      return 'author@domain.org'
    return ''

  def _Popen(self, args, shell=False, cwd=None, stdin=None, stdout=None,
             stderr=None):
    if args == ['git', 'update-index', '--skip-worktree', '--stdin']:
      self._commands.append((args, cwd))
      self.assertFalse(shell)
      self.assertEqual(stdin, subprocess.PIPE)
      class MockPopen(object):
        def __init__(self, *args, **kwargs):
          self.returncode = -999
        def communicate(self, stdin):
          if stdin == 'foo\nbar\n':
            self.returncode = 0
          else:
            self.returncode = 1
      return MockPopen()
    else:
      return self.real_popen(args, shell=shell, cwd=cwd, stdin=stdin,
                             stdout=stdout, stderr=stderr)

  def testSuccess(self):
    self._input = ['y', 'y']
    git_drover.cherry_pick_change('branch', 'cl', self._parent_repo, False)
    self.assertEqual(
        self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS +
        self.UPLOAD_COMMANDS + self.LAND_COMMAND + self.BRANCH_CLEANUP_COMMANDS,
        self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testDryRun(self):
    self._input = ['y']
    git_drover.cherry_pick_change('branch', 'cl', self._parent_repo, True)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS +
                     self.BRANCH_CLEANUP_COMMANDS, self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testCancelEarly(self):
    self._input = ['n']
    git_drover.cherry_pick_change('branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS, self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testEOFOnConfirm(self):
    self._input = ['EOF']
    git_drover.cherry_pick_change('branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS, self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testCancelLate(self):
    self._input = ['y', 'n']
    git_drover.cherry_pick_change('branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS +
                     self.UPLOAD_COMMANDS + self.BRANCH_CLEANUP_COMMANDS,
                     self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailDuringCheck(self):
    self._input = []
    self._fail_on_command = 1
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS[:1], self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailDuringBranchCreation(self):
    self._input = ['y']
    self._fail_on_command = 8
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS[:2] +
                     self.BRANCH_CLEANUP_COMMANDS[:-1], self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailDuringCherryPickAndAbort(self):
    self._input = ['y']
    self._fail_on_command = 10
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS[:4] +
                     self.MANUAL_RESOLVE_PREPARATION_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))
    self.assertFalse(self._input)
    self._commands = []
    git_drover.abort_cherry_pick(self._target_repo)
    self.assertEqual(self.BRANCH_CLEANUP_COMMANDS, self._commands)
    self.assertFalse(os.path.exists(self._target_repo))

  def testFailDuringCherryPickAndContinue(self):
    self._input = ['y']
    self._fail_on_command = 10
    self.assertRaises(git_drover.PatchError, git_drover.cherry_pick_change,
                      'branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS[:4] +
                     self.MANUAL_RESOLVE_PREPARATION_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

    self._commands = []
    self._input = ['n']
    git_drover.continue_cherry_pick(self._target_repo)
    self.assertEqual(self.UPLOAD_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

    self._commands = []
    self._input = ['y']
    git_drover.continue_cherry_pick(self._target_repo)
    self.assertEqual(
        self.UPLOAD_COMMANDS + self.LAND_COMMAND + self.BRANCH_CLEANUP_COMMANDS,
        self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailDuringCherryPickAndContinueWithoutCommitting(self):
    self._input = ['y']
    self._fail_on_command = 10
    self.assertRaises(git_drover.PatchError, git_drover.cherry_pick_change,
                      'branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS[:4] +
                     self.MANUAL_RESOLVE_PREPARATION_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))
    self.assertFalse(self._input)
    self._commands = []
    with open(os.path.join(self._target_repo, '.git', 'CHERRY_PICK_HEAD'), 'w'):
      pass

    self._commands = []
    self._input = ['y']
    git_drover.continue_cherry_pick(self._target_repo)
    self.assertEqual(self.FINISH_MANUAL_RESOLVE_COMMANDS + self.UPLOAD_COMMANDS
                     + self.LAND_COMMAND + self.BRANCH_CLEANUP_COMMANDS,
                     self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailDuringCherryPickAndContinueWithoutResolving(self):
    self._input = ['y']
    self._fail_on_command = 10
    self.assertRaises(git_drover.PatchError, git_drover.cherry_pick_change,
                      'branch', 'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS[:4] +
                     self.MANUAL_RESOLVE_PREPARATION_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))
    self.assertFalse(self._input)
    self._commands = []
    self._fail_on_command = 1
    with open(os.path.join(self._target_repo, '.git', 'CHERRY_PICK_HEAD'), 'w'):
      pass
    self.assertRaisesRegexp(git_drover.Error,
                            'All conflicts must be resolved before continuing',
                            git_drover.continue_cherry_pick, self._target_repo)
    self.assertEqual(self.FINISH_MANUAL_RESOLVE_COMMANDS, self._commands)
    self.assertTrue(os.path.exists(self._target_repo))

    self._commands = []
    git_drover.abort_cherry_pick(self._target_repo)
    self.assertEqual(self.BRANCH_CLEANUP_COMMANDS, self._commands)
    self.assertFalse(os.path.exists(self._target_repo))

  def testFailAfterCherryPick(self):
    self._input = ['y']
    self._fail_on_command = 11
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS +
                     self.UPLOAD_COMMANDS[:1] + self.BRANCH_CLEANUP_COMMANDS,
                     self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testFailOnUpload(self):
    self._input = ['y']
    self._fail_on_command = 13
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', self._parent_repo, False)
    self.assertEqual(self.REPO_CHECK_COMMANDS + self.LOCAL_REPO_COMMANDS +
                     self.UPLOAD_COMMANDS + self.BRANCH_CLEANUP_COMMANDS,
                     self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testInvalidParentRepoDirectory(self):
    self.assertRaises(git_drover.Error, git_drover.cherry_pick_change, 'branch',
                      'cl', os.path.join(self._parent_repo, 'fake'), False)
    self.assertFalse(self._commands)
    self.assertFalse(os.path.exists(self._target_repo))
    self.assertFalse(self._input)

  def testContinueInvalidWorkdir(self):
    self.assertRaises(git_drover.Error, git_drover.continue_cherry_pick,
                      self._parent_repo)

  def testAbortInvalidWorkdir(self):
    self.assertRaises(git_drover.Error, git_drover.abort_cherry_pick,
                      self._parent_repo)


if __name__ == '__main__':
  unittest.main()
