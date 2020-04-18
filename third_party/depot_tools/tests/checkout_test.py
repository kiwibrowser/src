#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for checkout.py."""

import logging
import os
import shutil
import sys
import unittest
from xml.etree import ElementTree

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(ROOT_DIR))

from testing_support import fake_repos
from testing_support.patches_data import GIT, RAW

import checkout
import patch
import subprocess2


# pass -v to enable it.
DEBUGGING = False

# A patch that will fail to apply.
BAD_PATCH = ''.join(
    [l for l in GIT.PATCH.splitlines(True) if l.strip() != 'e'])


class FakeRepos(fake_repos.FakeReposBase):
  TEST_GIT_REPO = 'repo_1'

  def populateGit(self):
    """Creates a few revisions of changes files."""
    self._commit_git(self.TEST_GIT_REPO, self._git_tree())
    # Fix for the remote rejected error. For more details see:
    # http://stackoverflow.com/questions/2816369/git-push-error-remote
    subprocess2.check_output(
        ['git', '--git-dir',
         os.path.join(self.git_root, self.TEST_GIT_REPO, '.git'),
         'config', '--bool', 'core.bare', 'true'])

    assert os.path.isdir(
        os.path.join(self.git_root, self.TEST_GIT_REPO, '.git'))

  @staticmethod
  def _git_tree():
    fs = {}
    fs['origin'] = 'git@1'
    fs['extra'] = 'dummy\n'  # new
    fs['codereview.settings'] = (
        '# Test data\n'
        'bar: pouet\n')
    fs['chrome/file.cc'] = (
        'a\n'
        'bb\n'
        'ccc\n'
        'dd\n'
        'e\n'
        'ff\n'
        'ggg\n'
        'hh\n'
        'i\n'
        'jj\n'
        'kkk\n'
        'll\n'
        'm\n'
        'nn\n'
        'ooo\n'
        'pp\n'
        'q\n')
    fs['chromeos/views/DOMui_menu_widget.h'] = (
      '// Copyright (c) 2010\n'
      '// Use of this source code\n'
      '// found in the LICENSE file.\n'
      '\n'
      '#ifndef DOM\n'
      '#define DOM\n'
      '#pragma once\n'
      '\n'
      '#include <string>\n'
      '#endif\n')
    return fs


# pylint: disable=no-self-use
class BaseTest(fake_repos.FakeReposTestBase):
  name = 'foo'
  FAKE_REPOS_CLASS = FakeRepos
  is_read_only = False

  def setUp(self):
    super(BaseTest, self).setUp()
    self._old_call = subprocess2.call
    def redirect_call(args, **kwargs):
      if not DEBUGGING:
        kwargs.setdefault('stdout', subprocess2.PIPE)
        kwargs.setdefault('stderr', subprocess2.STDOUT)
      return self._old_call(args, **kwargs)
    subprocess2.call = redirect_call
    self.usr, self.pwd = self.FAKE_REPOS.USERS[0]
    self.previous_log = None

  def tearDown(self):
    subprocess2.call = self._old_call
    super(BaseTest, self).tearDown()

  def get_patches(self):
    return patch.PatchSet([
        patch.FilePatchDiff('new_dir/subdir/new_file', GIT.NEW_SUBDIR, []),
        patch.FilePatchDiff('chrome/file.cc', GIT.PATCH, []),
        # TODO(maruel): Test with is_new == False.
        patch.FilePatchBinary('bin_file', '\x00', [], is_new=True),
        patch.FilePatchDelete('extra', False),
    ])

  def get_trunk(self, modified):
    raise NotImplementedError()

  def _check_base(self, co, root, expected):
    raise NotImplementedError()

  def _check_exception(self, co, err_msg):
    co.prepare(None)
    try:
      co.apply_patch([patch.FilePatchDiff('chrome/file.cc', BAD_PATCH, [])])
      self.fail()
    except checkout.PatchApplicationFailed, e:
      self.assertEquals(e.filename, 'chrome/file.cc')
      self.assertEquals(e.status, err_msg)

  def _log(self):
    raise NotImplementedError()

  def _test_process(self, co_lambda):
    """Makes sure the process lambda is called correctly."""
    post_processors = [lambda *args: results.append(args)]
    co = co_lambda(post_processors)
    self.assertEquals(post_processors, co.post_processors)
    co.prepare(None)
    ps = self.get_patches()
    results = []
    co.apply_patch(ps)
    expected_co = getattr(co, 'checkout', co)
    # Because of ReadOnlyCheckout.
    expected = [(expected_co, p) for p in ps.patches]
    self.assertEquals(len(expected), len(results))
    self.assertEquals(expected, results)

  def _check_move(self, co):
    """Makes sure file moves are handled correctly."""
    co.prepare(None)
    patchset = patch.PatchSet([
        patch.FilePatchDelete('chromeos/views/DOMui_menu_widget.h', False),
        patch.FilePatchDiff(
          'chromeos/views/webui_menu_widget.h', GIT.RENAME_PARTIAL, []),
    ])
    co.apply_patch(patchset)
    # Make sure chromeos/views/DOMui_menu_widget.h is deleted and
    # chromeos/views/webui_menu_widget.h is correctly created.
    root = os.path.join(self.root_dir, self.name)
    tree = self.get_trunk(False)
    del tree['chromeos/views/DOMui_menu_widget.h']
    tree['chromeos/views/webui_menu_widget.h'] = (
        '// Copyright (c) 2011\n'
        '// Use of this source code\n'
        '// found in the LICENSE file.\n'
        '\n'
        '#ifndef WEB\n'
        '#define WEB\n'
        '#pragma once\n'
        '\n'
        '#include <string>\n'
        '#endif\n')
    #print patchset[0].get()
    #print fake_repos.read_tree(root)
    self.assertTree(tree, root)


class GitBaseTest(BaseTest):
  def setUp(self):
    super(GitBaseTest, self).setUp()
    self.enabled = self.FAKE_REPOS.set_up_git()
    self.assertTrue(self.enabled)
    self.previous_log = self._log()

  # pylint: disable=arguments-differ
  def _log(self, log_from_local_repo=False):
    if log_from_local_repo:
      repo_root = os.path.join(self.root_dir, self.name)
    else:
      repo_root = os.path.join(self.FAKE_REPOS.git_root,
                               self.FAKE_REPOS.TEST_GIT_REPO)
    out = subprocess2.check_output(
        ['git',
         '--git-dir',
         os.path.join(repo_root, '.git'),
         'log', '--pretty=format:"%H%x09%ae%x09%ad%x09%s"',
         '--max-count=1']).strip('"')
    if out and len(out.split()) != 0:
      revision = out.split()[0]
    else:
      return {'revision': 0}

    return {
        'revision': revision,
        'author': out.split()[1],
        'msg': out.split()[-1],
    }

  def _check_base(self, co, root, expected):
    read_only = isinstance(co, checkout.ReadOnlyCheckout)
    self.assertEquals(read_only, self.is_read_only)
    if not read_only:
      self.FAKE_REPOS.git_dirty = True

    self.assertEquals(root, co.project_path)
    git_rev = co.prepare(None)
    self.assertEquals(unicode, type(git_rev))
    self.assertEquals(self.previous_log['revision'], git_rev)
    self.assertEquals('pouet', co.get_settings('bar'))
    self.assertTree(self.get_trunk(False), root)
    patches = self.get_patches()
    co.apply_patch(patches)
    self.assertEquals(
        ['bin_file', 'chrome/file.cc', 'new_dir/subdir/new_file', 'extra'],
        patches.filenames)

    # Hackish to verify _branches() internal function.
    # pylint: disable=protected-access
    self.assertEquals(
        (['master', 'working_branch'], 'working_branch'),
        co._branches())

    # Verify that the patch is applied even for read only checkout.
    self.assertTree(self.get_trunk(True), root)
    fake_author = self.FAKE_REPOS.USERS[1][0]
    revision = co.commit(u'msg', fake_author)
    # Nothing changed.
    self.assertTree(self.get_trunk(True), root)

    if read_only:
      self.assertEquals('FAKE', revision)
      self.assertEquals(self.previous_log['revision'], co.prepare(None))
      # Changes should be reverted now.
      self.assertTree(self.get_trunk(False), root)
      expected = self.previous_log
    else:
      self.assertEquals(self._log()['revision'], revision)
      self.assertEquals(self._log()['revision'], co.prepare(None))
      self.assertTree(self.get_trunk(True), root)
      expected = self._log()

    actual = self._log(log_from_local_repo=True)
    self.assertEquals(expected, actual)

  def get_trunk(self, modified):
    tree = {}
    for k, v in self.FAKE_REPOS.git_hashes[
        self.FAKE_REPOS.TEST_GIT_REPO][1][1].iteritems():
      assert k not in tree
      tree[k] = v

    if modified:
      content_lines = tree['chrome/file.cc'].splitlines(True)
      tree['chrome/file.cc'] = ''.join(
          content_lines[0:5] + ['FOO!\n'] + content_lines[5:])
      tree['bin_file'] = '\x00'
      del tree['extra']
      tree['new_dir/subdir/new_file'] = 'A new file\nshould exist.\n'
    return tree

  def _test_prepare(self, co):
    print co.prepare(None)


class GitCheckout(GitBaseTest):
  def _get_co(self, post_processors):
    self.assertNotEqual(False, post_processors)
    return checkout.GitCheckout(
      root_dir=self.root_dir,
      project_name=self.name,
      remote_branch='master',
      git_url=os.path.join(self.FAKE_REPOS.git_root,
                           self.FAKE_REPOS.TEST_GIT_REPO),
      commit_user=self.usr,
      post_processors=post_processors)

  def testAll(self):
    root = os.path.join(self.root_dir, self.name)
    self._check_base(self._get_co(None), root, None)

  @unittest.skip('flaky')
  def testException(self):
    self._check_exception(
        self._get_co(None),
        'While running git apply --index -3 -p1;\n  fatal: corrupt patch at '
        'line 12\n')

  def testProcess(self):
    self._test_process(self._get_co)

  def _testPrepare(self):
    self._test_prepare(self._get_co(None))

  def testMove(self):
    co = self._get_co(None)
    self._check_move(co)
    out = subprocess2.check_output(
        ['git', 'diff', '--staged', '--name-status', '--no-renames'],
        cwd=co.project_path)
    out = sorted(out.splitlines())
    expected = sorted(
      [
        'A\tchromeos/views/webui_menu_widget.h',
        'D\tchromeos/views/DOMui_menu_widget.h',
      ])
    self.assertEquals(expected, out)


if __name__ == '__main__':
  if '-v' in sys.argv:
    DEBUGGING = True
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(levelname)5s %(filename)15s(%(lineno)3d): %(message)s')
  else:
    logging.basicConfig(
        level=logging.ERROR,
        format='%(levelname)5s %(filename)15s(%(lineno)3d): %(message)s')
  unittest.main()
