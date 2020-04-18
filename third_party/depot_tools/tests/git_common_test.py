#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for git_common.py"""

import binascii
import collections
import datetime
import os
import shutil
import signal
import sys
import tempfile
import time
import unittest

DEPOT_TOOLS_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, DEPOT_TOOLS_ROOT)

from testing_support import coverage_utils
from testing_support import git_test_utils

GitRepo = git_test_utils.GitRepo


class GitCommonTestBase(unittest.TestCase):
  @classmethod
  def setUpClass(cls):
    super(GitCommonTestBase, cls).setUpClass()
    import git_common
    cls.gc = git_common
    cls.gc.TEST_MODE = True


class Support(GitCommonTestBase):
  def _testMemoizeOneBody(self, threadsafe):
    calls = collections.defaultdict(int)
    def double_if_even(val):
      calls[val] += 1
      return val * 2 if val % 2 == 0 else None
    # Use this explicitly as a wrapper fn instead of a decorator. Otherwise
    # pylint crashes (!!)
    double_if_even = self.gc.memoize_one(threadsafe=threadsafe)(double_if_even)

    self.assertEqual(4, double_if_even(2))
    self.assertEqual(4, double_if_even(2))
    self.assertEqual(None, double_if_even(1))
    self.assertEqual(None, double_if_even(1))
    self.assertDictEqual({1: 2, 2: 1}, calls)

    double_if_even.set(10, 20)
    self.assertEqual(20, double_if_even(10))
    self.assertDictEqual({1: 2, 2: 1}, calls)

    double_if_even.clear()
    self.assertEqual(4, double_if_even(2))
    self.assertEqual(4, double_if_even(2))
    self.assertEqual(None, double_if_even(1))
    self.assertEqual(None, double_if_even(1))
    self.assertEqual(20, double_if_even(10))
    self.assertDictEqual({1: 4, 2: 2, 10: 1}, calls)

  def testMemoizeOne(self):
    self._testMemoizeOneBody(threadsafe=False)

  def testMemoizeOneThreadsafe(self):
    self._testMemoizeOneBody(threadsafe=True)

  def testOnce(self):
    testlist = []

    # This works around a bug in pylint
    once = self.gc.once

    @once
    def add_to_list():
      testlist.append('dog')

    add_to_list()
    add_to_list()
    add_to_list()
    add_to_list()

    self.assertEquals(testlist, ['dog'])


def slow_square(i):
  """Helper for ScopedPoolTest.

  Must be global because non top-level functions aren't pickleable.
  """
  return i ** 2


class ScopedPoolTest(GitCommonTestBase):
  CTRL_C = signal.CTRL_C_EVENT if sys.platform == 'win32' else signal.SIGINT

  def testThreads(self):
    result = []
    with self.gc.ScopedPool(kind='threads') as pool:
      result = list(pool.imap(slow_square, xrange(10)))
    self.assertEqual([0, 1, 4, 9, 16, 25, 36, 49, 64, 81], result)

  def testThreadsCtrlC(self):
    result = []
    with self.assertRaises(KeyboardInterrupt):
      with self.gc.ScopedPool(kind='threads') as pool:
        # Make sure this pool is interrupted in mid-swing
        for i in pool.imap(slow_square, xrange(20)):
          if i > 32:
            os.kill(os.getpid(), self.CTRL_C)
          result.append(i)
    self.assertEqual([0, 1, 4, 9, 16, 25], result)

  def testProcs(self):
    result = []
    with self.gc.ScopedPool() as pool:
      result = list(pool.imap(slow_square, xrange(10)))
    self.assertEqual([0, 1, 4, 9, 16, 25, 36, 49, 64, 81], result)

  def testProcsCtrlC(self):
    result = []
    with self.assertRaises(KeyboardInterrupt):
      with self.gc.ScopedPool() as pool:
        # Make sure this pool is interrupted in mid-swing
        for i in pool.imap(slow_square, xrange(20)):
          if i > 32:
            os.kill(os.getpid(), self.CTRL_C)
          result.append(i)
    self.assertEqual([0, 1, 4, 9, 16, 25], result)


class ProgressPrinterTest(GitCommonTestBase):
  class FakeStream(object):
    def __init__(self):
      self.data = set()
      self.count = 0

    def write(self, line):
      self.data.add(line)

    def flush(self):
      self.count += 1

  @unittest.expectedFailure
  def testBasic(self):
    """This test is probably racy, but I don't have a better alternative."""
    fmt = '%(count)d/10'
    stream = self.FakeStream()

    pp = self.gc.ProgressPrinter(fmt, enabled=True, fout=stream, period=0.01)
    with pp as inc:
      for _ in xrange(10):
        time.sleep(0.02)
        inc()

    filtered = {x.strip() for x in stream.data}
    rslt = {fmt % {'count': i} for i in xrange(11)}
    self.assertSetEqual(filtered, rslt)
    self.assertGreaterEqual(stream.count, 10)


class GitReadOnlyFunctionsTest(git_test_utils.GitRepoReadOnlyTestBase,
                               GitCommonTestBase):
  REPO_SCHEMA = """
  A B C D
    B E D
  """

  COMMIT_A = {
    'some/files/file1': {'data': 'file1'},
    'some/files/file2': {'data': 'file2'},
    'some/files/file3': {'data': 'file3'},
    'some/other/file':  {'data': 'otherfile'},
  }

  COMMIT_C = {
    'some/files/file2': {
      'mode': 0o755,
      'data': 'file2 - vanilla\n'},
  }

  COMMIT_E = {
    'some/files/file2': {'data': 'file2 - merged\n'},
  }

  COMMIT_D = {
    'some/files/file2': {'data': 'file2 - vanilla\nfile2 - merged\n'},
  }

  def testHashes(self):
    ret = self.repo.run(
      self.gc.hash_multi, *[
        'master',
        'master~3',
        self.repo['E']+'~',
        self.repo['D']+'^2',
        'tag_C^{}',
      ]
    )
    self.assertEqual([
      self.repo['D'],
      self.repo['A'],
      self.repo['B'],
      self.repo['E'],
      self.repo['C'],
    ], ret)
    self.assertEquals(
      self.repo.run(self.gc.hash_one, 'branch_D'),
      self.repo['D']
    )
    self.assertTrue(self.repo['D'].startswith(
        self.repo.run(self.gc.hash_one, 'branch_D', short=True)))

  def testStream(self):
    items = set(self.repo.commit_map.itervalues())

    def testfn():
      for line in self.gc.run_stream('log', '--format=%H').xreadlines():
        line = line.strip()
        self.assertIn(line, items)
        items.remove(line)

    self.repo.run(testfn)

  def testStreamWithRetcode(self):
    items = set(self.repo.commit_map.itervalues())

    def testfn():
      with self.gc.run_stream_with_retcode('log', '--format=%H') as stdout:
        for line in stdout.xreadlines():
          line = line.strip()
          self.assertIn(line, items)
          items.remove(line)

    self.repo.run(testfn)

  def testStreamWithRetcodeException(self):
    import subprocess2
    with self.assertRaises(subprocess2.CalledProcessError):
      with self.gc.run_stream_with_retcode('checkout', 'unknown-branch'):
        pass

  def testCurrentBranch(self):
    def cur_branch_out_of_git():
      os.chdir('..')
      return self.gc.current_branch()
    self.assertIsNone(self.repo.run(cur_branch_out_of_git))

    self.repo.git('checkout', 'branch_D')
    self.assertEqual(self.repo.run(self.gc.current_branch), 'branch_D')

  def testBranches(self):
    # This check fails with git 2.4 (see crbug.com/487172)
    self.assertEqual(self.repo.run(set, self.gc.branches()),
                     {'master', 'branch_D', 'root_A'})

  def testDiff(self):
    # Get the names of the blobs being compared (to avoid hard-coding).
    c_blob_short = self.repo.git('rev-parse', '--short',
                                 'tag_C:some/files/file2').stdout.strip()
    d_blob_short = self.repo.git('rev-parse', '--short',
                                 'tag_D:some/files/file2').stdout.strip()
    expected_output = [
        'diff --git a/some/files/file2 b/some/files/file2',
        'index %s..%s 100755' % (c_blob_short, d_blob_short),
        '--- a/some/files/file2',
        '+++ b/some/files/file2',
        '@@ -1 +1,2 @@',
        ' file2 - vanilla',
        '+file2 - merged']
    self.assertEqual(expected_output,
                     self.repo.run(self.gc.diff, 'tag_C', 'tag_D').split('\n'))

  def testDormant(self):
    self.assertFalse(self.repo.run(self.gc.is_dormant, 'master'))
    self.repo.git('config', 'branch.master.dormant', 'true')
    self.assertTrue(self.repo.run(self.gc.is_dormant, 'master'))

  def testBlame(self):
    def get_porcelain_for_commit(commit_name, lines):
      format_string = ('%H {}\nauthor %an\nauthor-mail <%ae>\nauthor-time %at\n'
                       'author-tz +0000\ncommitter %cn\ncommitter-mail <%ce>\n'
                       'committer-time %ct\ncommitter-tz +0000\nsummary {}')
      format_string = format_string.format(lines, commit_name)
      info = self.repo.show_commit(commit_name, format_string=format_string)
      return info.split('\n')

    # Expect to blame line 1 on C, line 2 on E.
    ABBREV_LEN = 7
    c_short = self.repo['C'][:1 + ABBREV_LEN]
    c_author = self.repo.show_commit('C', format_string='%an %ai')
    e_short = self.repo['E'][:1 + ABBREV_LEN]
    e_author = self.repo.show_commit('E', format_string='%an %ai')
    expected_output = ['%s (%s 1) file2 - vanilla' % (c_short, c_author),
                       '%s (%s 2) file2 - merged' % (e_short, e_author)]
    self.assertEqual(expected_output,
                     self.repo.run(self.gc.blame, 'some/files/file2',
                                   'tag_D', abbrev=ABBREV_LEN).split('\n'))

    # Test porcelain.
    expected_output = []
    expected_output.extend(get_porcelain_for_commit('C', '1 1 1'))
    expected_output.append('previous %s some/files/file2' % self.repo['B'])
    expected_output.append('filename some/files/file2')
    expected_output.append('\tfile2 - vanilla')
    expected_output.extend(get_porcelain_for_commit('E', '1 2 1'))
    expected_output.append('previous %s some/files/file2' % self.repo['B'])
    expected_output.append('filename some/files/file2')
    expected_output.append('\tfile2 - merged')
    self.assertEqual(expected_output,
                     self.repo.run(self.gc.blame, 'some/files/file2',
                                   'tag_D', porcelain=True).split('\n'))

  def testParseCommitrefs(self):
    ret = self.repo.run(
      self.gc.parse_commitrefs, *[
        'master',
        'master~3',
        self.repo['E']+'~',
        self.repo['D']+'^2',
        'tag_C^{}',
      ]
    )
    self.assertEqual(ret, map(binascii.unhexlify, [
      self.repo['D'],
      self.repo['A'],
      self.repo['B'],
      self.repo['E'],
      self.repo['C'],
    ]))

    with self.assertRaisesRegexp(Exception, r"one of \('master', 'bananas'\)"):
      self.repo.run(self.gc.parse_commitrefs, 'master', 'bananas')

  def testRepoRoot(self):
    def cd_and_repo_root(path):
      os.chdir(path)
      return self.gc.repo_root()

    self.assertEqual(self.repo.repo_path, self.repo.run(self.gc.repo_root))
    # cd to a subdirectory; repo_root should still return the root dir.
    self.assertEqual(self.repo.repo_path,
                     self.repo.run(cd_and_repo_root, 'some/files'))

  def testTags(self):
    self.assertEqual(set(self.repo.run(self.gc.tags)),
                     {'tag_'+l for l in 'ABCDE'})

  def testTree(self):
    tree = self.repo.run(self.gc.tree, 'master:some/files')
    file1 = self.COMMIT_A['some/files/file1']['data']
    file2 = self.COMMIT_D['some/files/file2']['data']
    file3 = self.COMMIT_A['some/files/file3']['data']
    self.assertEquals(
        tree['file1'],
        ('100644', 'blob', git_test_utils.git_hash_data(file1)))
    self.assertEquals(
        tree['file2'],
        ('100755', 'blob', git_test_utils.git_hash_data(file2)))
    self.assertEquals(
        tree['file3'],
        ('100644', 'blob', git_test_utils.git_hash_data(file3)))

    tree = self.repo.run(self.gc.tree, 'master:some')
    self.assertEquals(len(tree), 2)
    # Don't check the tree hash because we're lazy :)
    self.assertEquals(tree['files'][:2], ('040000', 'tree'))

    tree = self.repo.run(self.gc.tree, 'master:wat')
    self.assertEqual(tree, None)

  def testTreeRecursive(self):
    tree = self.repo.run(self.gc.tree, 'master:some', recurse=True)
    file1 = self.COMMIT_A['some/files/file1']['data']
    file2 = self.COMMIT_D['some/files/file2']['data']
    file3 = self.COMMIT_A['some/files/file3']['data']
    other = self.COMMIT_A['some/other/file']['data']
    self.assertEquals(
        tree['files/file1'],
        ('100644', 'blob', git_test_utils.git_hash_data(file1)))
    self.assertEquals(
        tree['files/file2'],
        ('100755', 'blob', git_test_utils.git_hash_data(file2)))
    self.assertEquals(
        tree['files/file3'],
        ('100644', 'blob', git_test_utils.git_hash_data(file3)))
    self.assertEquals(
        tree['other/file'],
        ('100644', 'blob', git_test_utils.git_hash_data(other)))


class GitMutableFunctionsTest(git_test_utils.GitRepoReadWriteTestBase,
                              GitCommonTestBase):
  REPO_SCHEMA = ''

  def _intern_data(self, data):
    with tempfile.TemporaryFile() as f:
      f.write(data)
      f.seek(0)
      return self.repo.run(self.gc.intern_f, f)

  def testInternF(self):
    data = 'CoolBobcatsBro'
    data_hash = self._intern_data(data)
    self.assertEquals(git_test_utils.git_hash_data(data), data_hash)
    self.assertEquals(data, self.repo.git('cat-file', 'blob', data_hash).stdout)

  def testMkTree(self):
    tree = {}
    for i in 1, 2, 3:
      name = 'file%d' % i
      tree[name] = ('100644', 'blob', self._intern_data(name))
    tree_hash = self.repo.run(self.gc.mktree, tree)
    self.assertEquals('37b61866d6e061c4ba478e7eb525be7b5752737d', tree_hash)

  def testConfig(self):
    self.repo.git('config', '--add', 'happy.derpies', 'food')
    self.assertEquals(self.repo.run(self.gc.get_config_list, 'happy.derpies'),
                      ['food'])
    self.assertEquals(self.repo.run(self.gc.get_config_list, 'sad.derpies'), [])

    self.repo.git('config', '--add', 'happy.derpies', 'cat')
    self.assertEquals(self.repo.run(self.gc.get_config_list, 'happy.derpies'),
                      ['food', 'cat'])

    self.assertEquals('cat', self.repo.run(self.gc.get_config, 'dude.bob',
                                           'cat'))

    self.repo.run(self.gc.set_config, 'dude.bob', 'dog')

    self.assertEquals('dog', self.repo.run(self.gc.get_config, 'dude.bob',
                                           'cat'))

    self.repo.run(self.gc.del_config, 'dude.bob')

    # This should work without raising an exception
    self.repo.run(self.gc.del_config, 'dude.bob')

    self.assertEquals('cat', self.repo.run(self.gc.get_config, 'dude.bob',
                                           'cat'))

    self.assertEquals('origin/master', self.repo.run(self.gc.root))

    self.repo.git('config', 'depot-tools.upstream', 'catfood')

    self.assertEquals('catfood', self.repo.run(self.gc.root))

  def testUpstream(self):
    self.repo.git('commit', '--allow-empty', '-am', 'foooooo')
    self.assertEquals(self.repo.run(self.gc.upstream, 'bobly'), None)
    self.assertEquals(self.repo.run(self.gc.upstream, 'master'), None)
    self.repo.git('checkout', '-tb', 'happybranch', 'master')
    self.assertEquals(self.repo.run(self.gc.upstream, 'happybranch'),
                      'master')

  def testNormalizedVersion(self):
    self.assertTrue(all(
        isinstance(x, int) for x in self.repo.run(self.gc.get_git_version)))

  def testGetBranchesInfo(self):
    self.repo.git('commit', '--allow-empty', '-am', 'foooooo')
    self.repo.git('checkout', '-tb', 'happybranch', 'master')
    self.repo.git('commit', '--allow-empty', '-am', 'foooooo')
    self.repo.git('checkout', '-tb', 'child', 'happybranch')

    self.repo.git('checkout', '-tb', 'to_delete', 'master')
    self.repo.git('checkout', '-tb', 'parent_gone', 'to_delete')
    self.repo.git('branch', '-D', 'to_delete')

    supports_track = (
        self.repo.run(self.gc.get_git_version)
        >= self.gc.MIN_UPSTREAM_TRACK_GIT_VERSION)
    actual = self.repo.run(self.gc.get_branches_info, supports_track)

    expected = {
        'happybranch': (
            self.repo.run(self.gc.hash_one, 'happybranch', short=True),
            'master',
            1 if supports_track else None,
            None
        ),
        'child': (
            self.repo.run(self.gc.hash_one, 'child', short=True),
            'happybranch',
            None,
            None
        ),
        'master': (
            self.repo.run(self.gc.hash_one, 'master', short=True),
            '',
            None,
            None
        ),
        '': None,
        'parent_gone': (
            self.repo.run(self.gc.hash_one, 'parent_gone', short=True),
            'to_delete',
            None,
            None
        ),
        'to_delete': None
    }
    self.assertEquals(expected, actual)


class GitMutableStructuredTest(git_test_utils.GitRepoReadWriteTestBase,
                               GitCommonTestBase):
  REPO_SCHEMA = """
  A B C D E F G
    B H I J K
          J L

  X Y Z

  CAT DOG
  """

  COMMIT_B = {'file': {'data': 'B'}}
  COMMIT_H = {'file': {'data': 'H'}}
  COMMIT_I = {'file': {'data': 'I'}}
  COMMIT_J = {'file': {'data': 'J'}}
  COMMIT_K = {'file': {'data': 'K'}}
  COMMIT_L = {'file': {'data': 'L'}}

  def setUp(self):
    super(GitMutableStructuredTest, self).setUp()
    self.repo.git('branch', '--set-upstream-to', 'root_X', 'branch_Z')
    self.repo.git('branch', '--set-upstream-to', 'branch_G', 'branch_K')
    self.repo.git('branch', '--set-upstream-to', 'branch_K', 'branch_L')
    self.repo.git('branch', '--set-upstream-to', 'root_A', 'branch_G')
    self.repo.git('branch', '--set-upstream-to', 'root_X', 'root_A')

  def testTooManyBranches(self):
    for i in xrange(30):
      self.repo.git('branch', 'a'*i)

    _, rslt = self.repo.capture_stdio(list, self.gc.branches())
    self.assertIn('too many branches (39/20)', rslt)

    self.repo.git('config', 'depot-tools.branch-limit', 'cat')

    _, rslt = self.repo.capture_stdio(list, self.gc.branches())
    self.assertIn('too many branches (39/20)', rslt)

    self.repo.git('config', 'depot-tools.branch-limit', '100')

    # should not raise
    # This check fails with git 2.4 (see crbug.com/487172)
    self.assertEqual(38, len(self.repo.run(list, self.gc.branches())))

  def testMergeBase(self):
    self.repo.git('checkout', 'branch_K')

    self.assertEqual(
      self.repo['B'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_K', 'branch_G')
    )

    self.assertEqual(
      self.repo['J'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_L', 'branch_K')
    )

    self.assertEqual(
      self.repo['B'], self.repo.run(self.gc.get_config, 'branch.branch_K.base')
    )
    self.assertEqual(
      'branch_G', self.repo.run(self.gc.get_config,
                                'branch.branch_K.base-upstream')
    )

    # deadbeef is a bad hash, so this will result in repo['B']
    self.repo.run(self.gc.manual_merge_base, 'branch_K', 'deadbeef', 'branch_G')

    self.assertEqual(
      self.repo['B'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_K', 'branch_G')
    )

    # but if we pick a real ancestor, then it'll work
    self.repo.run(self.gc.manual_merge_base, 'branch_K', self.repo['I'],
                  'branch_G')

    self.assertEqual(
      self.repo['I'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_K', 'branch_G')
    )

    self.assertEqual({'branch_K': self.repo['I'], 'branch_L': self.repo['J']},
                     self.repo.run(self.gc.branch_config_map, 'base'))

    self.repo.run(self.gc.remove_merge_base, 'branch_K')
    self.repo.run(self.gc.remove_merge_base, 'branch_L')

    self.assertEqual(None, self.repo.run(self.gc.get_config,
                                         'branch.branch_K.base'))

    self.assertEqual({}, self.repo.run(self.gc.branch_config_map, 'base'))

    # if it's too old, then it caps at merge-base
    self.repo.run(self.gc.manual_merge_base, 'branch_K', self.repo['A'],
                  'branch_G')

    self.assertEqual(
      self.repo['B'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_K', 'branch_G')
    )

    # If the user does --set-upstream-to something else, then we discard the
    # base and recompute it.
    self.repo.run(self.gc.run, 'branch', '-u', 'root_A')
    self.assertEqual(
      self.repo['A'],
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_K')
    )

    self.assertIsNone(
      self.repo.run(self.gc.get_or_create_merge_base, 'branch_DOG'))

  def testGetBranchTree(self):
    skipped, tree = self.repo.run(self.gc.get_branch_tree)
    # This check fails with git 2.4 (see crbug.com/487172)
    self.assertEqual(skipped, {'master', 'root_X', 'branch_DOG', 'root_CAT'})
    self.assertEqual(tree, {
      'branch_G': 'root_A',
      'root_A': 'root_X',
      'branch_K': 'branch_G',
      'branch_L': 'branch_K',
      'branch_Z': 'root_X'
    })

    topdown = list(self.gc.topo_iter(tree))
    bottomup = list(self.gc.topo_iter(tree, top_down=False))

    self.assertEqual(topdown, [
      ('branch_Z', 'root_X'),
      ('root_A', 'root_X'),
      ('branch_G', 'root_A'),
      ('branch_K', 'branch_G'),
      ('branch_L', 'branch_K'),
    ])

    self.assertEqual(bottomup, [
      ('branch_L', 'branch_K'),
      ('branch_Z', 'root_X'),
      ('branch_K', 'branch_G'),
      ('branch_G', 'root_A'),
      ('root_A', 'root_X'),
    ])

  def testIsGitTreeDirty(self):
    retval = []
    self.repo.capture_stdio(
      lambda: retval.append(self.repo.run(self.gc.is_dirty_git_tree, 'foo')))

    self.assertEquals(False, retval[0])
    self.repo.open('test.file', 'w').write('test data')
    self.repo.git('add', 'test.file')

    retval = []
    self.repo.capture_stdio(
      lambda: retval.append(self.repo.run(self.gc.is_dirty_git_tree, 'foo')))
    self.assertEquals(True, retval[0])

  def testSquashBranch(self):
    self.repo.git('checkout', 'branch_K')

    self.assertEquals(True, self.repo.run(self.gc.squash_current_branch,
                                          'cool message'))

    lines = ['cool message', '']
    for l in 'HIJK':
      lines.extend((self.repo[l], l, ''))
    lines.pop()
    msg = '\n'.join(lines)

    self.assertEquals(self.repo.run(self.gc.run, 'log', '-n1', '--format=%B'),
                      msg)

    self.assertEquals(
      self.repo.git('cat-file', 'blob', 'branch_K:file').stdout,
      'K'
    )

  def testSquashBranchDefaultMessage(self):
    self.repo.git('checkout', 'branch_K')
    self.assertEquals(True, self.repo.run(self.gc.squash_current_branch))
    self.assertEquals(self.repo.run(self.gc.run, 'log', '-n1', '--format=%s'),
                      'git squash commit for branch_K.')

  def testSquashBranchEmpty(self):
    self.repo.git('checkout', 'branch_K')
    self.repo.git('checkout', 'branch_G', '.')
    self.repo.git('commit', '-m', 'revert all changes no branch')
    # Should return False since the quash would result in an empty commit
    stdout = self.repo.capture_stdio(self.gc.squash_current_branch)[0]
    self.assertEquals(stdout, 'Nothing to commit; squashed branch is empty\n')

  def testRebase(self):
    self.assertSchema("""
    A B C D E F G
      B H I J K
            J L

    X Y Z

    CAT DOG
    """)

    rslt = self.repo.run(
      self.gc.rebase, 'branch_G', 'branch_K~4', 'branch_K')
    self.assertTrue(rslt.success)

    self.assertSchema("""
    A B C D E F G H I J K
      B H I J L

    X Y Z

    CAT DOG
    """)

    rslt = self.repo.run(
      self.gc.rebase, 'branch_K', 'branch_L~1', 'branch_L', abort=True)
    self.assertFalse(rslt.success)

    self.assertFalse(self.repo.run(self.gc.in_rebase))

    rslt = self.repo.run(
      self.gc.rebase, 'branch_K', 'branch_L~1', 'branch_L', abort=False)
    self.assertFalse(rslt.success)

    self.assertTrue(self.repo.run(self.gc.in_rebase))

    self.assertEqual(self.repo.git('status', '--porcelain').stdout, 'UU file\n')
    self.repo.git('checkout', '--theirs', 'file')
    self.repo.git('add', 'file')
    self.repo.git('rebase', '--continue')

    self.assertSchema("""
    A B C D E F G H I J K L

    X Y Z

    CAT DOG
    """)

  def testStatus(self):
    def inner():
      dictified_status = lambda: {
          k: dict(v._asdict())  # pylint: disable=protected-access
          for k, v in self.repo.run(self.gc.status)
      }
      self.repo.git('mv', 'file', 'cat')
      with open('COOL', 'w') as f:
        f.write('Super cool file!')
      self.assertDictEqual(
          dictified_status(),
          {'cat':  {'lstat': 'R', 'rstat': ' ', 'src': 'file'},
           'COOL': {'lstat': '?', 'rstat': '?', 'src': 'COOL'}}
      )

    self.repo.run(inner)


class GitFreezeThaw(git_test_utils.GitRepoReadWriteTestBase):
  @classmethod
  def setUpClass(cls):
    super(GitFreezeThaw, cls).setUpClass()
    import git_common
    cls.gc = git_common
    cls.gc.TEST_MODE = True

  REPO_SCHEMA = """
  A B C D
    B E D
  """

  COMMIT_A = {
    'some/files/file1': {'data': 'file1'},
    'some/files/file2': {'data': 'file2'},
    'some/files/file3': {'data': 'file3'},
    'some/other/file':  {'data': 'otherfile'},
  }

  COMMIT_C = {
    'some/files/file2': {
      'mode': 0o755,
      'data': 'file2 - vanilla'},
  }

  COMMIT_E = {
    'some/files/file2': {'data': 'file2 - merged'},
  }

  COMMIT_D = {
    'some/files/file2': {'data': 'file2 - vanilla\nfile2 - merged'},
  }

  def testNothing(self):
    self.assertIsNotNone(self.repo.run(self.gc.thaw))  # 'Nothing to thaw'
    self.assertIsNotNone(self.repo.run(self.gc.freeze))  # 'Nothing to freeze'

  def testAll(self):
    def inner():
      with open('some/files/file2', 'a') as f2:
        print >> f2, 'cool appended line'
      os.mkdir('some/other_files')
      with open('some/other_files/subdir_file', 'w') as f3:
        print >> f3, 'new file!'
      with open('some/files/file5', 'w') as f5:
        print >> f5, 'New file!1!one!'

      STATUS_1 = '\n'.join((
        ' M some/files/file2',
        'A  some/files/file5',
        '?? some/other_files/'
      )) + '\n'

      self.repo.git('add', 'some/files/file5')

      # Freeze group 1
      self.assertEquals(self.repo.git('status', '--porcelain').stdout, STATUS_1)
      self.assertIsNone(self.gc.freeze())
      self.assertEquals(self.repo.git('status', '--porcelain').stdout, '')

      # Freeze group 2
      with open('some/files/file2', 'a') as f2:
        print >> f2, 'new! appended line!'
      self.assertEquals(self.repo.git('status', '--porcelain').stdout,
                        ' M some/files/file2\n')
      self.assertIsNone(self.gc.freeze())
      self.assertEquals(self.repo.git('status', '--porcelain').stdout, '')

      # Thaw it out!
      self.assertIsNone(self.gc.thaw())
      self.assertIsNotNone(self.gc.thaw())  # One thaw should thaw everything

      self.assertEquals(self.repo.git('status', '--porcelain').stdout, STATUS_1)

    self.repo.run(inner)

  def testTooBig(self):
    def inner():
      self.repo.git('config', 'depot-tools.freeze-size-limit', '1')
      with open('bigfile', 'w') as f:
        chunk = 'NERDFACE' * 1024
        for _ in xrange(128 * 2 + 1):  # Just over 2 mb
          f.write(chunk)
      _, err = self.repo.capture_stdio(self.gc.freeze)
      self.assertIn('too much untracked+unignored', err)

    self.repo.run(inner)

  def testTooBigMultipleFiles(self):
    def inner():
      self.repo.git('config', 'depot-tools.freeze-size-limit', '1')
      for i in xrange(3):
        with open('file%d' % i, 'w') as f:
          chunk = 'NERDFACE' * 1024
          for _ in xrange(50):  # About 400k
            f.write(chunk)
      _, err = self.repo.capture_stdio(self.gc.freeze)
      self.assertIn('too much untracked+unignored', err)

    self.repo.run(inner)

  def testMerge(self):
    def inner():
      self.repo.git('checkout', '-b', 'bad_merge_branch')
      with open('bad_merge', 'w') as f:
        f.write('bad_merge_left')
      self.repo.git('add', 'bad_merge')
      self.repo.git('commit', '-m', 'bad_merge')

      self.repo.git('checkout', 'branch_D')
      with open('bad_merge', 'w') as f:
        f.write('bad_merge_right')
      self.repo.git('add', 'bad_merge')
      self.repo.git('commit', '-m', 'bad_merge_d')

      self.repo.git('merge', 'bad_merge_branch')

      _, err = self.repo.capture_stdio(self.gc.freeze)
      self.assertIn('Cannot freeze unmerged changes', err)

    self.repo.run(inner)

  def testAddError(self):
    def inner():
      self.repo.git('checkout', '-b', 'unreadable_file_branch')
      with open('bad_file', 'w') as f:
        f.write('some text')
      os.chmod('bad_file', 0111)
      ret = self.repo.run(self.gc.freeze)
      self.assertIn('Failed to index some unindexed files.', ret)

    self.repo.run(inner)


class GitMakeWorkdir(git_test_utils.GitRepoReadOnlyTestBase, GitCommonTestBase):
  def setUp(self):
    self._tempdir = tempfile.mkdtemp()

  def tearDown(self):
    shutil.rmtree(self._tempdir)

  REPO_SCHEMA = """
  A
  """

  @unittest.skipIf(not hasattr(os, 'symlink'), "OS doesn't support symlink")
  def testMakeWorkdir(self):
    workdir = os.path.join(self._tempdir, 'workdir')
    self.gc.make_workdir(os.path.join(self.repo.repo_path, '.git'),
                         os.path.join(workdir, '.git'))
    EXPECTED_LINKS = [
        'config', 'info', 'hooks', 'logs/refs', 'objects', 'refs',
    ]
    for path in EXPECTED_LINKS:
      self.assertTrue(os.path.islink(os.path.join(workdir, '.git', path)))
      self.assertEqual(os.path.realpath(os.path.join(workdir, '.git', path)),
                       os.path.join(self.repo.repo_path, '.git', path))
    self.assertFalse(os.path.islink(os.path.join(workdir, '.git', 'HEAD')))


class GitTestUtilsTest(git_test_utils.GitRepoReadOnlyTestBase):
  REPO_SCHEMA = """
  A B C
  """

  COMMIT_A = {
    'file1': {'data': 'file1'},
  }

  COMMIT_B = {
    'file1': {'data': 'file1 changed'},
  }

  # Test special keys (custom commit data).
  COMMIT_C = {
    GitRepo.AUTHOR_NAME: 'Custom Author',
    GitRepo.AUTHOR_EMAIL: 'author@example.com',
    GitRepo.AUTHOR_DATE: datetime.datetime(1980, 9, 8, 7, 6, 5,
                                           tzinfo=git_test_utils.UTC),
    GitRepo.COMMITTER_NAME: 'Custom Committer',
    GitRepo.COMMITTER_EMAIL: 'committer@example.com',
    GitRepo.COMMITTER_DATE: datetime.datetime(1990, 4, 5, 6, 7, 8,
                                              tzinfo=git_test_utils.UTC),
    'file1': {'data': 'file1 changed again'},
  }

  def testAutomaticCommitDates(self):
    # The dates should start from 1970-01-01 and automatically increment. They
    # must be in UTC (otherwise the tests are system-dependent, and if your
    # local timezone is positive, timestamps will be <0 which causes bizarre
    # behaviour in Git; http://crbug.com/581895).
    self.assertEquals('Author McAuthorly 1970-01-01 00:00:00 +0000',
                      self.repo.show_commit('A', format_string='%an %ai'))
    self.assertEquals('Charles Committish 1970-01-02 00:00:00 +0000',
                      self.repo.show_commit('A', format_string='%cn %ci'))
    self.assertEquals('Author McAuthorly 1970-01-03 00:00:00 +0000',
                      self.repo.show_commit('B', format_string='%an %ai'))
    self.assertEquals('Charles Committish 1970-01-04 00:00:00 +0000',
                      self.repo.show_commit('B', format_string='%cn %ci'))

  def testCustomCommitData(self):
    self.assertEquals('Custom Author author@example.com '
                      '1980-09-08 07:06:05 +0000',
                      self.repo.show_commit('C', format_string='%an %ae %ai'))
    self.assertEquals('Custom Committer committer@example.com '
                      '1990-04-05 06:07:08 +0000',
                      self.repo.show_commit('C', format_string='%cn %ce %ci'))

if __name__ == '__main__':
  sys.exit(coverage_utils.covered_main(
    os.path.join(DEPOT_TOOLS_ROOT, 'git_common.py')))
