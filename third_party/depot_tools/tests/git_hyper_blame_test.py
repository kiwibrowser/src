#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for git_dates."""

import datetime
import os
import re
import shutil
import StringIO
import sys
import tempfile
import unittest

DEPOT_TOOLS_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, DEPOT_TOOLS_ROOT)

from testing_support import coverage_utils
from testing_support import git_test_utils

import git_common

GitRepo = git_test_utils.GitRepo


class GitHyperBlameTestBase(git_test_utils.GitRepoReadOnlyTestBase):
  @classmethod
  def setUpClass(cls):
    super(GitHyperBlameTestBase, cls).setUpClass()
    import git_hyper_blame
    cls.git_hyper_blame = git_hyper_blame

  def run_hyperblame(self, ignored, filename, revision):
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    ignored = [self.repo[c] for c in ignored]
    retval = self.repo.run(self.git_hyper_blame.hyper_blame, ignored, filename,
                           revision=revision, out=stdout, err=stderr)
    return retval, stdout.getvalue().rstrip().split('\n')

  def blame_line(self, commit_name, rest, author=None, filename=None):
    """Generate a blame line from a commit.

    Args:
      commit_name: The commit's schema name.
      rest: The blame line after the timestamp. e.g., '2) file2 - merged'.
      author: The author's name. If omitted, reads the name out of the commit.
      filename: The filename. If omitted, not shown in the blame line.
    """
    short = self.repo[commit_name][:8]
    start = '%s %s' % (short, filename) if filename else short
    if author is None:
      author = self.repo.show_commit(commit_name, format_string='%an %ai')
    else:
      author += self.repo.show_commit(commit_name, format_string=' %ai')
    return '%s (%s %s' % (start, author, rest)

class GitHyperBlameMainTest(GitHyperBlameTestBase):
  """End-to-end tests on a very simple repo."""
  REPO_SCHEMA = "A B C D"

  COMMIT_A = {
    'some/files/file': {'data': 'line 1\nline 2\n'},
  }

  COMMIT_B = {
    'some/files/file': {'data': 'line 1\nline 2.1\n'},
  }

  COMMIT_C = {
    'some/files/file': {'data': 'line 1.1\nline 2.1\n'},
  }

  COMMIT_D = {
    # This file should be automatically considered for ignore.
    '.git-blame-ignore-revs': {'data': 'tag_C'},
    # This file should not be considered.
    'some/files/.git-blame-ignore-revs': {'data': 'tag_B'},
  }

  def setUp(self):
    super(GitHyperBlameMainTest, self).setUp()
    # Most tests want to check out C (so the .git-blame-ignore-revs is not
    # used).
    self.repo.git('checkout', '-f', 'tag_C')

  def testBasicBlame(self):
    """Tests the main function (simple end-to-end test with no ignores)."""
    expected_output = [self.blame_line('C', '1) line 1.1'),
                       self.blame_line('B', '2) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['tag_C', 'some/files/file'], stdout=stdout,
                           stderr=stderr)
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('', stderr.getvalue())

  def testIgnoreSimple(self):
    """Tests the main function (simple end-to-end test with ignores)."""
    expected_output = [self.blame_line('C', ' 1) line 1.1'),
                       self.blame_line('A', '2*) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['-i', 'tag_B', 'tag_C', 'some/files/file'],
                           stdout=stdout, stderr=stderr)
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('', stderr.getvalue())

  def testBadRepo(self):
    """Tests the main function (not in a repo)."""
    # Make a temp dir that has no .git directory.
    curdir = os.getcwd()
    tempdir = tempfile.mkdtemp(suffix='_nogit', prefix='git_repo')
    try:
      os.chdir(tempdir)
      stdout = StringIO.StringIO()
      stderr = StringIO.StringIO()
      retval = self.git_hyper_blame.main(
          args=['-i', 'tag_B', 'tag_C', 'some/files/file'], stdout=stdout,
          stderr=stderr)
    finally:
      shutil.rmtree(tempdir)
      os.chdir(curdir)

    self.assertNotEqual(0, retval)
    self.assertEqual('', stdout.getvalue())
    r = re.compile('^fatal: Not a git repository', re.I)
    self.assertRegexpMatches(stderr.getvalue(), r)

  def testBadFilename(self):
    """Tests the main function (bad filename)."""
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['-i', 'tag_B', 'tag_C', 'some/files/xxxx'],
                           stdout=stdout, stderr=stderr)
    self.assertNotEqual(0, retval)
    self.assertEqual('', stdout.getvalue())
    # TODO(mgiuca): This test used to test the exact string, but it broke due to
    # an upstream bug in git-blame. For now, just check the start of the string.
    # A patch has been sent upstream; when it rolls out we can revert back to
    # the original test logic.
    self.assertTrue(
        stderr.getvalue().startswith('fatal: no such path some/files/xxxx in '))

  def testBadRevision(self):
    """Tests the main function (bad revision to blame from)."""
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['-i', 'tag_B', 'xxxx', 'some/files/file'],
                           stdout=stdout, stderr=stderr)
    self.assertNotEqual(0, retval)
    self.assertEqual('', stdout.getvalue())
    self.assertRegexpMatches(stderr.getvalue(),
                             '^fatal: ambiguous argument \'xxxx\': unknown '
                             'revision or path not in the working tree.')

  def testBadIgnore(self):
    """Tests the main function (bad revision passed to -i)."""
    expected_output = [self.blame_line('C', '1) line 1.1'),
                       self.blame_line('B', '2) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['-i', 'xxxx', 'tag_C', 'some/files/file'],
                           stdout=stdout, stderr=stderr)
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('warning: unknown revision \'xxxx\'.\n', stderr.getvalue())

  def testIgnoreFile(self):
    """Tests passing the ignore list in a file."""
    expected_output = [self.blame_line('C', ' 1) line 1.1'),
                       self.blame_line('A', '2*) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()

    with tempfile.NamedTemporaryFile(mode='w+', prefix='ignore') as ignore_file:
      ignore_file.write('# Line comments are allowed.\n'.format(self.repo['B']))
      ignore_file.write('\n')
      ignore_file.write('{}\n'.format(self.repo['B']))
      # A revision that is not in the repo (should be ignored).
      ignore_file.write('xxxx\n')
      ignore_file.flush()
      retval = self.repo.run(self.git_hyper_blame.main,
                             args=['--ignore-file', ignore_file.name, 'tag_C',
                                   'some/files/file'],
                             stdout=stdout, stderr=stderr)

    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('warning: unknown revision \'xxxx\'.\n', stderr.getvalue())

  def testDefaultIgnoreFile(self):
    """Tests automatically using a default ignore list."""
    # Check out revision D. We expect the script to use the default ignore list
    # that is checked out, *not* the one committed at the given revision.
    self.repo.git('checkout', '-f', 'tag_D')

    expected_output = [self.blame_line('A', '1*) line 1.1'),
                       self.blame_line('B', ' 2) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()

    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['tag_D', 'some/files/file'],
                           stdout=stdout, stderr=stderr)

    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('', stderr.getvalue())

    # Test blame from a different revision. Despite the default ignore file
    # *not* being committed at that revision, it should still be picked up
    # because D is currently checked out.
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()

    retval = self.repo.run(self.git_hyper_blame.main,
                           args=['tag_C', 'some/files/file'],
                           stdout=stdout, stderr=stderr)

    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('', stderr.getvalue())

  def testNoDefaultIgnores(self):
    """Tests the --no-default-ignores switch."""
    # Check out revision D. This has a .git-blame-ignore-revs file, which we
    # expect to be ignored due to --no-default-ignores.
    self.repo.git('checkout', '-f', 'tag_D')

    expected_output = [self.blame_line('C', '1) line 1.1'),
                       self.blame_line('B', '2) line 2.1')]
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()

    retval = self.repo.run(
        self.git_hyper_blame.main,
        args=['tag_D', 'some/files/file', '--no-default-ignores'],
        stdout=stdout, stderr=stderr)

    self.assertEqual(0, retval)
    self.assertEqual(expected_output, stdout.getvalue().rstrip().split('\n'))
    self.assertEqual('', stderr.getvalue())

class GitHyperBlameSimpleTest(GitHyperBlameTestBase):
  REPO_SCHEMA = """
  A B D E F G H
  A C D
  """

  COMMIT_A = {
    'some/files/file1': {'data': 'file1'},
    'some/files/file2': {'data': 'file2'},
    'some/files/empty': {'data': ''},
    'some/other/file':  {'data': 'otherfile'},
  }

  COMMIT_B = {
    'some/files/file2': {
      'mode': 0o755,
      'data': 'file2 - vanilla\n'},
    'some/files/empty': {'data': 'not anymore'},
    'some/files/file3': {'data': 'file3'},
  }

  COMMIT_C = {
    'some/files/file2': {'data': 'file2 - merged\n'},
  }

  COMMIT_D = {
    'some/files/file2': {'data': 'file2 - vanilla\nfile2 - merged\n'},
  }

  COMMIT_E = {
    'some/files/file2': {'data': 'file2 - vanilla\nfile_x - merged\n'},
  }

  COMMIT_F = {
    'some/files/file2': {'data': 'file2 - vanilla\nfile_y - merged\n'},
  }

  # Move file2 from files to other.
  COMMIT_G = {
    'some/files/file2': {'data': None},
    'some/other/file2': {'data': 'file2 - vanilla\nfile_y - merged\n'},
  }

  COMMIT_H = {
    'some/other/file2': {'data': 'file2 - vanilla\nfile_z - merged\n'},
  }

  def testBlameError(self):
    """Tests a blame on a non-existent file."""
    expected_output = ['']
    retval, output = self.run_hyperblame([], 'some/other/file2', 'tag_D')
    self.assertNotEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testBlameEmpty(self):
    """Tests a blame of an empty file with no ignores."""
    expected_output = ['']
    retval, output = self.run_hyperblame([], 'some/files/empty', 'tag_A')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testBasicBlame(self):
    """Tests a basic blame with no ignores."""
    # Expect to blame line 1 on B, line 2 on C.
    expected_output = [self.blame_line('B', '1) file2 - vanilla'),
                       self.blame_line('C', '2) file2 - merged')]
    retval, output = self.run_hyperblame([], 'some/files/file2', 'tag_D')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testBlameRenamed(self):
    """Tests a blame with no ignores on a renamed file."""
    # Expect to blame line 1 on B, line 2 on H.
    # Because the file has a different name than it had when (some of) these
    # lines were changed, expect the filenames to be displayed.
    expected_output = [self.blame_line('B', '1) file2 - vanilla',
                                       filename='some/files/file2'),
                       self.blame_line('H', '2) file_z - merged',
                                       filename='some/other/file2')]
    retval, output = self.run_hyperblame([], 'some/other/file2', 'tag_H')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testIgnoreSimpleEdits(self):
    """Tests a blame with simple (line-level changes) commits ignored."""
    # Expect to blame line 1 on B, line 2 on E.
    expected_output = [self.blame_line('B', '1) file2 - vanilla'),
                       self.blame_line('E', '2) file_x - merged')]
    retval, output = self.run_hyperblame([], 'some/files/file2', 'tag_E')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

    # Ignore E; blame line 1 on B, line 2 on C.
    expected_output = [self.blame_line('B', ' 1) file2 - vanilla'),
                       self.blame_line('C', '2*) file_x - merged')]
    retval, output = self.run_hyperblame(['E'], 'some/files/file2', 'tag_E')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

    # Ignore E and F; blame line 1 on B, line 2 on C.
    expected_output = [self.blame_line('B', ' 1) file2 - vanilla'),
                       self.blame_line('C', '2*) file_y - merged')]
    retval, output = self.run_hyperblame(['E', 'F'], 'some/files/file2',
                                         'tag_F')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testIgnoreInitialCommit(self):
    """Tests a blame with the initial commit ignored."""
    # Ignore A. Expect A to get blamed anyway.
    expected_output = [self.blame_line('A', '1) file1')]
    retval, output = self.run_hyperblame(['A'], 'some/files/file1', 'tag_A')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testIgnoreFileAdd(self):
    """Tests a blame ignoring the commit that added this file."""
    # Ignore A. Expect A to get blamed anyway.
    expected_output = [self.blame_line('B', '1) file3')]
    retval, output = self.run_hyperblame(['B'], 'some/files/file3', 'tag_B')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testIgnoreFilePopulate(self):
    """Tests a blame ignoring the commit that added data to an empty file."""
    # Ignore A. Expect A to get blamed anyway.
    expected_output = [self.blame_line('B', '1) not anymore')]
    retval, output = self.run_hyperblame(['B'], 'some/files/empty', 'tag_B')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

class GitHyperBlameLineMotionTest(GitHyperBlameTestBase):
  REPO_SCHEMA = """
  A B C D E F
  """

  COMMIT_A = {
    'file':  {'data': 'A\ngreen\nblue\n'},
  }

  # Change "green" to "yellow".
  COMMIT_B = {
    'file': {'data': 'A\nyellow\nblue\n'},
  }

  # Insert 2 lines at the top,
  # Change "yellow" to "red".
  # Insert 1 line at the bottom.
  COMMIT_C = {
    'file': {'data': 'X\nY\nA\nred\nblue\nZ\n'},
  }

  # Insert 2 more lines at the top.
  COMMIT_D = {
    'file': {'data': 'earth\nfire\nX\nY\nA\nred\nblue\nZ\n'},
  }

  # Insert a line before "red", and indent "red" and "blue".
  COMMIT_E = {
    'file': {'data': 'earth\nfire\nX\nY\nA\ncolors:\n red\n blue\nZ\n'},
  }

  # Insert a line between "A" and "colors".
  COMMIT_F = {
    'file': {'data': 'earth\nfire\nX\nY\nA\nB\ncolors:\n red\n blue\nZ\n'},
  }

  def testCacheDiffHunks(self):
    """Tests the cache_diff_hunks internal function."""
    expected_hunks = [((0, 0), (1, 2)),
                      ((2, 1), (4, 1)),
                      ((3, 0), (6, 1)),
                      ]
    hunks = self.repo.run(self.git_hyper_blame.cache_diff_hunks, 'tag_B',
                          'tag_C')
    self.assertEqual(expected_hunks, hunks)

  def testApproxLinenoAcrossRevs(self):
    """Tests the approx_lineno_across_revs internal function."""
    # Note: For all of these tests, the "old revision" and "new revision" are
    # reversed, which matches the usage by hyper_blame.

    # Test an unchanged line before any hunks in the diff. Should be unchanged.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_B', 'tag_A', 1)
    self.assertEqual(1, lineno)

    # Test an unchanged line after all hunks in the diff. Should be matched to
    # the line's previous position in the file.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_D', 'tag_C', 6)
    self.assertEqual(4, lineno)

    # Test a line added in a new hunk. Should be matched to the line *before*
    # where the hunk was inserted in the old version of the file.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_F', 'tag_E', 6)
    self.assertEqual(5, lineno)

    # Test lines added in a new hunk at the very start of the file. This tests
    # an edge case: normally it would be matched to the line *before* where the
    # hunk was inserted (Line 0), but since the hunk is at the start of the
    # file, we match to Line 1.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_C', 'tag_B', 1)
    self.assertEqual(1, lineno)
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_C', 'tag_B', 2)
    self.assertEqual(1, lineno)

    # Test an unchanged line in between hunks in the diff. Should be matched to
    # the line's previous position in the file.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_C', 'tag_B', 3)
    self.assertEqual(1, lineno)

    # Test a changed line. Should be matched to the hunk's previous position in
    # the file.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_C', 'tag_B', 4)
    self.assertEqual(2, lineno)

    # Test a line added in a new hunk at the very end of the file. Should be
    # matched to the line *before* where the hunk was inserted (the last line of
    # the file). Technically same as the case above but good to boundary test.
    lineno = self.repo.run(self.git_hyper_blame.approx_lineno_across_revs,
                           'file', 'file', 'tag_C', 'tag_B', 6)
    self.assertEqual(3, lineno)

  def testInterHunkLineMotion(self):
    """Tests a blame with line motion in another hunk in the ignored commit."""
    # Blame from D, ignoring C.

    # Lines 1, 2 were added by D.
    # Lines 3, 4 were added by C (but ignored, so blame A).
    # Line 5 was added by A.
    # Line 6 was modified by C (but ignored, so blame B). (Note: This requires
    # the algorithm to figure out that Line 6 in D == Line 4 in C ~= Line 2 in
    # B, so it blames B. Otherwise, it would blame A.)
    # Line 7 was added by A.
    # Line 8 was added by C (but ignored, so blame A).
    expected_output = [self.blame_line('D', ' 1) earth'),
                       self.blame_line('D', ' 2) fire'),
                       self.blame_line('A', '3*) X'),
                       self.blame_line('A', '4*) Y'),
                       self.blame_line('A', ' 5) A'),
                       self.blame_line('B', '6*) red'),
                       self.blame_line('A', ' 7) blue'),
                       self.blame_line('A', '8*) Z'),
                       ]
    retval, output = self.run_hyperblame(['C'], 'file', 'tag_D')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testIntraHunkLineMotion(self):
    """Tests a blame with line motion in the same hunk in the ignored commit."""
    # This test was mostly written as a demonstration of the limitations of the
    # current algorithm (it exhibits non-ideal behaviour).

    # Blame from E, ignoring E.
    # Line 6 was added by E (but ignored, so blame C).
    # Lines 7, 8 were modified by E (but ignored, so blame A).
    # TODO(mgiuca): Ideally, this would blame Line 7 on C, because the line
    # "red" was added by C, and this is just a small change to that line. But
    # the current algorithm can't deal with line motion within a hunk, so it
    # just assumes Line 7 in E ~= Line 7 in D == Line 3 in A (which was "blue").
    expected_output = [self.blame_line('D', ' 1) earth'),
                       self.blame_line('D', ' 2) fire'),
                       self.blame_line('C', ' 3) X'),
                       self.blame_line('C', ' 4) Y'),
                       self.blame_line('A', ' 5) A'),
                       self.blame_line('C', '6*) colors:'),
                       self.blame_line('A', '7*)  red'),
                       self.blame_line('A', '8*)  blue'),
                       self.blame_line('C', ' 9) Z'),
                       ]
    retval, output = self.run_hyperblame(['E'], 'file', 'tag_E')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)


class GitHyperBlameLineNumberTest(GitHyperBlameTestBase):
  REPO_SCHEMA = """
  A B C D
  """

  COMMIT_A = {
    'file':  {'data': 'red\nblue\n'},
  }

  # Change "blue" to "green".
  COMMIT_B = {
    'file': {'data': 'red\ngreen\n'},
  }

  # Insert 2 lines at the top,
  COMMIT_C = {
    'file': {'data': '\n\nred\ngreen\n'},
  }

  # Change "green" to "yellow".
  COMMIT_D = {
    'file': {'data': '\n\nred\nyellow\n'},
  }

  def testTwoChangesWithAddedLines(self):
    """Regression test for https://crbug.com/709831.

    Tests a line with multiple ignored edits, and a line number change in
    between (such that the line number in the current revision is bigger than
    the file's line count at the older ignored revision).
    """
    expected_output = [self.blame_line('C', ' 1) '),
                       self.blame_line('C', ' 2) '),
                       self.blame_line('A', ' 3) red'),
                       self.blame_line('A', '4*) yellow'),
                       ]
    # Due to https://crbug.com/709831, ignoring both B and D would crash,
    # because of C (in between those revisions) which moves Line 2 to Line 4.
    # The algorithm would incorrectly think that Line 4 was still on Line 4 in
    # Commit B, even though it was Line 2 at that time. Its index is out of
    # range in the number of lines in Commit B.
    retval, output = self.run_hyperblame(['B', 'D'], 'file', 'tag_D')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)


class GitHyperBlameUnicodeTest(GitHyperBlameTestBase):
  REPO_SCHEMA = """
  A B C
  """

  COMMIT_A = {
    GitRepo.AUTHOR_NAME: 'ASCII Author',
    'file':  {'data': 'red\nblue\n'},
  }

  # Add a line.
  COMMIT_B = {
    GitRepo.AUTHOR_NAME: u'\u4e2d\u56fd\u4f5c\u8005'.encode('utf-8'),
    'file': {'data': 'red\ngreen\nblue\n'},
  }

  # Modify a line with non-UTF-8 author and file text.
  COMMIT_C = {
    GitRepo.AUTHOR_NAME: u'Lat\u00edn-1 Author'.encode('latin-1'),
    'file': {'data': u'red\ngre\u00e9n\nblue\n'.encode('latin-1')},
  }

  def testNonASCIIAuthorName(self):
    """Ensures correct tabulation.

    Tests the case where there are non-ASCII (UTF-8) characters in the author
    name.

    Regression test for https://crbug.com/808905.
    """
    expected_output = [
        self.blame_line('A', '1) red', author='ASCII Author'),
        # Expect 8 spaces, to line up with the other name.
        self.blame_line('B', '2) green',
            author=u'\u4e2d\u56fd\u4f5c\u8005        '.encode('utf-8')),
        self.blame_line('A', '3) blue', author='ASCII Author'),
    ]
    retval, output = self.run_hyperblame([], 'file', 'tag_B')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)

  def testNonUTF8Data(self):
    """Ensures correct behaviour even if author or file data is not UTF-8.

    There is no guarantee that a file will be UTF-8-encoded, so this is
    realistic.
    """
    expected_output = [
        self.blame_line('A', '1) red', author='ASCII Author  '),
        # The Author has been re-encoded as UTF-8. The file data is preserved as
        # raw byte data.
        self.blame_line('C', '2) gre\xe9n', author='Lat\xc3\xadn-1 Author'),
        self.blame_line('A', '3) blue', author='ASCII Author  '),
    ]
    retval, output = self.run_hyperblame([], 'file', 'tag_C')
    self.assertEqual(0, retval)
    self.assertEqual(expected_output, output)


if __name__ == '__main__':
  sys.exit(coverage_utils.covered_main(
    os.path.join(DEPOT_TOOLS_ROOT, 'git_hyper_blame.py')))
