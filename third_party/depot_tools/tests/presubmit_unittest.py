#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for presubmit_support.py and presubmit_canned_checks.py."""

# pylint: disable=no-member,E1103

import StringIO
import functools
import itertools
import logging
import multiprocessing
import os
import re
import sys
import time
import unittest
import urllib2

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, _ROOT)

from testing_support.super_mox import mox, SuperMoxTestBase
from third_party import mock

import owners
import owners_finder
import subprocess2 as subprocess
import presubmit_support as presubmit
import auth
import git_cl
import git_common as git
import json

# Shortcut.
presubmit_canned_checks = presubmit.presubmit_canned_checks


# Access to a protected member XXX of a client class
# pylint: disable=protected-access


class MockTemporaryFile(object):
  """Simple mock for files returned by tempfile.NamedTemporaryFile()."""
  def __init__(self, name):
    self.name = name

  def __enter__(self):
    return self

  def __exit__(self, *args):
    pass


class PresubmitTestsBase(SuperMoxTestBase):
  """Sets up and tears down the mocks but doesn't test anything as-is."""
  presubmit_text = """
def CheckChangeOnUpload(input_api, output_api):
  if input_api.change.tags.get('ERROR'):
    return [output_api.PresubmitError("!!")]
  if input_api.change.tags.get('PROMPT_WARNING'):
    return [output_api.PresubmitPromptWarning("??")]
  else:
    return ()
"""

  presubmit_trymaster = """
def GetPreferredTryMasters(project, change):
  return %s
"""

  presubmit_diffs = """
diff --git %(filename)s %(filename)s
index fe3de7b..54ae6e1 100755
--- %(filename)s       2011-02-09 10:38:16.517224845 -0800
+++ %(filename)s       2011-02-09 10:38:53.177226516 -0800
@@ -1,6 +1,5 @@
 this is line number 0
 this is line number 1
-this is line number 2 to be deleted
 this is line number 3
 this is line number 4
 this is line number 5
@@ -8,7 +7,7 @@
 this is line number 7
 this is line number 8
 this is line number 9
-this is line number 10 to be modified
+this is line number 10
 this is line number 11
 this is line number 12
 this is line number 13
@@ -21,9 +20,8 @@
 this is line number 20
 this is line number 21
 this is line number 22
-this is line number 23
-this is line number 24
-this is line number 25
+this is line number 23.1
+this is line number 25.1
 this is line number 26
 this is line number 27
 this is line number 28
@@ -31,6 +29,7 @@
 this is line number 30
 this is line number 31
 this is line number 32
+this is line number 32.1
 this is line number 33
 this is line number 34
 this is line number 35
@@ -38,14 +37,14 @@
 this is line number 37
 this is line number 38
 this is line number 39
-
 this is line number 40
-this is line number 41
+this is line number 41.1
 this is line number 42
 this is line number 43
 this is line number 44
 this is line number 45
+
 this is line number 46
 this is line number 47
-this is line number 48
+this is line number 48.1
 this is line number 49
"""

  def setUp(self):
    SuperMoxTestBase.setUp(self)
    class FakeChange(object):
      def __init__(self, obj):
        self._root = obj.fake_root_dir
      def RepositoryRoot(self):
        return self._root

    self.mox.StubOutWithMock(presubmit, 'random')
    self.mox.StubOutWithMock(presubmit, 'warn')
    presubmit._ASKED_FOR_FEEDBACK = False
    self.fake_root_dir = self.RootDir()
    self.fake_change = FakeChange(self)

    # Special mocks.
    def MockAbsPath(f):
      return f
    def MockChdir(f):
      return None
    # SuperMoxTestBase already mock these but simplify our life.
    presubmit.os.path.abspath = MockAbsPath
    presubmit.os.getcwd = self.RootDir
    presubmit.os.chdir = MockChdir
    self.mox.StubOutWithMock(presubmit.scm, 'determine_scm')
    self.mox.StubOutWithMock(presubmit.gclient_utils, 'FileRead')
    self.mox.StubOutWithMock(presubmit.gclient_utils, 'FileWrite')
    self.mox.StubOutWithMock(presubmit.scm.GIT, 'GenerateDiff')

    # On some platforms this does all sorts of undesirable system calls, so
    # just permanently mock it with a lambda that returns 2
    multiprocessing.cpu_count = lambda: 2


class PresubmitUnittest(PresubmitTestsBase):
  """General presubmit_support.py tests (excluding InputApi and OutputApi)."""

  _INHERIT_SETTINGS = 'inherit-review-settings-ok'

  def testMembersChanged(self):
    self.mox.ReplayAll()
    members = [
      'AffectedFile', 'Change', 'DoPostUploadExecuter', 'DoPresubmitChecks',
      'GetPostUploadExecuter', 'GitAffectedFile', 'CommandData',
      'GitChange', 'InputApi', 'ListRelevantPresubmitFiles', 'main',
      'OutputApi', 'ParseFiles',
      'PresubmitFailure', 'PresubmitExecuter', 'PresubmitOutput', 'ScanSubDirs',
      'SigintHandler', 'ThreadPool',
      'ast', 'cPickle', 'cpplint', 'cStringIO', 'contextlib',
      'canned_check_filter', 'fix_encoding', 'fnmatch', 'gclient_utils',
      'git_footers', 'glob', 'inspect', 'json', 'load_files', 'logging',
      'marshal', 'normpath', 'optparse', 'os', 'owners', 'owners_finder',
      'pickle', 'presubmit_canned_checks', 'random', 're', 'scm',
      'sigint_handler', 'signal',
      'subprocess', 'sys', 'tempfile', 'threading',
      'time', 'traceback', 'types', 'unittest',
      'urllib2', 'warn', 'multiprocessing', 'DoGetTryMasters',
      'GetTryMastersExecuter', 'itertools', 'urlparse', 'gerrit_util',
      'GerritAccessor',
    ]
    # If this test fails, you should add the relevant test.
    self.compareMembers(presubmit, members)

  def testCannedCheckFilter(self):
    canned = presubmit.presubmit_canned_checks
    orig = canned.CheckOwners
    with presubmit.canned_check_filter(['CheckOwners']):
      self.assertNotEqual(canned.CheckOwners, orig)
      self.assertEqual(canned.CheckOwners(None, None), [])
    self.assertEqual(canned.CheckOwners, orig)

  def testListRelevantPresubmitFiles(self):
    files = [
      'blat.cc',
      presubmit.os.path.join('foo', 'haspresubmit', 'yodle', 'smart.h'),
      presubmit.os.path.join('moo', 'mat', 'gat', 'yo.h'),
      presubmit.os.path.join('foo', 'luck.h'),
    ]
    inherit_path = presubmit.os.path.join(self.fake_root_dir,
                                          self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit.os.path.join(self.fake_root_dir,
                                  'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'foo')).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(self.fake_root_dir, 'foo',
                              'haspresubmit')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(
        presubmit.os.path.join(self.fake_root_dir, 'foo', 'haspresubmit',
             'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.listdir(
        presubmit.os.path.join(
            self.fake_root_dir, 'foo', 'haspresubmit', 'yodle')).AndReturn(
            ['PRESUBMIT.py'])
    presubmit.os.path.isfile(
        presubmit.os.path.join(
            self.fake_root_dir, 'foo', 'haspresubmit', 'yodle',
            'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'moo')).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'moo', 'mat')).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'moo', 'mat', 'gat')).AndReturn([])
    self.mox.ReplayAll()

    presubmit_files = presubmit.ListRelevantPresubmitFiles(files,
                                                           self.fake_root_dir)
    self.assertEqual(presubmit_files,
        [
          presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py'),
          presubmit.os.path.join(
              self.fake_root_dir, 'foo', 'haspresubmit', 'PRESUBMIT.py'),
          presubmit.os.path.join(
              self.fake_root_dir, 'foo', 'haspresubmit', 'yodle',
              'PRESUBMIT.py')
        ])

  def testListUserPresubmitFiles(self):
    files = ['blat.cc',]
    inherit_path = presubmit.os.path.join(self.fake_root_dir,
                                          self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn(
        ['PRESUBMIT.py', 'PRESUBMIT_test.py', 'PRESUBMIT-user.py'])
    presubmit.os.path.isfile(presubmit.os.path.join(self.fake_root_dir,
                                  'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.path.isfile(presubmit.os.path.join(self.fake_root_dir,
                                  'PRESUBMIT_test.py')).AndReturn(True)
    presubmit.os.path.isfile(presubmit.os.path.join(self.fake_root_dir,
                                  'PRESUBMIT-user.py')).AndReturn(True)
    self.mox.ReplayAll()

    presubmit_files = presubmit.ListRelevantPresubmitFiles(files,
                                                           self.fake_root_dir)
    self.assertEqual(presubmit_files, [
        presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py'),
        presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT-user.py'),
    ])

  def testListRelevantPresubmitFilesInheritSettings(self):
    sys_root_dir = self._OS_SEP
    root_dir = presubmit.os.path.join(sys_root_dir, 'foo', 'bar')
    files = [
      'test.cc',
      presubmit.os.path.join('moo', 'test2.cc'),
      presubmit.os.path.join('zoo', 'test3.cc')
    ]
    inherit_path = presubmit.os.path.join(root_dir, self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(True)
    presubmit.os.listdir(sys_root_dir).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(
        sys_root_dir, 'foo')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit.os.path.join(
        sys_root_dir, 'foo', 'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        sys_root_dir, 'foo', 'bar')).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(
        sys_root_dir, 'foo', 'bar', 'moo')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit.os.path.join(
        sys_root_dir, 'foo', 'bar', 'moo', 'PRESUBMIT.py')).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        sys_root_dir, 'foo', 'bar', 'zoo')).AndReturn([])
    self.mox.ReplayAll()

    presubmit_files = presubmit.ListRelevantPresubmitFiles(files, root_dir)
    self.assertEqual(presubmit_files,
        [
          presubmit.os.path.join(sys_root_dir, 'foo', 'PRESUBMIT.py'),
          presubmit.os.path.join(
              sys_root_dir, 'foo', 'bar', 'moo', 'PRESUBMIT.py')
        ])

  def testTagLineRe(self):
    self.mox.ReplayAll()
    m = presubmit.Change.TAG_LINE_RE.match(' BUG =1223, 1445  \t')
    self.failUnless(m)
    self.failUnlessEqual(m.group('key'), 'BUG')
    self.failUnlessEqual(m.group('value'), '1223, 1445')

  def testGitChange(self):
    description_lines = ('Hello there',
                         'this is a change',
                         'BUG=123',
                         'and some more regular text  \t')
    unified_diff = [
        'diff --git binary_a.png binary_a.png',
        'new file mode 100644',
        'index 0000000..6fbdd6d',
        'Binary files /dev/null and binary_a.png differ',
        'diff --git binary_d.png binary_d.png',
        'deleted file mode 100644',
        'index 6fbdd6d..0000000',
        'Binary files binary_d.png and /dev/null differ',
        'diff --git binary_md.png binary_md.png',
        'index 6fbdd6..be3d5d8 100644',
        'GIT binary patch',
        'delta 109',
        'zcmeyihjs5>)(Opwi4&WXB~yyi6N|G`(i5|?i<2_a@)OH5N{Um`D-<SM@g!_^W9;SR',
        'zO9b*W5{pxTM0slZ=F42indK9U^MTyVQlJ2s%1BMmEKMv1Q^gtS&9nHn&*Ede;|~CU',
        'CMJxLN',
        '',
        'delta 34',
        'scmV+-0Nww+y#@BX1(1W0gkzIp3}CZh0gVZ>`wGVcgW(Rh;SK@ZPa9GXlK=n!',
        '',
        'diff --git binary_m.png binary_m.png',
        'index 6fbdd6d..be3d5d8 100644',
        'Binary files binary_m.png and binary_m.png differ',
        'diff --git boo/blat.cc boo/blat.cc',
        'new file mode 100644',
        'index 0000000..37d18ad',
        '--- boo/blat.cc',
        '+++ boo/blat.cc',
        '@@ -0,0 +1,5 @@',
        '+This is some text',
        '+which lacks a copyright warning',
        '+but it is nonetheless interesting',
        '+and worthy of your attention.',
        '+Its freshness factor is through the roof.',
        'diff --git floo/delburt.cc floo/delburt.cc',
        'deleted file mode 100644',
        'index e06377a..0000000',
        '--- floo/delburt.cc',
        '+++ /dev/null',
        '@@ -1,14 +0,0 @@',
        '-This text used to be here',
        '-but someone, probably you,',
        '-having consumed the text',
        '-  (absorbed its meaning)',
        '-decided that it should be made to not exist',
        '-that others would not read it.',
        '-  (What happened here?',
        '-was the author incompetent?',
        '-or is the world today so different from the world',
        '-   the author foresaw',
        '-and past imaginination',
        '-   amounts to rubble, insignificant,',
        '-something to be tripped over',
        '-and frustrated by)',
        'diff --git foo/TestExpectations foo/TestExpectations',
        'index c6e12ab..d1c5f23 100644',
        '--- foo/TestExpectations',
        '+++ foo/TestExpectations',
        '@@ -1,12 +1,24 @@',
        '-Stranger, behold:',
        '+Strange to behold:',
        '   This is a text',
        ' Its contents existed before.',
        '',
        '-It is written:',
        '+Weasel words suggest:',
        '   its contents shall exist after',
        '   and its contents',
        ' with the progress of time',
        ' will evolve,',
        '-   snaillike,',
        '+   erratically,',
        ' into still different texts',
        '-from this.',
        '\ No newline at end of file',
        '+from this.',
        '+',
        '+For the most part,',
        '+I really think unified diffs',
        '+are elegant: the way you can type',
        '+diff --git inside/a/text inside/a/text',
        '+or something silly like',
        '+@@ -278,6 +278,10 @@',
        '+and have this not be interpreted',
        '+as the start of a new file',
        '+or anything messed up like that,',
        '+because you parsed the header',
        '+correctly.',
        '\ No newline at end of file',
            '']
    files = [('A      ', 'binary_a.png'),
             ('D      ', 'binary_d.png'),
             ('M      ', 'binary_m.png'),
             ('M      ', 'binary_md.png'),  # Binary w/ diff
             ('A      ', 'boo/blat.cc'),
             ('D      ', 'floo/delburt.cc'),
             ('M      ', 'foo/TestExpectations')]

    for op, path in files:
      full_path = presubmit.os.path.join(self.fake_root_dir, *path.split('/'))
      if not op.startswith('D'):
        os.path.isfile(full_path).AndReturn(True)

    presubmit.scm.GIT.GenerateDiff(
        self.fake_root_dir, files=[], full_move=True, branch=None
        ).AndReturn('\n'.join(unified_diff))

    self.mox.ReplayAll()

    change = presubmit.GitChange(
        'mychange',
        '\n'.join(description_lines),
        self.fake_root_dir,
        files,
        0,
        0,
        None,
        upstream=None)
    self.failUnless(change.Name() == 'mychange')
    self.failUnless(change.DescriptionText() ==
                    'Hello there\nthis is a change\nand some more regular text')
    self.failUnless(change.FullDescriptionText() ==
                    '\n'.join(description_lines))

    self.failUnless(change.BugsFromDescription() == ['123'])

    self.failUnless(len(change.AffectedFiles()) == 7)
    self.failUnless(len(change.AffectedFiles()) == 7)
    self.failUnless(len(change.AffectedFiles(include_deletes=False)) == 5)
    self.failUnless(len(change.AffectedFiles(include_deletes=False)) == 5)

    # Note that on git, there's no distinction between binary files and text
    # files; everything that's not a delete is a text file.
    affected_text_files = change.AffectedTestableFiles()
    self.failUnless(len(affected_text_files) == 5)

    local_paths = change.LocalPaths()
    expected_paths = [os.path.normpath(f) for op, f in files]
    self.assertEqual(local_paths, expected_paths)

    actual_rhs_lines = []
    for f, linenum, line in change.RightHandSideLines():
      actual_rhs_lines.append((f.LocalPath(), linenum, line))

    f_blat = os.path.normpath('boo/blat.cc')
    f_test_expectations = os.path.normpath('foo/TestExpectations')
    expected_rhs_lines = [
        (f_blat, 1, 'This is some text'),
        (f_blat, 2, 'which lacks a copyright warning'),
        (f_blat, 3, 'but it is nonetheless interesting'),
        (f_blat, 4, 'and worthy of your attention.'),
        (f_blat, 5, 'Its freshness factor is through the roof.'),
        (f_test_expectations, 1, 'Strange to behold:'),
        (f_test_expectations, 5, 'Weasel words suggest:'),
        (f_test_expectations, 10, '   erratically,'),
        (f_test_expectations, 13, 'from this.'),
        (f_test_expectations, 14, ''),
        (f_test_expectations, 15, 'For the most part,'),
        (f_test_expectations, 16, 'I really think unified diffs'),
        (f_test_expectations, 17, 'are elegant: the way you can type'),
        (f_test_expectations, 18, 'diff --git inside/a/text inside/a/text'),
        (f_test_expectations, 19, 'or something silly like'),
        (f_test_expectations, 20, '@@ -278,6 +278,10 @@'),
        (f_test_expectations, 21, 'and have this not be interpreted'),
        (f_test_expectations, 22, 'as the start of a new file'),
        (f_test_expectations, 23, 'or anything messed up like that,'),
        (f_test_expectations, 24, 'because you parsed the header'),
        (f_test_expectations, 25, 'correctly.')]

    self.assertEquals(expected_rhs_lines, actual_rhs_lines)

  def testInvalidChange(self):
    try:
      presubmit.GitChange(
          'mychange',
          'description',
          self.fake_root_dir,
          ['foo/blat.cc', 'bar'],
          0,
          0,
          None)
      self.fail()
    except AssertionError:
      pass

  def testExecPresubmitScript(self):
    description_lines = ('Hello there',
                         'this is a change',
                         'BUG=123')
    files = [
      ['A', 'foo\\blat.cc'],
    ]
    fake_presubmit = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    self.mox.ReplayAll()

    change = presubmit.Change(
        'mychange',
        '\n'.join(description_lines),
        self.fake_root_dir,
        files,
        0,
        0,
        None)
    executer = presubmit.PresubmitExecuter(change, False, None, False)
    self.failIf(executer.ExecPresubmitScript('', fake_presubmit))
    # No error if no on-upload entry point
    self.failIf(executer.ExecPresubmitScript(
      ('def CheckChangeOnCommit(input_api, output_api):\n'
       '  return (output_api.PresubmitError("!!"))\n'),
      fake_presubmit
    ))

    executer = presubmit.PresubmitExecuter(change, True, None, False)
    # No error if no on-commit entry point
    self.failIf(executer.ExecPresubmitScript(
      ('def CheckChangeOnUpload(input_api, output_api):\n'
       '  return (output_api.PresubmitError("!!"))\n'),
      fake_presubmit
    ))

    self.failIf(executer.ExecPresubmitScript(
      ('def CheckChangeOnUpload(input_api, output_api):\n'
       '  if not input_api.change.BugsFromDescription():\n'
       '    return (output_api.PresubmitError("!!"))\n'
       '  else:\n'
       '    return ()'),
      fake_presubmit
    ))

    self.assertRaises(presubmit.PresubmitFailure,
      executer.ExecPresubmitScript,
      'def CheckChangeOnCommit(input_api, output_api):\n'
      '  return "foo"',
      fake_presubmit)

    self.assertRaises(presubmit.PresubmitFailure,
      executer.ExecPresubmitScript,
      'def CheckChangeOnCommit(input_api, output_api):\n'
      '  return ["foo"]',
      fake_presubmit)

  def testExecPresubmitScriptTemporaryFilesRemoval(self):
    self.mox.StubOutWithMock(presubmit.tempfile, 'NamedTemporaryFile')
    presubmit.tempfile.NamedTemporaryFile(delete=False).AndReturn(
        MockTemporaryFile('baz'))
    presubmit.tempfile.NamedTemporaryFile(delete=False).AndReturn(
        MockTemporaryFile('quux'))
    presubmit.os.remove('baz')
    presubmit.os.remove('quux')
    self.mox.ReplayAll()

    fake_presubmit = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    executer = presubmit.PresubmitExecuter(
        self.fake_change, False, None, False)

    self.assertEqual((), executer.ExecPresubmitScript(
      ('def CheckChangeOnUpload(input_api, output_api):\n'
       '  if len(input_api._named_temporary_files):\n'
       '    return (output_api.PresubmitError("!!"),)\n'
       '  return ()\n'),
      fake_presubmit
    ))

    result = executer.ExecPresubmitScript(
      ('def CheckChangeOnUpload(input_api, output_api):\n'
       '  with input_api.CreateTemporaryFile():\n'
       '    pass\n'
       '  with input_api.CreateTemporaryFile():\n'
       '    pass\n'
       '  return [output_api.PresubmitResult(None, f)\n'
       '          for f in input_api._named_temporary_files]\n'),
      fake_presubmit
    )
    self.assertEqual(['baz', 'quux'], [r._items for r in result])

  def testDoPresubmitChecksNoWarningsOrErrors(self):
    haspresubmit_path = presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit', 'PRESUBMIT.py')
    root_path = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    inherit_path = presubmit.os.path.join(
        self.fake_root_dir, self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(root_path).AndReturn(True)
    presubmit.os.listdir(os.path.join(
        self.fake_root_dir, 'haspresubmit')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(haspresubmit_path).AndReturn(True)
    presubmit.gclient_utils.FileRead(root_path, 'rU').AndReturn(
        self.presubmit_text)
    presubmit.gclient_utils.FileRead(haspresubmit_path, 'rU').AndReturn(
        self.presubmit_text)
    self.mox.ReplayAll()

    # Make a change which will have no warnings.
    change = self.ExampleChange(extra_lines=['STORY=http://tracker/123'])

    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=None,
        default_presubmit=None, may_prompt=False, gerrit_obj=None)
    self.failUnless(output.should_continue())
    self.assertEqual(output.getvalue().count('!!'), 0)
    self.assertEqual(output.getvalue().count('??'), 0)
    self.assertEqual(output.getvalue().count(
        'Running presubmit upload checks ...\n'), 1)

  def testDoPresubmitChecksPromptsAfterWarnings(self):
    presubmit_path = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    haspresubmit_path = presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit', 'PRESUBMIT.py')
    inherit_path = presubmit.os.path.join(
        self.fake_root_dir, self._INHERIT_SETTINGS)
    for _ in range(2):
      presubmit.os.path.isfile(inherit_path).AndReturn(False)
      presubmit.os.listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
      presubmit.os.path.isfile(presubmit_path).AndReturn(True)
      presubmit.os.listdir(presubmit.os.path.join(
          self.fake_root_dir, 'haspresubmit')).AndReturn(['PRESUBMIT.py'])
      presubmit.os.path.isfile(haspresubmit_path).AndReturn(True)
      presubmit.gclient_utils.FileRead(presubmit_path, 'rU').AndReturn(
          self.presubmit_text)
      presubmit.gclient_utils.FileRead(haspresubmit_path, 'rU').AndReturn(
          self.presubmit_text)
    presubmit.random.randint(0, 4).AndReturn(1)
    presubmit.random.randint(0, 4).AndReturn(1)
    self.mox.ReplayAll()

    # Make a change with a single warning.
    change = self.ExampleChange(extra_lines=['PROMPT_WARNING=yes'])

    input_buf = StringIO.StringIO('n\n')  # say no to the warning
    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=input_buf,
        default_presubmit=None, may_prompt=True, gerrit_obj=None)
    self.failIf(output.should_continue())
    self.assertEqual(output.getvalue().count('??'), 2)

    input_buf = StringIO.StringIO('y\n')  # say yes to the warning
    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=input_buf,
        default_presubmit=None, may_prompt=True, gerrit_obj=None)
    self.failUnless(output.should_continue())
    self.assertEquals(output.getvalue().count('??'), 2)
    self.assertEqual(output.getvalue().count(
        'Running presubmit upload checks ...\n'), 1)

  def testDoPresubmitChecksWithWarningsAndNoPrompt(self):
    presubmit_path = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    haspresubmit_path = presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit', 'PRESUBMIT.py')
    inherit_path = presubmit.os.path.join(self.fake_root_dir,
                                          self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit_path).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(haspresubmit_path).AndReturn(True)
    presubmit.gclient_utils.FileRead(presubmit_path, 'rU').AndReturn(
        self.presubmit_text)
    presubmit.gclient_utils.FileRead(haspresubmit_path, 'rU').AndReturn(
        self.presubmit_text)
    presubmit.random.randint(0, 4).AndReturn(1)
    self.mox.ReplayAll()

    change = self.ExampleChange(extra_lines=['PROMPT_WARNING=yes'])

    # There is no input buffer and may_prompt is set to False.
    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=None,
        default_presubmit=None, may_prompt=False, gerrit_obj=None)
    # A warning is printed, and should_continue is True.
    self.failUnless(output.should_continue())
    self.assertEquals(output.getvalue().count('??'), 2)
    self.assertEqual(output.getvalue().count('(y/N)'), 0)
    self.assertEqual(output.getvalue().count(
        'Running presubmit upload checks ...\n'), 1)

  def testDoPresubmitChecksNoWarningPromptIfErrors(self):
    presubmit_path = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    haspresubmit_path = presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit', 'PRESUBMIT.py')
    inherit_path = presubmit.os.path.join(
        self.fake_root_dir, self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit_path).AndReturn(True)
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(haspresubmit_path).AndReturn(True)
    presubmit.gclient_utils.FileRead(presubmit_path, 'rU'
                                     ).AndReturn(self.presubmit_text)
    presubmit.gclient_utils.FileRead(haspresubmit_path, 'rU').AndReturn(
        self.presubmit_text)
    presubmit.random.randint(0, 4).AndReturn(1)
    self.mox.ReplayAll()

    change = self.ExampleChange(extra_lines=['ERROR=yes'])
    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=None,
        default_presubmit=None, may_prompt=True, gerrit_obj=None)
    self.failIf(output.should_continue())
    self.assertEqual(output.getvalue().count('??'), 0)
    self.assertEqual(output.getvalue().count('!!'), 2)
    self.assertEqual(output.getvalue().count('(y/N)'), 0)
    self.assertEqual(output.getvalue().count(
        'Running presubmit upload checks ...\n'), 1)

  def testDoDefaultPresubmitChecksAndFeedback(self):
    always_fail_presubmit_script = """
def CheckChangeOnUpload(input_api, output_api):
  return [output_api.PresubmitError("!!")]
def CheckChangeOnCommit(input_api, output_api):
  raise Exception("Test error")
"""
    inherit_path = presubmit.os.path.join(
        self.fake_root_dir, self._INHERIT_SETTINGS)
    presubmit.os.path.isfile(inherit_path).AndReturn(False)
    presubmit.os.listdir(self.fake_root_dir).AndReturn([])
    presubmit.os.listdir(presubmit.os.path.join(
        self.fake_root_dir, 'haspresubmit')).AndReturn(['PRESUBMIT.py'])
    presubmit.os.path.isfile(presubmit.os.path.join(self.fake_root_dir,
                                  'haspresubmit',
                                  'PRESUBMIT.py')).AndReturn(False)
    presubmit.random.randint(0, 4).AndReturn(0)
    self.mox.ReplayAll()

    input_buf = StringIO.StringIO('y\n')

    change = self.ExampleChange(extra_lines=['STORY=http://tracker/123'])
    output = presubmit.DoPresubmitChecks(
        change=change, committing=False, verbose=True,
        output_stream=None, input_stream=input_buf,
        default_presubmit=always_fail_presubmit_script,
        may_prompt=False, gerrit_obj=None)
    self.failIf(output.should_continue())
    text = (
        'Running presubmit upload checks ...\n'
        'Warning, no PRESUBMIT.py found.\n'
        'Running default presubmit script.\n'
        '\n'
        '** Presubmit ERRORS **\n!!\n\n'
        'Was the presubmit check useful? If not, run "git cl presubmit -v"\n'
        'to figure out which PRESUBMIT.py was run, then run git blame\n'
        'on the file to figure out who to ask for help.\n')
    self.assertEquals(output.getvalue(), text)

  def testGetTryMastersExecuter(self):
    self.mox.ReplayAll()
    change = self.ExampleChange(
        extra_lines=['STORY=http://tracker.com/42', 'BUG=boo\n'])
    executer = presubmit.GetTryMastersExecuter()
    self.assertEqual({}, executer.ExecPresubmitScript('', '', '', change))
    self.assertEqual({},
        executer.ExecPresubmitScript('def foo():\n  return\n', '', '', change))

    expected_result = {'m1': {'s1': set(['t1', 't2'])},
                       'm2': {'s1': set(['defaulttests']),
                              's2': set(['defaulttests'])}}
    empty_result1 = {}
    empty_result2 = {'m': {}}
    space_in_name_result = {'m r': {'s\tv': set(['t1'])}}
    for result in (
        expected_result, empty_result1, empty_result2, space_in_name_result):
      self.assertEqual(
          result,
          executer.ExecPresubmitScript(
              self.presubmit_trymaster % result, '', '', change))

  def ExampleChange(self, extra_lines=None):
    """Returns an example Change instance for tests."""
    description_lines = [
      'Hello there',
      'This is a change',
    ] + (extra_lines or [])
    files = [
      ['A', presubmit.os.path.join('haspresubmit', 'blat.cc')],
    ]
    return presubmit.Change(
        name='mychange',
        description='\n'.join(description_lines),
        local_root=self.fake_root_dir,
        files=files,
        issue=0,
        patchset=0,
        author=None)

  def testMergeMasters(self):
    merge = presubmit._MergeMasters
    self.assertEqual({}, merge({}, {}))
    self.assertEqual({'m1': {}}, merge({}, {'m1': {}}))
    self.assertEqual({'m1': {}}, merge({'m1': {}}, {}))
    parts = [
      {'try1.cr': {'win': set(['defaulttests'])}},
      {'try1.cr': {'linux1': set(['test1'])},
       'try2.cr': {'linux2': set(['defaulttests'])}},
      {'try1.cr': {'mac1': set(['defaulttests']),
                   'mac2': set(['test1', 'test2']),
                   'linux1': set(['defaulttests'])}},
    ]
    expected = {
      'try1.cr': {'win': set(['defaulttests']),
                  'linux1': set(['defaulttests', 'test1']),
                  'mac1': set(['defaulttests']),
                  'mac2': set(['test1', 'test2'])},
      'try2.cr': {'linux2': set(['defaulttests'])},
    }
    for permutation in itertools.permutations(parts):
      self.assertEqual(expected, reduce(merge, permutation, {}))

  def testDoGetTryMasters(self):
    root_text = (self.presubmit_trymaster
        % '{"t1.cr": {"win": set(["defaulttests"])}}')
    linux_text = (self.presubmit_trymaster
        % ('{"t1.cr": {"linux1": set(["t1"])},'
           ' "t2.cr": {"linux2": set(["defaulttests"])}}'))

    isfile = presubmit.os.path.isfile
    listdir = presubmit.os.listdir
    FileRead = presubmit.gclient_utils.FileRead
    filename = 'foo.cc'
    filename_linux = presubmit.os.path.join('linux_only', 'penguin.cc')
    root_presubmit = presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py')
    linux_presubmit = presubmit.os.path.join(
        self.fake_root_dir, 'linux_only', 'PRESUBMIT.py')
    inherit_path = presubmit.os.path.join(
        self.fake_root_dir, self._INHERIT_SETTINGS)

    isfile(inherit_path).AndReturn(False)
    listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    isfile(root_presubmit).AndReturn(True)
    FileRead(root_presubmit, 'rU').AndReturn(root_text)

    isfile(inherit_path).AndReturn(False)
    listdir(self.fake_root_dir).AndReturn(['PRESUBMIT.py'])
    isfile(root_presubmit).AndReturn(True)
    listdir(presubmit.os.path.join(
        self.fake_root_dir, 'linux_only')).AndReturn(['PRESUBMIT.py'])
    isfile(linux_presubmit).AndReturn(True)
    FileRead(root_presubmit, 'rU').AndReturn(root_text)
    FileRead(linux_presubmit, 'rU').AndReturn(linux_text)
    self.mox.ReplayAll()

    change = presubmit.Change(
        'mychange', '', self.fake_root_dir, [], 0, 0, None)

    output = StringIO.StringIO()
    self.assertEqual({'t1.cr': {'win': ['defaulttests']}},
                     presubmit.DoGetTryMasters(change, [filename],
                                               self.fake_root_dir,
                                               None, None, False, output))
    output = StringIO.StringIO()
    expected = {
      't1.cr': {'win': ['defaulttests'], 'linux1': ['t1']},
      't2.cr': {'linux2': ['defaulttests']},
    }
    self.assertEqual(expected,
                     presubmit.DoGetTryMasters(change,
                                               [filename, filename_linux],
                                               self.fake_root_dir, None, None,
                                               False, output))

  def testMainUnversioned(self):
    # OptParser calls presubmit.os.path.exists and is a pain when mocked.
    self.UnMock(presubmit.os.path, 'exists')
    self.mox.StubOutWithMock(presubmit, 'DoPresubmitChecks')
    self.mox.StubOutWithMock(presubmit, 'ParseFiles')
    presubmit.scm.determine_scm(self.fake_root_dir).AndReturn(None)
    presubmit.ParseFiles(['random_file.txt'], None
        ).AndReturn([('M', 'random_file.txt')])
    output = self.mox.CreateMock(presubmit.PresubmitOutput)
    output.should_continue().AndReturn(False)

    presubmit.DoPresubmitChecks(mox.IgnoreArg(), False, False,
                                mox.IgnoreArg(),
                                mox.IgnoreArg(),
                                None, False, None, None, None).AndReturn(output)
    self.mox.ReplayAll()

    self.assertEquals(
        True,
        presubmit.main(['--root', self.fake_root_dir, 'random_file.txt']))

  def testMainUnversionedFail(self):
    # OptParser calls presubmit.os.path.exists and is a pain when mocked.
    self.UnMock(presubmit.os.path, 'exists')
    self.mox.StubOutWithMock(presubmit, 'DoPresubmitChecks')
    self.mox.StubOutWithMock(presubmit, 'ParseFiles')
    presubmit.scm.determine_scm(self.fake_root_dir).AndReturn(None)
    self.mox.StubOutWithMock(presubmit.sys, 'stderr')
    presubmit.sys.stderr.write(
        'Usage: presubmit_unittest.py [options] <files...>\n')
    presubmit.sys.stderr.write('\n')
    presubmit.sys.stderr.write(
        'presubmit_unittest.py: error: For unversioned directory, <files> is '
        'not optional.\n')
    self.mox.ReplayAll()

    try:
      presubmit.main(['--root', self.fake_root_dir])
      self.fail()
    except SystemExit, e:
      self.assertEquals(2, e.code)


class InputApiUnittest(PresubmitTestsBase):
  """Tests presubmit.InputApi."""
  def testMembersChanged(self):
    self.mox.ReplayAll()
    members = [
        'AbsoluteLocalPaths',
        'AffectedFiles',
        'AffectedSourceFiles',
        'AffectedTestableFiles',
        'AffectedTextFiles',
        'DEFAULT_BLACK_LIST',
        'DEFAULT_WHITE_LIST',
        'CreateTemporaryFile',
        'FilterSourceFile',
        'LocalPaths',
        'Command',
        'RunTests',
        'PresubmitLocalPath',
        'ReadFile',
        'RightHandSideLines',
        'ast',
        'basename',
        'cPickle',
        'cpplint',
        'cStringIO',
        'canned_checks',
        'change',
        'cpu_count',
        'environ',
        'fnmatch',
        'glob',
        'is_committing',
        'is_windows',
        'json',
        'logging',
        'marshal',
        'os_listdir',
        'os_walk',
        'os_path',
        'os_stat',
        'owners_db',
        'owners_finder',
        'parallel',
        'pickle',
        'platform',
        'python_executable',
        're',
        'subprocess',
        'tbr',
        'tempfile',
        'thread_pool',
        'time',
        'traceback',
        'unittest',
        'urllib2',
        'version',
        'verbose',
        'dry_run',
        'gerrit',
    ]
    # If this test fails, you should add the relevant test.
    self.compareMembers(
        presubmit.InputApi(self.fake_change, './.', False, None, False),
        members)

  def testInputApiConstruction(self):
    self.mox.ReplayAll()
    api = presubmit.InputApi(
        self.fake_change,
        presubmit_path='foo/path/PRESUBMIT.py',
        is_committing=False, gerrit_obj=None, verbose=False)
    self.assertEquals(api.PresubmitLocalPath(), 'foo/path')
    self.assertEquals(api.change, self.fake_change)

  def testInputApiPresubmitScriptFiltering(self):
    description_lines = ('Hello there',
                         'this is a change',
                         'BUG=123',
                         ' STORY =http://foo/  \t',
                         'and some more regular text')
    files = [
      ['A', presubmit.os.path.join('foo', 'blat.cc'), True],
      ['M', presubmit.os.path.join('foo', 'blat', 'READ_ME2'), True],
      ['M', presubmit.os.path.join('foo', 'blat', 'binary.dll'), True],
      ['M', presubmit.os.path.join('foo', 'blat', 'weird.xyz'), True],
      ['M', presubmit.os.path.join('foo', 'blat', 'another.h'), True],
      ['M', presubmit.os.path.join('foo', 'third_party', 'third.cc'), True],
      ['D', presubmit.os.path.join('foo', 'mat', 'beingdeleted.txt'), False],
      ['M', presubmit.os.path.join('flop', 'notfound.txt'), False],
      ['A', presubmit.os.path.join('boo', 'flap.h'), True],
    ]
    diffs = []
    for _, f, exists in files:
      full_file = presubmit.os.path.join(self.fake_root_dir, f)
      if exists and f.startswith('foo/'):
        presubmit.os.path.isfile(full_file).AndReturn(exists)
      diffs.append(self.presubmit_diffs % {'filename': f})
    presubmit.scm.GIT.GenerateDiff(
        self.fake_root_dir, branch=None, files=[], full_move=True
        ).AndReturn('\n'.join(diffs))

    self.mox.ReplayAll()

    change = presubmit.GitChange(
        'mychange',
        '\n'.join(description_lines),
        self.fake_root_dir,
        [[f[0], f[1]] for f in files],
        0,
        0,
        None)
    input_api = presubmit.InputApi(
        change,
        presubmit.os.path.join(self.fake_root_dir, 'foo', 'PRESUBMIT.py'),
        False, None, False)
    # Doesn't filter much
    got_files = input_api.AffectedFiles()
    self.assertEquals(len(got_files), 7)
    self.assertEquals(got_files[0].LocalPath(), presubmit.normpath(files[0][1]))
    self.assertEquals(got_files[1].LocalPath(), presubmit.normpath(files[1][1]))
    self.assertEquals(got_files[2].LocalPath(), presubmit.normpath(files[2][1]))
    self.assertEquals(got_files[3].LocalPath(), presubmit.normpath(files[3][1]))
    self.assertEquals(got_files[4].LocalPath(), presubmit.normpath(files[4][1]))
    self.assertEquals(got_files[5].LocalPath(), presubmit.normpath(files[5][1]))
    self.assertEquals(got_files[6].LocalPath(), presubmit.normpath(files[6][1]))
    # Ignores weird because of whitelist, third_party because of blacklist,
    # binary isn't a text file and beingdeleted doesn't exist. The rest is
    # outside foo/.
    rhs_lines = [x for x in input_api.RightHandSideLines(None)]
    self.assertEquals(len(rhs_lines), 14)
    self.assertEqual(rhs_lines[0][0].LocalPath(),
                     presubmit.normpath(files[0][1]))
    self.assertEqual(rhs_lines[3][0].LocalPath(),
                     presubmit.normpath(files[0][1]))
    self.assertEqual(rhs_lines[7][0].LocalPath(),
                     presubmit.normpath(files[4][1]))
    self.assertEqual(rhs_lines[13][0].LocalPath(),
                     presubmit.normpath(files[4][1]))

  def testInputApiFilterSourceFile(self):
    files = [
      ['A', presubmit.os.path.join('foo', 'blat.cc')],
      ['M', presubmit.os.path.join('foo', 'blat', 'READ_ME2')],
      ['M', presubmit.os.path.join('foo', 'blat', 'binary.dll')],
      ['M', presubmit.os.path.join('foo', 'blat', 'weird.xyz')],
      ['M', presubmit.os.path.join('foo', 'blat', 'another.h')],
      ['M', presubmit.os.path.join(
        'foo', 'third_party', 'WebKit', 'WebKit.cpp')],
      ['M', presubmit.os.path.join(
        'foo', 'third_party', 'WebKit2', 'WebKit2.cpp')],
      ['M', presubmit.os.path.join('foo', 'third_party', 'blink', 'blink.cc')],
      ['M', presubmit.os.path.join(
        'foo', 'third_party', 'blink1', 'blink1.cc')],
      ['M', presubmit.os.path.join('foo', 'third_party', 'third', 'third.cc')],
    ]
    for _, f, in files:
      full_file = presubmit.os.path.join(self.fake_root_dir, f)
      presubmit.os.path.isfile(full_file).AndReturn(True)

    self.mox.ReplayAll()

    change = presubmit.GitChange(
        'mychange',
        'description\nlines\n',
        self.fake_root_dir,
        [[f[0], f[1]] for f in files],
        0,
        0,
        None)
    input_api = presubmit.InputApi(
        change,
        presubmit.os.path.join(self.fake_root_dir, 'foo', 'PRESUBMIT.py'),
        False, None, False)
    # We'd like to test FilterSourceFile, which is used by
    # AffectedSourceFiles(None).
    got_files = input_api.AffectedSourceFiles(None)
    self.assertEquals(len(got_files), 4)
    # blat.cc, another.h, WebKit.cpp, and blink.cc remain.
    self.assertEquals(got_files[0].LocalPath(), presubmit.normpath(files[0][1]))
    self.assertEquals(got_files[1].LocalPath(), presubmit.normpath(files[4][1]))
    self.assertEquals(got_files[2].LocalPath(), presubmit.normpath(files[5][1]))
    self.assertEquals(got_files[3].LocalPath(), presubmit.normpath(files[7][1]))

  def testDefaultWhiteListBlackListFilters(self):
    def f(x):
      return presubmit.AffectedFile(x, 'M', self.fake_root_dir, None)
    files = [
      (
        [
          # To be tested.
          f('testing_support/google_appengine/b'),
          f('testing_support/not_google_appengine/foo.cc'),
        ],
        [
          # Expected.
          'testing_support/not_google_appengine/foo.cc',
        ],
      ),
      (
        [
          # To be tested.
          f('a/experimental/b'),
          f('experimental/b'),
          f('a/experimental'),
          f('a/experimental.cc'),
          f('a/experimental.S'),
        ],
        [
          # Expected.
          'a/experimental.cc',
          'a/experimental.S',
        ],
      ),
      (
        [
          # To be tested.
          f('a/third_party/b'),
          f('third_party/b'),
          f('a/third_party'),
          f('a/third_party.cc'),
        ],
        [
          # Expected.
          'a/third_party.cc',
        ],
      ),
      (
        [
          # To be tested.
          f('a/LOL_FILE/b'),
          f('b.c/LOL_FILE'),
          f('a/PRESUBMIT.py'),
          f('a/FOO.json'),
          f('a/FOO.java'),
          f('a/FOO.mojom'),
        ],
        [
          # Expected.
          'a/PRESUBMIT.py',
          'a/FOO.java',
          'a/FOO.mojom',
        ],
      ),
      (
        [
          # To be tested.
          f('a/.git'),
          f('b.c/.git'),
          f('a/.git/bleh.py'),
          f('.git/bleh.py'),
          f('bleh.diff'),
          f('foo/bleh.patch'),
        ],
        [
          # Expected.
        ],
      ),
    ]
    input_api = presubmit.InputApi(
        self.fake_change, './PRESUBMIT.py', False, None, False)
    self.mox.ReplayAll()

    self.assertEqual(len(input_api.DEFAULT_WHITE_LIST), 24)
    self.assertEqual(len(input_api.DEFAULT_BLACK_LIST), 12)
    for item in files:
      results = filter(input_api.FilterSourceFile, item[0])
      for i in range(len(results)):
        self.assertEquals(results[i].LocalPath(),
                          presubmit.normpath(item[1][i]))
      # Same number of expected results.
      self.assertEquals(sorted([f.LocalPath().replace(presubmit.os.sep, '/')
                                for f in results]),
                        sorted(item[1]))

  def testCustomFilter(self):
    def FilterSourceFile(affected_file):
      return 'a' in affected_file.LocalPath()
    files = [('A', 'eeaee'), ('M', 'eeabee'), ('M', 'eebcee')]
    for _, item in files:
      full_item = presubmit.os.path.join(self.fake_root_dir, item)
      presubmit.os.path.isfile(full_item).AndReturn(True)
    self.mox.ReplayAll()

    change = presubmit.GitChange(
        'mychange', '', self.fake_root_dir, files, 0, 0, None)
    input_api = presubmit.InputApi(
        change,
        presubmit.os.path.join(self.fake_root_dir, 'PRESUBMIT.py'),
        False, None, False)
    got_files = input_api.AffectedSourceFiles(FilterSourceFile)
    self.assertEquals(len(got_files), 2)
    self.assertEquals(got_files[0].LocalPath(), 'eeaee')
    self.assertEquals(got_files[1].LocalPath(), 'eeabee')

  def testLambdaFilter(self):
    white_list = presubmit.InputApi.DEFAULT_BLACK_LIST + (r".*?a.*?",)
    black_list = [r".*?b.*?"]
    files = [('A', 'eeaee'), ('M', 'eeabee'), ('M', 'eebcee'), ('M', 'eecaee')]
    for _, item in files:
      full_item = presubmit.os.path.join(self.fake_root_dir, item)
      presubmit.os.path.isfile(full_item).AndReturn(True)
    self.mox.ReplayAll()

    change = presubmit.GitChange(
        'mychange', '', self.fake_root_dir, files, 0, 0, None)
    input_api = presubmit.InputApi(
        change, './PRESUBMIT.py', False, None, False)
    # Sample usage of overiding the default white and black lists.
    got_files = input_api.AffectedSourceFiles(
        lambda x: input_api.FilterSourceFile(x, white_list, black_list))
    self.assertEquals(len(got_files), 2)
    self.assertEquals(got_files[0].LocalPath(), 'eeaee')
    self.assertEquals(got_files[1].LocalPath(), 'eecaee')

  def testGetAbsoluteLocalPath(self):
    normpath = presubmit.normpath
    # Regression test for bug of presubmit stuff that relies on invoking
    # SVN (e.g. to get mime type of file) not working unless gcl invoked
    # from the client root (e.g. if you were at 'src' and did 'cd base' before
    # invoking 'gcl upload' it would fail because svn wouldn't find the files
    # the presubmit script was asking about).
    files = [
      ['A', 'isdir'],
      ['A', presubmit.os.path.join('isdir', 'blat.cc')],
      ['M', presubmit.os.path.join('elsewhere', 'ouf.cc')],
    ]
    self.mox.ReplayAll()

    change = presubmit.Change(
        'mychange', '', self.fake_root_dir, files, 0, 0, None)
    affected_files = change.AffectedFiles()
    # Local paths should remain the same
    self.assertEquals(affected_files[0].LocalPath(), normpath('isdir'))
    self.assertEquals(affected_files[1].LocalPath(), normpath('isdir/blat.cc'))
    # Absolute paths should be prefixed
    self.assertEquals(
        affected_files[0].AbsoluteLocalPath(),
        presubmit.normpath(presubmit.os.path.join(self.fake_root_dir, 'isdir')))
    self.assertEquals(
        affected_files[1].AbsoluteLocalPath(),
        presubmit.normpath(presubmit.os.path.join(
            self.fake_root_dir, 'isdir/blat.cc')))

    # New helper functions need to work
    paths_from_change = change.AbsoluteLocalPaths()
    self.assertEqual(len(paths_from_change), 3)
    presubmit_path = presubmit.os.path.join(
        self.fake_root_dir, 'isdir', 'PRESUBMIT.py')
    api = presubmit.InputApi(
        change=change, presubmit_path=presubmit_path,
        is_committing=True, gerrit_obj=None, verbose=False)
    paths_from_api = api.AbsoluteLocalPaths()
    self.assertEqual(len(paths_from_api), 2)
    for absolute_paths in [paths_from_change, paths_from_api]:
      self.assertEqual(
          absolute_paths[0],
          presubmit.normpath(presubmit.os.path.join(
              self.fake_root_dir, 'isdir')))
      self.assertEqual(
          absolute_paths[1],
          presubmit.normpath(presubmit.os.path.join(
              self.fake_root_dir, 'isdir', 'blat.cc')))

  def testDeprecated(self):
    presubmit.warn(mox.IgnoreArg(), category=mox.IgnoreArg(), stacklevel=2)
    self.mox.ReplayAll()

    change = presubmit.Change(
        'mychange', '', self.fake_root_dir, [], 0, 0, None)
    api = presubmit.InputApi(
        change,
        presubmit.os.path.join(self.fake_root_dir, 'foo', 'PRESUBMIT.py'), True,
        None, False)
    api.AffectedTestableFiles(include_deletes=False)

  def testReadFileStringDenied(self):
    self.mox.ReplayAll()

    change = presubmit.Change(
        'foo', 'foo', self.fake_root_dir, [('M', 'AA')], 0, 0, None)
    input_api = presubmit.InputApi(
        change, presubmit.os.path.join(self.fake_root_dir, '/p'), False,
        None, False)
    self.assertRaises(IOError, input_api.ReadFile, 'boo', 'x')

  def testReadFileStringAccepted(self):
    path = presubmit.os.path.join(self.fake_root_dir, 'AA/boo')
    presubmit.gclient_utils.FileRead(path, 'x').AndReturn(None)
    self.mox.ReplayAll()

    change = presubmit.Change(
        'foo', 'foo', self.fake_root_dir, [('M', 'AA')], 0, 0, None)
    input_api = presubmit.InputApi(
        change, presubmit.os.path.join(self.fake_root_dir, '/p'), False,
        None, False)
    input_api.ReadFile(path, 'x')

  def testReadFileAffectedFileDenied(self):
    fileobj = presubmit.AffectedFile('boo', 'M', 'Unrelated',
                                     diff_cache=mox.IsA(presubmit._DiffCache))
    self.mox.ReplayAll()

    change = presubmit.Change(
        'foo', 'foo', self.fake_root_dir, [('M', 'AA')], 0, 0, None)
    input_api = presubmit.InputApi(
        change, presubmit.os.path.join(self.fake_root_dir, '/p'), False,
        None, False)
    self.assertRaises(IOError, input_api.ReadFile, fileobj, 'x')

  def testReadFileAffectedFileAccepted(self):
    fileobj = presubmit.AffectedFile('AA/boo', 'M', self.fake_root_dir,
                                     diff_cache=mox.IsA(presubmit._DiffCache))
    presubmit.gclient_utils.FileRead(fileobj.AbsoluteLocalPath(), 'x'
                                     ).AndReturn(None)
    self.mox.ReplayAll()

    change = presubmit.Change(
        'foo', 'foo', self.fake_root_dir, [('M', 'AA')], 0, 0, None)
    input_api = presubmit.InputApi(
        change, presubmit.os.path.join(self.fake_root_dir, '/p'), False,
        None, False)
    input_api.ReadFile(fileobj, 'x')

  def testCreateTemporaryFile(self):
    input_api = presubmit.InputApi(
        self.fake_change,
        presubmit_path='foo/path/PRESUBMIT.py',
        is_committing=False, gerrit_obj=None, verbose=False)
    input_api.tempfile.NamedTemporaryFile = self.mox.CreateMock(
        input_api.tempfile.NamedTemporaryFile)
    input_api.tempfile.NamedTemporaryFile(
        delete=False).AndReturn(MockTemporaryFile('foo'))
    input_api.tempfile.NamedTemporaryFile(
        delete=False).AndReturn(MockTemporaryFile('bar'))
    self.mox.ReplayAll()

    self.assertEqual(0, len(input_api._named_temporary_files))
    with input_api.CreateTemporaryFile():
      self.assertEqual(1, len(input_api._named_temporary_files))
    self.assertEqual(['foo'], input_api._named_temporary_files)
    with input_api.CreateTemporaryFile():
      self.assertEqual(2, len(input_api._named_temporary_files))
    self.assertEqual(2, len(input_api._named_temporary_files))
    self.assertEqual(['foo', 'bar'], input_api._named_temporary_files)

    self.assertRaises(TypeError, input_api.CreateTemporaryFile, delete=True)
    self.assertRaises(TypeError, input_api.CreateTemporaryFile, delete=False)
    self.assertEqual(['foo', 'bar'], input_api._named_temporary_files)


class OutputApiUnittest(PresubmitTestsBase):
  """Tests presubmit.OutputApi."""

  def testMembersChanged(self):
    self.mox.ReplayAll()
    members = [
        'AppendCC',
        'MailTextResult',
        'PresubmitError',
        'PresubmitNotifyResult',
        'PresubmitPromptWarning',
        'PresubmitPromptOrNotify',
        'PresubmitResult',
        'is_committing',
        'more_cc',
        'EnsureCQIncludeTrybotsAreAdded',
    ]
    # If this test fails, you should add the relevant test.
    self.compareMembers(presubmit.OutputApi(False), members)

  def testOutputApiBasics(self):
    self.mox.ReplayAll()
    self.failUnless(presubmit.OutputApi.PresubmitError('').fatal)
    self.failIf(presubmit.OutputApi.PresubmitError('').should_prompt)

    self.failIf(presubmit.OutputApi.PresubmitPromptWarning('').fatal)
    self.failUnless(
        presubmit.OutputApi.PresubmitPromptWarning('').should_prompt)

    self.failIf(presubmit.OutputApi.PresubmitNotifyResult('').fatal)
    self.failIf(presubmit.OutputApi.PresubmitNotifyResult('').should_prompt)

    # TODO(joi) Test MailTextResult once implemented.

  def testAppendCC(self):
    output_api = presubmit.OutputApi(False)
    output_api.AppendCC('chromium-reviews@chromium.org')
    self.assertEquals(['chromium-reviews@chromium.org'], output_api.more_cc)


  def testOutputApiHandling(self):
    self.mox.ReplayAll()

    output = presubmit.PresubmitOutput()
    presubmit.OutputApi.PresubmitError('!!!').handle(output)
    self.failIf(output.should_continue())
    self.failUnless(output.getvalue().count('!!!'))

    output = presubmit.PresubmitOutput()
    presubmit.OutputApi.PresubmitNotifyResult('?see?').handle(output)
    self.failUnless(output.should_continue())
    self.failUnless(output.getvalue().count('?see?'))

    output = presubmit.PresubmitOutput(input_stream=StringIO.StringIO('y'))
    presubmit.OutputApi.PresubmitPromptWarning('???').handle(output)
    output.prompt_yes_no('prompt: ')
    self.failUnless(output.should_continue())
    self.failUnless(output.getvalue().count('???'))

    output = presubmit.PresubmitOutput(input_stream=StringIO.StringIO('\n'))
    presubmit.OutputApi.PresubmitPromptWarning('???').handle(output)
    output.prompt_yes_no('prompt: ')
    self.failIf(output.should_continue())
    self.failUnless(output.getvalue().count('???'))

    output_api = presubmit.OutputApi(True)
    output = presubmit.PresubmitOutput(input_stream=StringIO.StringIO('y'))
    output_api.PresubmitPromptOrNotify('???').handle(output)
    output.prompt_yes_no('prompt: ')
    self.failUnless(output.should_continue())
    self.failUnless(output.getvalue().count('???'))

    output_api = presubmit.OutputApi(False)
    output = presubmit.PresubmitOutput(input_stream=StringIO.StringIO('y'))
    output_api.PresubmitPromptOrNotify('???').handle(output)
    self.failUnless(output.should_continue())
    self.failUnless(output.getvalue().count('???'))

    output_api = presubmit.OutputApi(True)
    output = presubmit.PresubmitOutput(input_stream=StringIO.StringIO('\n'))
    output_api.PresubmitPromptOrNotify('???').handle(output)
    output.prompt_yes_no('prompt: ')
    self.failIf(output.should_continue())
    self.failUnless(output.getvalue().count('???'))

  def _testIncludingCQTrybots(self, cl_text, new_trybots, updated_cl_text):
    class FakeCL(object):
      def __init__(self, description):
        self._description = description

      def GetDescription(self, force=False):
        return self._description

      def UpdateDescription(self, description, force=False):
        self._description = description

    def FakePresubmitNotifyResult(message):
      return message

    cl = FakeCL(cl_text)
    output_api = presubmit.OutputApi(False)
    output_api.PresubmitNotifyResult = FakePresubmitNotifyResult
    message = 'Automatically added optional bots to run on CQ.'
    results = output_api.EnsureCQIncludeTrybotsAreAdded(
      cl,
      new_trybots,
      message)
    self.assertEqual(updated_cl_text, cl.GetDescription())
    self.assertEqual([message], results)

  def testEnsureCQIncludeTrybotsAreAdded(self):
    # We need long lines in this test.
    # pylint: disable=line-too-long

    # Deliberately has a spaces to exercise space-stripping code.
    self._testIncludingCQTrybots(
      """A change to GPU-related code.

Cq-Include-Trybots:  master.tryserver.blink:linux_trusty_blink_rel ;luci.chromium.try:win_optional_gpu_tests_rel
""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:win_optional_gpu_tests_rel'
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:win_optional_gpu_tests_rel;master.tryserver.blink:linux_trusty_blink_rel""")

    # Starting without any CQ_INCLUDE_TRYBOTS line.
    self._testIncludingCQTrybots(
      """A change to GPU-related code.""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:mac_optional_gpu_tests_rel',
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:mac_optional_gpu_tests_rel""")

    # All pre-existing bots are already in output set.
    self._testIncludingCQTrybots(
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:win_optional_gpu_tests_rel
""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:win_optional_gpu_tests_rel'
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:win_optional_gpu_tests_rel""")

    # Equivalent tests with a pre-existing Change-Id line.
    self._testIncludingCQTrybots(
      """A change to GPU-related code.

Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:mac_optional_gpu_tests_rel',
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:mac_optional_gpu_tests_rel
Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2""")

    self._testIncludingCQTrybots(
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel
Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2
""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:win_optional_gpu_tests_rel',
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:win_optional_gpu_tests_rel
Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2""")

    self._testIncludingCQTrybots(
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_rel
Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_dbg
Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2
""",
      [
        'luci.chromium.try:linux_optional_gpu_tests_rel',
        'luci.chromium.try:win_optional_gpu_tests_rel',
      ],
      """A change to GPU-related code.

Cq-Include-Trybots: luci.chromium.try:linux_optional_gpu_tests_dbg;luci.chromium.try:linux_optional_gpu_tests_rel;luci.chromium.try:win_optional_gpu_tests_rel
Change-Id: Idaeacea9cdbe912c24c8388147a8a767c7baa5f2""")


class AffectedFileUnittest(PresubmitTestsBase):
  def testMembersChanged(self):
    self.mox.ReplayAll()
    members = [
      'AbsoluteLocalPath', 'Action', 'ChangedContents', 'DIFF_CACHE',
      'GenerateScmDiff', 'IsTestableFile', 'IsTextFile', 'LocalPath',
      'NewContents', 'OldContents',
    ]
    # If this test fails, you should add the relevant test.
    self.compareMembers(
        presubmit.AffectedFile('a', 'b', self.fake_root_dir, None), members)
    self.compareMembers(
        presubmit.GitAffectedFile('a', 'b', self.fake_root_dir, None), members)

  def testAffectedFile(self):
    path = presubmit.os.path.join('foo', 'blat.cc')
    f_path = presubmit.os.path.join(self.fake_root_dir, path)
    presubmit.gclient_utils.FileRead(f_path, 'rU').AndReturn('whatever\ncookie')
    self.mox.ReplayAll()
    af = presubmit.GitAffectedFile('foo/blat.cc', 'M', self.fake_root_dir, None)
    self.assertEquals(presubmit.normpath('foo/blat.cc'), af.LocalPath())
    self.assertEquals('M', af.Action())
    self.assertEquals(['whatever', 'cookie'], af.NewContents())

  def testAffectedFileNotExists(self):
    notfound = 'notfound.cc'
    f_notfound = presubmit.os.path.join(self.fake_root_dir, notfound)
    presubmit.gclient_utils.FileRead(f_notfound, 'rU').AndRaise(IOError)
    self.mox.ReplayAll()
    af = presubmit.AffectedFile(notfound, 'A', self.fake_root_dir, None)
    self.assertEquals([], af.NewContents())

  def testIsTestableFile(self):
    files = [
        presubmit.GitAffectedFile('foo/blat.txt', 'M', self.fake_root_dir,
                                  None),
        presubmit.GitAffectedFile('foo/binary.blob', 'M', self.fake_root_dir,
                                  None),
        presubmit.GitAffectedFile('blat/flop.txt', 'D', self.fake_root_dir,
                                  None)
    ]
    blat = presubmit.os.path.join('foo', 'blat.txt')
    blob = presubmit.os.path.join('foo', 'binary.blob')
    f_blat = presubmit.os.path.join(self.fake_root_dir, blat)
    f_blob = presubmit.os.path.join(self.fake_root_dir, blob)
    presubmit.os.path.isfile(f_blat).AndReturn(True)
    presubmit.os.path.isfile(f_blob).AndReturn(True)
    self.mox.ReplayAll()

    output = filter(lambda x: x.IsTestableFile(), files)
    self.assertEquals(2, len(output))
    self.assertEquals(files[0], output[0])


class ChangeUnittest(PresubmitTestsBase):
  def testMembersChanged(self):
    members = [
        'AbsoluteLocalPaths', 'AffectedFiles', 'AffectedTestableFiles',
        'AffectedTextFiles',
        'AllFiles', 'DescriptionText', 'FullDescriptionText',
        'LocalPaths', 'Name', 'OriginalOwnersFiles', 'RepositoryRoot',
        'RightHandSideLines', 'SetDescriptionText', 'TAG_LINE_RE',
        'BUG', 'R', 'TBR', 'BugsFromDescription',
        'ReviewersFromDescription', 'TBRsFromDescription',
        'author_email', 'issue', 'patchset', 'scm', 'tags',
    ]
    # If this test fails, you should add the relevant test.
    self.mox.ReplayAll()

    change = presubmit.Change(
        'foo', 'foo', self.fake_root_dir, [('M', 'AA')], 0, 0, 'foo')
    self.compareMembers(change, members)

  def testAffectedFiles(self):
    change = presubmit.Change(
        '', '', self.fake_root_dir, [('Y', 'AA')], 3, 5, '')
    self.assertEquals(1, len(change.AffectedFiles()))
    self.assertEquals('Y', change.AffectedFiles()[0].Action())

  def testSetDescriptionText(self):
    change = presubmit.Change(
        '', 'foo\nDRU=ro', self.fake_root_dir, [], 3, 5, '')
    self.assertEquals('foo', change.DescriptionText())
    self.assertEquals('foo\nDRU=ro', change.FullDescriptionText())
    self.assertEquals({'DRU': 'ro'}, change.tags)

    change.SetDescriptionText('WHIZ=bang\nbar\nFOO=baz')
    self.assertEquals('bar', change.DescriptionText())
    self.assertEquals('WHIZ=bang\nbar\nFOO=baz', change.FullDescriptionText())
    self.assertEquals({'WHIZ': 'bang', 'FOO': 'baz'}, change.tags)

  def testBugsFromDescription(self):
    change = presubmit.Change(
        '', 'foo\nBUG=2,1\n\nChange-Id: asdf\nBug: 3',
        self.fake_root_dir, [], 0, 0, '')
    self.assertEquals(['1', '2', '3'], change.BugsFromDescription())
    self.assertEquals('1,2,3', change.BUG)

  def testReviewersFromDescription(self):
    change = presubmit.Change(
        '', 'foo\nR=foo,bar\n\nChange-Id: asdf\nR: baz',
        self.fake_root_dir, [], 0, 0, '')
    self.assertEquals(['bar', 'foo'], change.ReviewersFromDescription())
    self.assertEquals('bar,foo', change.R)

  def testTBRsFromDescription(self):
    change = presubmit.Change(
        '', 'foo\nTBR=foo,bar\n\nChange-Id: asdf\nTBR: baz',
        self.fake_root_dir, [], 0, 0, '')
    self.assertEquals(['bar', 'baz', 'foo'], change.TBRsFromDescription())
    self.assertEquals('bar,baz,foo', change.TBR)


class CannedChecksUnittest(PresubmitTestsBase):
  """Tests presubmit_canned_checks.py."""
  def CommHelper(self, input_api, cmd, stdin=None, ret=None, **kwargs):
    ret = ret or (('', None), 0)
    kwargs.setdefault('cwd', mox.IgnoreArg())
    kwargs.setdefault('stdin', subprocess.PIPE)

    mock_process = input_api.mox.CreateMockAnything()
    mock_process.returncode = ret[1]

    input_api.PresubmitLocalPath().AndReturn(self.fake_root_dir)
    input_api.subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs
    ).AndReturn(mock_process)

    presubmit.sigint_handler.wait(mock_process, stdin).AndReturn(ret[0])

  def MockInputApi(self, change, committing):
    # pylint: disable=no-self-use
    input_api = self.mox.CreateMock(presubmit.InputApi)
    input_api.mox = self.mox
    input_api.thread_pool = presubmit.ThreadPool()
    input_api.parallel = False
    input_api.cStringIO = presubmit.cStringIO
    input_api.json = presubmit.json
    input_api.logging = logging
    input_api.os_listdir = self.mox.CreateMockAnything()
    input_api.os_walk = self.mox.CreateMockAnything()
    input_api.os_path = presubmit.os.path
    input_api.re = presubmit.re
    input_api.gerrit = self.mox.CreateMock(presubmit.GerritAccessor)
    input_api.traceback = presubmit.traceback
    input_api.urllib2 = self.mox.CreateMock(presubmit.urllib2)
    input_api.unittest = unittest
    input_api.subprocess = self.mox.CreateMock(subprocess)
    presubmit.subprocess = input_api.subprocess
    class fake_CalledProcessError(Exception):
      def __str__(self):
        return 'foo'
    input_api.subprocess.CalledProcessError = fake_CalledProcessError
    input_api.verbose = False
    input_api.is_windows = False

    input_api.change = change
    input_api.is_committing = committing
    input_api.tbr = False
    input_api.dry_run = None
    input_api.python_executable = 'pyyyyython'
    input_api.platform = sys.platform
    input_api.cpu_count = 2
    input_api.time = time
    input_api.canned_checks = presubmit_canned_checks
    input_api.Command = presubmit.CommandData
    input_api.RunTests = functools.partial(
        presubmit.InputApi.RunTests, input_api)
    presubmit.sigint_handler = self.mox.CreateMock(presubmit.SigintHandler)
    return input_api

  def testMembersChanged(self):
    self.mox.ReplayAll()
    members = [
      'DEFAULT_LINT_FILTERS',
      'BLACKLIST_LINT_FILTERS',
      'CheckAuthorizedAuthor',
      'CheckBuildbotPendingBuilds',
      'CheckChangeHasBugField', 'CheckChangeHasDescription',
      'CheckChangeHasNoStrayWhitespace',
      'CheckChangeHasOnlyOneEol', 'CheckChangeHasNoCR',
      'CheckChangeHasNoCrAndHasOnlyOneEol', 'CheckChangeHasNoTabs',
      'CheckChangeTodoHasOwner',
      'CheckChangeLintsClean',
      'CheckChangeWasUploaded',
      'CheckDoNotSubmit',
      'CheckDoNotSubmitInDescription', 'CheckDoNotSubmitInFiles',
      'CheckGenderNeutral',
      'CheckLongLines', 'CheckTreeIsOpen', 'PanProjectChecks',
      'CheckLicense',
      'CheckOwners',
      'CheckOwnersFormat',
      'CheckPatchFormatted',
      'CheckGNFormatted',
      'CheckSingletonInHeaders',
      'CheckVPythonSpec',
      'RunPythonUnitTests', 'RunPylint',
      'RunUnitTests', 'RunUnitTestsInDirectory',
      'GetCodereviewOwnerAndReviewers',
      'GetPythonUnitTests', 'GetPylint',
      'GetUnitTests', 'GetUnitTestsInDirectory', 'GetUnitTestsRecursively',
      'CheckCIPDManifest', 'CheckCIPDPackages',
      'CheckChangedLUCIConfigs',
    ]
    # If this test fails, you should add the relevant test.
    self.compareMembers(presubmit_canned_checks, members)

  def DescriptionTest(self, check, description1, description2, error_type,
                      committing):
    change1 = presubmit.Change(
        'foo1', description1, self.fake_root_dir, None, 0, 0, None)
    input_api1 = self.MockInputApi(change1, committing)
    change2 = presubmit.Change(
        'foo2', description2, self.fake_root_dir, None, 0, 0, None)
    input_api2 = self.MockInputApi(change2, committing)
    self.mox.ReplayAll()

    results1 = check(input_api1, presubmit.OutputApi)
    self.assertEquals(results1, [])
    results2 = check(input_api2, presubmit.OutputApi)
    self.assertEquals(len(results2), 1)
    self.assertEquals(results2[0].__class__, error_type)

  def ContentTest(self, check, content1, content1_path, content2,
                  content2_path, error_type):
    """Runs a test of a content-checking rule.

      Args:
        check: the check to run.
        content1: content which is expected to pass the check.
        content1_path: file path for content1.
        content2: content which is expected to fail the check.
        content2_path: file path for content2.
        error_type: the type of the error expected for content2.
    """
    change1 = presubmit.Change(
        'foo1', 'foo1\n', self.fake_root_dir, None, 0, 0, None)
    input_api1 = self.MockInputApi(change1, False)
    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    input_api1.AffectedFiles(
        include_deletes=False,
        file_filter=mox.IgnoreArg()).AndReturn([affected_file])
    affected_file.LocalPath().AndReturn(content1_path)
    affected_file.NewContents().AndReturn([
        'afoo',
        content1,
        'bfoo',
        'cfoo',
        'dfoo'])

    change2 = presubmit.Change(
        'foo2', 'foo2\n', self.fake_root_dir, None, 0, 0, None)
    input_api2 = self.MockInputApi(change2, False)

    input_api2.AffectedFiles(
        include_deletes=False,
        file_filter=mox.IgnoreArg()).AndReturn([affected_file])
    affected_file.LocalPath().AndReturn(content2_path)
    affected_file.NewContents().AndReturn([
        'dfoo',
        content2,
        'efoo',
        'ffoo',
        'gfoo'])
    # It falls back to ChangedContents when there is a failure. This is an
    # optimization since NewContents() is much faster to execute than
    # ChangedContents().
    affected_file.ChangedContents().AndReturn([
        (42, content2),
        (43, 'hfoo'),
        (23, 'ifoo')])
    affected_file.LocalPath().AndReturn('foo.cc')

    self.mox.ReplayAll()

    results1 = check(input_api1, presubmit.OutputApi, None)
    self.assertEquals(results1, [])
    results2 = check(input_api2, presubmit.OutputApi, None)
    self.assertEquals(len(results2), 1)
    self.assertEquals(results2[0].__class__, error_type)

  def ReadFileTest(self, check, content1, content2, error_type):
    change1 = presubmit.Change(
        'foo1', 'foo1\n', self.fake_root_dir, None, 0, 0, None)
    input_api1 = self.MockInputApi(change1, False)
    affected_file1 = self.mox.CreateMock(presubmit.GitAffectedFile)
    input_api1.AffectedSourceFiles(None).AndReturn([affected_file1])
    input_api1.ReadFile(affected_file1, 'rb').AndReturn(content1)
    change2 = presubmit.Change(
        'foo2', 'foo2\n', self.fake_root_dir, None, 0, 0, None)
    input_api2 = self.MockInputApi(change2, False)
    affected_file2 = self.mox.CreateMock(presubmit.GitAffectedFile)
    input_api2.AffectedSourceFiles(None).AndReturn([affected_file2])
    input_api2.ReadFile(affected_file2, 'rb').AndReturn(content2)
    affected_file2.LocalPath().AndReturn('bar.cc')
    self.mox.ReplayAll()

    results = check(input_api1, presubmit.OutputApi)
    self.assertEquals(results, [])
    results2 = check(input_api2, presubmit.OutputApi)
    self.assertEquals(len(results2), 1)
    self.assertEquals(results2[0].__class__, error_type)

  def testCannedCheckChangeHasBugField(self):
    self.DescriptionTest(presubmit_canned_checks.CheckChangeHasBugField,
                         'Foo\nBUG=1234', 'Foo\n',
                         presubmit.OutputApi.PresubmitNotifyResult,
                         False)

  def testCheckChangeHasDescription(self):
    self.DescriptionTest(presubmit_canned_checks.CheckChangeHasDescription,
                         'Bleh', '',
                         presubmit.OutputApi.PresubmitNotifyResult,
                         False)
    self.mox.VerifyAll()
    self.DescriptionTest(presubmit_canned_checks.CheckChangeHasDescription,
                         'Bleh', '',
                         presubmit.OutputApi.PresubmitError,
                         True)

  def testCannedCheckDoNotSubmitInDescription(self):
    self.DescriptionTest(presubmit_canned_checks.CheckDoNotSubmitInDescription,
                         'Foo\nDO NOTSUBMIT', 'Foo\nDO NOT ' + 'SUBMIT',
                         presubmit.OutputApi.PresubmitError,
                         False)

  def testCannedCheckDoNotSubmitInFiles(self):
    self.ContentTest(
        lambda x,y,z: presubmit_canned_checks.CheckDoNotSubmitInFiles(x, y),
        'DO NOTSUBMIT', None, 'DO NOT ' + 'SUBMIT', None,
        presubmit.OutputApi.PresubmitError)

  def testCheckChangeHasNoStrayWhitespace(self):
    self.ContentTest(
        lambda x,y,z:
            presubmit_canned_checks.CheckChangeHasNoStrayWhitespace(x, y),
        'Foo', None, 'Foo ', None,
        presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckChangeHasOnlyOneEol(self):
    self.ReadFileTest(presubmit_canned_checks.CheckChangeHasOnlyOneEol,
                      "Hey!\nHo!\n", "Hey!\nHo!\n\n",
                      presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckChangeHasNoCR(self):
    self.ReadFileTest(presubmit_canned_checks.CheckChangeHasNoCR,
                      "Hey!\nHo!\n", "Hey!\r\nHo!\r\n",
                      presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckChangeHasNoCrAndHasOnlyOneEol(self):
    self.ReadFileTest(
        presubmit_canned_checks.CheckChangeHasNoCrAndHasOnlyOneEol,
        "Hey!\nHo!\n", "Hey!\nHo!\n\n",
        presubmit.OutputApi.PresubmitPromptWarning)
    self.mox.VerifyAll()
    self.ReadFileTest(
        presubmit_canned_checks.CheckChangeHasNoCrAndHasOnlyOneEol,
        "Hey!\nHo!\n", "Hey!\r\nHo!\r\n",
        presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckChangeTodoHasOwner(self):
    self.ContentTest(presubmit_canned_checks.CheckChangeTodoHasOwner,
                     "TODO(foo): bar", None, "TODO: bar", None,
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckChangedLUCIConfigs(self):
    affected_file1 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file1.LocalPath().AndReturn('foo.cfg')
    affected_file1.NewContents().AndReturn(['test', 'foo'])
    affected_file2 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file2.LocalPath().AndReturn('bar.cfg')
    affected_file2.NewContents().AndReturn(['test', 'bar'])

    token_mock = self.mox.CreateMock(auth.AccessToken)
    token_mock.token = 123
    auth_mock = self.mox.CreateMock(auth.Authenticator)
    auth_mock.get_access_token().AndReturn(token_mock)
    self.mox.StubOutWithMock(auth, 'get_authenticator_for_host')
    auth.get_authenticator_for_host(
        mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(auth_mock)

    host = 'https://host.com'
    branch = 'branch'
    http_resp = {
      'messages': [{'severity': 'ERROR', 'text': 'deadbeef'}],
      'config_sets': [{'config_set': 'deadbeef',
                       'location': '%s/+/%s' % (host, branch)}]
    }
    self.mox.StubOutWithMock(urllib2, 'urlopen')
    urllib2.urlopen(mox.IgnoreArg()).MultipleTimes().AndReturn(http_resp)
    self.mox.StubOutWithMock(json, 'load')
    json.load(http_resp).MultipleTimes().AndReturn(http_resp)

    mock_cl = self.mox.CreateMock(git_cl.Changelist)
    mock_cl.GetRemoteBranch().AndReturn(('remote', branch))
    mock_cl.GetRemoteUrl().AndReturn(host)
    self.mox.StubOutWithMock(git_cl, 'Changelist', use_mock_anything=True)
    git_cl.Changelist().AndReturn(mock_cl)

    change1 = presubmit.Change(
      'foo', 'foo1', self.fake_root_dir, None, 0, 0, None)
    input_api = self.MockInputApi(change1, False)
    affected_files = (affected_file1, affected_file2)

    input_api.AffectedFiles = lambda: affected_files

    self.mox.ReplayAll()

    results = presubmit_canned_checks.CheckChangedLUCIConfigs(
        input_api, presubmit.OutputApi)
    self.assertEquals(len(results), 1)

  def testCannedCheckChangeHasNoTabs(self):
    self.ContentTest(presubmit_canned_checks.CheckChangeHasNoTabs,
                     'blah blah', None, 'blah\tblah', None,
                     presubmit.OutputApi.PresubmitPromptWarning)

    # Make sure makefiles are ignored.
    change1 = presubmit.Change(
        'foo1', 'foo1\n', self.fake_root_dir, None, 0, 0, None)
    input_api1 = self.MockInputApi(change1, False)
    affected_file1 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file1.LocalPath().AndReturn('foo.cc')
    affected_file2 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file2.LocalPath().AndReturn('foo/Makefile')
    affected_file3 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file3.LocalPath().AndReturn('makefile')
    # Only this one will trigger.
    affected_file4 = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file1.LocalPath().AndReturn('foo.cc')
    affected_file1.NewContents().AndReturn(['yo, '])
    affected_file4.LocalPath().AndReturn('makefile.foo')
    affected_file4.LocalPath().AndReturn('makefile.foo')
    affected_file4.NewContents().AndReturn(['ye\t'])
    affected_file4.ChangedContents().AndReturn([(46, 'ye\t')])
    affected_file4.LocalPath().AndReturn('makefile.foo')
    affected_files = (affected_file1, affected_file2,
                      affected_file3, affected_file4)

    def test(include_deletes=True, file_filter=None):
      self.assertFalse(include_deletes)
      for x in affected_files:
        if file_filter(x):
          yield x
    # Override the mock of these functions.
    input_api1.FilterSourceFile = lambda x: x
    input_api1.AffectedFiles = test
    self.mox.ReplayAll()

    results1 = presubmit_canned_checks.CheckChangeHasNoTabs(input_api1,
        presubmit.OutputApi, None)
    self.assertEquals(len(results1), 1)
    self.assertEquals(results1[0].__class__,
        presubmit.OutputApi.PresubmitPromptWarning)
    self.assertEquals(results1[0]._long_text,
        'makefile.foo:46')

  def testCannedCheckLongLines(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(check, '0123456789', None, '01234567890', None,
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckJavaLongLines(self):
    check = lambda x, y, _: presubmit_canned_checks.CheckLongLines(x, y, 80)
    self.ContentTest(check, 'A ' * 50, 'foo.java', 'A ' * 50 + 'B', 'foo.java',
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckSpecialJavaLongLines(self):
    check = lambda x, y, _: presubmit_canned_checks.CheckLongLines(x, y, 80)
    self.ContentTest(check, 'import ' + 'A ' * 150, 'foo.java',
                     'importSomething ' + 'A ' * 50, 'foo.java',
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckJSLongLines(self):
    check = lambda x, y, _: presubmit_canned_checks.CheckLongLines(x, y, 10)
    self.ContentTest(check, 'GEN(\'#include "c/b/ui/webui/fixture.h"\');',
                     'foo.js', "// GEN('something');", 'foo.js',
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckObjCExceptionLongLines(self):
    check = lambda x, y, _: presubmit_canned_checks.CheckLongLines(x, y, 80)
    self.ContentTest(check, '#import ' + 'A ' * 150, 'foo.mm',
                     'import' + 'A ' * 150, 'foo.mm',
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckMakefileLongLines(self):
    check = lambda x, y, _: presubmit_canned_checks.CheckLongLines(x, y, 80)
    self.ContentTest(check, 'A ' * 100, 'foo.mk', 'A ' * 100 + 'B', 'foo.mk',
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckLongLinesLF(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(check, '012345678\n', None, '0123456789\n', None,
                     presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckCppExceptionLongLines(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(
        check,
        '#if 56 89 12 45 9191919191919',
        'foo.cc',
        '#nif 56 89 12 45 9191919191919',
        'foo.cc',
        presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckLongLinesHttp(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(
        check,
        ' http:// 0 23 56',
        None,
        ' foob:// 0 23 56',
        None,
        presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckLongLinesFile(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(
        check,
        ' file:// 0 23 56',
        None,
        ' foob:// 0 23 56',
        None,
        presubmit.OutputApi.PresubmitPromptWarning)

  def testCannedCheckLongLinesCssUrl(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(
        check,
        ' url(some.png)',
        'foo.css',
        ' url(some.png)',
        'foo.cc',
        presubmit.OutputApi.PresubmitPromptWarning)


  def testCannedCheckLongLinesLongSymbol(self):
    check = lambda x, y, z: presubmit_canned_checks.CheckLongLines(x, y, 10, z)
    self.ContentTest(
        check,
        ' TUP5D_LoNG_SY ',
        None,
        ' TUP5D_LoNG_SY5 ',
        None,
        presubmit.OutputApi.PresubmitPromptWarning)

  def _LicenseCheck(self, text, license_text, committing, expected_result,
      **kwargs):
    change = self.mox.CreateMock(presubmit.GitChange)
    change.scm = 'svn'
    input_api = self.MockInputApi(change, committing)
    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    input_api.AffectedSourceFiles(42).AndReturn([affected_file])
    input_api.ReadFile(affected_file, 'rb').AndReturn(text)
    if expected_result:
      affected_file.LocalPath().AndReturn('bleh')

    self.mox.ReplayAll()
    result = presubmit_canned_checks.CheckLicense(
                 input_api, presubmit.OutputApi, license_text,
                 source_file_filter=42,
                 **kwargs)
    if expected_result:
      self.assertEqual(len(result), 1)
      self.assertEqual(result[0].__class__, expected_result)
    else:
      self.assertEqual(result, [])

  def testCheckLicenseSuccess(self):
    text = (
        "#!/bin/python\n"
        "# Copyright (c) 2037 Nobody.\n"
        "# All Rights Reserved.\n"
        "print 'foo'\n"
    )
    license_text = (
        r".*? Copyright \(c\) 2037 Nobody." "\n"
        r".*? All Rights Reserved\." "\n"
    )
    self._LicenseCheck(text, license_text, True, None)

  def testCheckLicenseFailCommit(self):
    text = (
        "#!/bin/python\n"
        "# Copyright (c) 2037 Nobody.\n"
        "# All Rights Reserved.\n"
        "print 'foo'\n"
    )
    license_text = (
        r".*? Copyright \(c\) 0007 Nobody." "\n"
        r".*? All Rights Reserved\." "\n"
    )
    self._LicenseCheck(text, license_text, True,
                       presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckLicenseFailUpload(self):
    text = (
        "#!/bin/python\n"
        "# Copyright (c) 2037 Nobody.\n"
        "# All Rights Reserved.\n"
        "print 'foo'\n"
    )
    license_text = (
        r".*? Copyright \(c\) 0007 Nobody." "\n"
        r".*? All Rights Reserved\." "\n"
    )
    self._LicenseCheck(text, license_text, False,
                       presubmit.OutputApi.PresubmitPromptWarning)

  def testCheckLicenseEmptySuccess(self):
    text = ''
    license_text = (
        r".*? Copyright \(c\) 2037 Nobody." "\n"
        r".*? All Rights Reserved\." "\n"
    )
    self._LicenseCheck(text, license_text, True, None, accept_empty_files=True)

  def testCannedCheckTreeIsOpenOpen(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('url_to_open').AndReturn(connection)
    connection.read().AndReturn('The tree is open')
    connection.close()
    self.mox.ReplayAll()
    results = presubmit_canned_checks.CheckTreeIsOpen(
        input_api, presubmit.OutputApi, url='url_to_open', closed='.*closed.*')
    self.assertEquals(results, [])

  def testCannedCheckTreeIsOpenClosed(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('url_to_closed').AndReturn(connection)
    connection.read().AndReturn('Tree is closed for maintenance')
    connection.close()
    self.mox.ReplayAll()
    results = presubmit_canned_checks.CheckTreeIsOpen(
        input_api, presubmit.OutputApi,
        url='url_to_closed', closed='.*closed.*')
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
                      presubmit.OutputApi.PresubmitError)

  def testCannedCheckJsonTreeIsOpenOpen(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('url_to_open').AndReturn(connection)
    status = {
        'can_commit_freely': True,
        'general_state': 'open',
        'message': 'The tree is open'
    }
    connection.read().AndReturn(input_api.json.dumps(status))
    connection.close()
    self.mox.ReplayAll()
    results = presubmit_canned_checks.CheckTreeIsOpen(
        input_api, presubmit.OutputApi, json_url='url_to_open')
    self.assertEquals(results, [])

  def testCannedCheckJsonTreeIsOpenClosed(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('url_to_closed').AndReturn(connection)
    status = {
        'can_commit_freely': False,
        'general_state': 'closed',
        'message': 'The tree is close',
    }
    connection.read().AndReturn(input_api.json.dumps(status))
    connection.close()
    self.mox.ReplayAll()
    results = presubmit_canned_checks.CheckTreeIsOpen(
        input_api, presubmit.OutputApi, json_url='url_to_closed')
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
                      presubmit.OutputApi.PresubmitError)

  def testRunPythonUnitTestsNoTest(self):
    input_api = self.MockInputApi(None, False)
    self.mox.ReplayAll()
    presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, [])
    results = input_api.thread_pool.RunAsync()
    self.assertEquals(results, [])

  def testRunPythonUnitTestsNonExistentUpload(self):
    input_api = self.MockInputApi(None, False)
    self.CommHelper(input_api, ['pyyyyython', '-m', '_non_existent_module'],
                    ret=(('foo', None), 1), env=None)
    self.mox.ReplayAll()

    results = presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, ['_non_existent_module'])
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
                      presubmit.OutputApi.PresubmitNotifyResult)

  def testRunPythonUnitTestsNonExistentCommitting(self):
    input_api = self.MockInputApi(None, True)
    self.CommHelper(input_api, ['pyyyyython', '-m', '_non_existent_module'],
                    ret=(('foo', None), 1), env=None)
    self.mox.ReplayAll()

    results = presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, ['_non_existent_module'])
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__, presubmit.OutputApi.PresubmitError)

  def testRunPythonUnitTestsFailureUpload(self):
    input_api = self.MockInputApi(None, False)
    input_api.unittest = self.mox.CreateMock(unittest)
    input_api.cStringIO = self.mox.CreateMock(presubmit.cStringIO)
    self.CommHelper(input_api, ['pyyyyython', '-m', 'test_module'],
                    ret=(('foo', None), 1), env=None)
    self.mox.ReplayAll()

    results = presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, ['test_module'])
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
                      presubmit.OutputApi.PresubmitNotifyResult)
    self.assertEquals('test_module (0.00s) failed\nfoo', results[0]._message)

  def testRunPythonUnitTestsFailureCommitting(self):
    input_api = self.MockInputApi(None, True)
    self.CommHelper(input_api, ['pyyyyython', '-m', 'test_module'],
                    ret=(('foo', None), 1), env=None)
    self.mox.ReplayAll()

    results = presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, ['test_module'])
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__, presubmit.OutputApi.PresubmitError)
    self.assertEquals('test_module (0.00s) failed\nfoo', results[0]._message)

  def testRunPythonUnitTestsSuccess(self):
    input_api = self.MockInputApi(None, False)
    input_api.cStringIO = self.mox.CreateMock(presubmit.cStringIO)
    input_api.unittest = self.mox.CreateMock(unittest)
    self.CommHelper(input_api, ['pyyyyython', '-m', 'test_module'], env=None)
    self.mox.ReplayAll()

    presubmit_canned_checks.RunPythonUnitTests(
        input_api, presubmit.OutputApi, ['test_module'])
    results = input_api.thread_pool.RunAsync()
    self.assertEquals(results, [])

  def testCannedRunPylint(self):
    input_api = self.MockInputApi(None, True)
    input_api.environ = self.mox.CreateMock(os.environ)
    input_api.environ.copy().AndReturn({})
    input_api.AffectedSourceFiles(mox.IgnoreArg()).AndReturn(True)
    input_api.PresubmitLocalPath().AndReturn('/foo')
    input_api.PresubmitLocalPath().AndReturn('/foo')
    input_api.os_walk('/foo').AndReturn([('/foo', [], ['file1.py'])])
    pylint = os.path.join(_ROOT, 'third_party', 'pylint.py')
    pylintrc = os.path.join(_ROOT, 'pylintrc')

    self.CommHelper(input_api,
        ['pyyyyython', pylint, '--args-on-stdin'],
        env=mox.IgnoreArg(), stdin=
               '--rcfile=%s\n--disable=all\n--enable=cyclic-import\nfile1.py'
               % pylintrc)
    self.CommHelper(input_api,
        ['pyyyyython', pylint, '--args-on-stdin'],
        env=mox.IgnoreArg(), stdin=
               '--rcfile=%s\n--disable=cyclic-import\n--jobs=2\nfile1.py'
               % pylintrc)
    self.mox.ReplayAll()

    results = presubmit_canned_checks.RunPylint(
        input_api, presubmit.OutputApi)

    self.assertEquals([], results)
    self.checkstdout('')

  def testCheckBuildbotPendingBuildsBad(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('uurl').AndReturn(connection)
    connection.read().AndReturn('foo')
    connection.close()
    self.mox.ReplayAll()

    results = presubmit_canned_checks.CheckBuildbotPendingBuilds(
        input_api, presubmit.OutputApi, 'uurl', 2, ('foo'))
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
        presubmit.OutputApi.PresubmitNotifyResult)

  def testCheckBuildbotPendingBuildsGood(self):
    input_api = self.MockInputApi(None, True)
    connection = self.mox.CreateMockAnything()
    input_api.urllib2.urlopen('uurl').AndReturn(connection)
    connection.read().AndReturn("""
    {
      'b1': { 'pending_builds': [0, 1, 2, 3, 4, 5, 6, 7] },
      'foo': { 'pending_builds': [0, 1, 2, 3, 4, 5, 6, 7] },
      'b2': { 'pending_builds': [0] }
    }""")
    connection.close()
    self.mox.ReplayAll()

    results = presubmit_canned_checks.CheckBuildbotPendingBuilds(
        input_api, presubmit.OutputApi, 'uurl', 2, ('foo'))
    self.assertEquals(len(results), 1)
    self.assertEquals(results[0].__class__,
        presubmit.OutputApi.PresubmitNotifyResult)

  def GetInputApiWithOWNERS(self, owners_content):
    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file.LocalPath = lambda: 'OWNERS'
    affected_file.Action = lambda: 'M'

    change = self.mox.CreateMock(presubmit.Change)
    change.AffectedFiles = lambda: [affected_file]

    input_api = self.MockInputApi(None, False)
    input_api.change = change

    os.path.exists = lambda _: True

    owners_file = presubmit.cStringIO.StringIO(owners_content)
    fopen = lambda *args: owners_file

    input_api.owners_db = owners.Database('', fopen, os.path)

    return input_api

  def testCheckOwnersFormatWorks(self):
    input_api = self.GetInputApiWithOWNERS('\n'.join([
        'set noparent',
        'per-file lalala = lemur@chromium.org',
    ]))
    self.assertEqual(
        [],
        presubmit_canned_checks.CheckOwnersFormat(
            input_api, presubmit.OutputApi)
    )

  def testCheckOwnersFormatFails(self):
    input_api = self.GetInputApiWithOWNERS('\n'.join([
        'set noparent',
        'invalid format',
    ]))
    results = presubmit_canned_checks.CheckOwnersFormat(
        input_api, presubmit.OutputApi)

    self.assertEqual(1, len(results))
    self.assertIsInstance(results[0], presubmit.OutputApi.PresubmitError)

  def AssertOwnersWorks(self, tbr=False, issue='1', approvers=None,
      reviewers=None, is_committing=True,
      response=None, uncovered_files=None, expected_output='',
      manually_specified_reviewers=None, dry_run=None,
      modified_file='foo/xyz.cc'):
    if approvers is None:
      # The set of people who lgtm'ed a change.
      approvers = set()
    if reviewers is None:
      # The set of people needed to lgtm a change. We default to
      # the same list as the people who approved it. We use 'reviewers'
      # to avoid a name collision w/ owners.py.
      reviewers = approvers
    if uncovered_files is None:
      uncovered_files = set()
    if manually_specified_reviewers is None:
      manually_specified_reviewers = []

    change = self.mox.CreateMock(presubmit.Change)
    change.issue = issue
    change.author_email = 'john@example.com'
    change.ReviewersFromDescription = lambda: manually_specified_reviewers
    change.TBRsFromDescription = lambda: []
    change.RepositoryRoot = lambda: None
    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    input_api = self.MockInputApi(change, False)
    input_api.gerrit = presubmit.GerritAccessor('host')

    fake_db = self.mox.CreateMock(owners.Database)
    fake_db.email_regexp = input_api.re.compile(owners.BASIC_EMAIL_REGEXP)
    input_api.owners_db = fake_db

    fake_finder = self.mox.CreateMock(owners_finder.OwnersFinder)
    fake_finder.unreviewed_files = uncovered_files
    fake_finder.print_indent = lambda: ''
    # pylint: disable=unnecessary-lambda
    fake_finder.print_comments = lambda owner: fake_finder.writeln(owner)
    input_api.owners_finder = lambda *args, **kwargs: fake_finder
    input_api.is_committing = is_committing
    input_api.tbr = tbr
    input_api.dry_run = dry_run

    affected_file.LocalPath().AndReturn(modified_file)
    change.AffectedFiles(file_filter=None).AndReturn([affected_file])
    if not is_committing or (not tbr and issue) or ('OWNERS' in modified_file):
      change.OriginalOwnersFiles().AndReturn({})
      if issue and not response:
        response = {
          "owner": {"email": change.author_email},
          "labels": {"Code-Review": {
            u'all': [
              {
                u'email': a,
                u'value': +1
              } for a in approvers
            ],
            u'default_value': 0,
            u'values': {u' 0': u'No score',
                        u'+1': u'Looks good to me',
                        u'-1': u"I would prefer that you didn't submit this"}
          }},
          "reviewers": {"REVIEWER": [{u'email': a}] for a in approvers},
        }

      if is_committing:
        people = approvers
      else:
        people = reviewers

      if issue:
        input_api.gerrit._FetchChangeDetail = lambda _: response

      people.add(change.author_email)
      change.OriginalOwnersFiles().AndReturn({})
      if not is_committing and uncovered_files:
        fake_db.reviewers_for(set(['foo']),
            change.author_email).AndReturn([change.author_email])

    self.mox.ReplayAll()
    output = presubmit.PresubmitOutput()
    results = presubmit_canned_checks.CheckOwners(input_api,
        presubmit.OutputApi)
    for result in results:
      result.handle(output)
    if isinstance(expected_output, re._pattern_type):
      self.assertRegexpMatches(output.getvalue(), expected_output)
    else:
      self.assertEquals(output.getvalue(), expected_output)

  def testCannedCheckOwners_DryRun(self):
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'all': [
          {
            u'email': u'ben@example.com',
            u'value': 0
          },
        ],
        u'approved': {u'email': u'ben@example.org'},
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me',
                    u'-1': u"I would prefer that you didn't submit this"}
      }},
      "reviewers": {"REVIEWER": [{u'email': u'ben@example.com'}]},
    }
    self.AssertOwnersWorks(approvers=set(),
        dry_run=True,
        response=response,
        reviewers=set(["ben@example.com"]),
        expected_output='This is a dry run, but these failures would be ' +
                        'reported on commit:\nMissing LGTM from someone ' +
                        'other than john@example.com\n')

    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
        is_committing=False,
        response=response,
        expected_output='')

  def testCannedCheckOwners_Approved(self):
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'all': [
          {
            u'email': u'john@example.com',  # self +1 :)
            u'value': 1
          },
          {
            u'email': u'ben@example.com',
            u'value': 2
          },
        ],
        u'approved': {u'email': u'ben@example.org'},
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me, but someone else must approve',
                    u'+2': u'Looks good to me, approved',
                    u'-1': u"I would prefer that you didn't submit this",
                    u'-2': u'Do not submit'}
      }},
      "reviewers": {"REVIEWER": [{u'email': u'ben@example.com'}]},
    }
    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
        response=response,
        is_committing=True,
        expected_output='')

    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
        is_committing=False,
        response=response,
        expected_output='')

    # Testing configuration with on -1..+1.
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'all': [
          {
            u'email': u'ben@example.com',
            u'value': 1
          },
        ],
        u'approved': {u'email': u'ben@example.org'},
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me',
                    u'-1': u"I would prefer that you didn't submit this"}
      }},
      "reviewers": {"REVIEWER": [{u'email': u'ben@example.com'}]},
    }
    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
        response=response,
        is_committing=True,
        expected_output='')

  def testCannedCheckOwners_NotApproved(self):
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'all': [
          {
            u'email': u'john@example.com',  # self +1 :)
            u'value': 1
          },
          {
            u'email': u'ben@example.com',
            u'value': 1
          },
        ],
        u'approved': {u'email': u'ben@example.org'},
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me, but someone else must approve',
                    u'+2': u'Looks good to me, approved',
                    u'-1': u"I would prefer that you didn't submit this",
                    u'-2': u'Do not submit'}
      }},
      "reviewers": {"REVIEWER": [{u'email': u'ben@example.com'}]},
    }
    self.AssertOwnersWorks(
        approvers=set(),
        reviewers=set(["ben@example.com"]),
        response=response,
        is_committing=True,
        expected_output=
            'Missing LGTM from someone other than john@example.com\n')

    self.AssertOwnersWorks(
        approvers=set(),
        reviewers=set(["ben@example.com"]),
        is_committing=False,
        response=response,
        expected_output='')

    # Testing configuration with on -1..+1.
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'all': [
          {
            u'email': u'ben@example.com',
            u'value': 0
          },
        ],
        u'approved': {u'email': u'ben@example.org'},
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me',
                    u'-1': u"I would prefer that you didn't submit this"}
      }},
      "reviewers": {"REVIEWER": [{u'email': u'ben@example.com'}]},
    }
    self.AssertOwnersWorks(
        approvers=set(),
        reviewers=set(["ben@example.com"]),
        response=response,
        is_committing=True,
        expected_output=
            'Missing LGTM from someone other than john@example.com\n')

  def testCannedCheckOwners_NoReviewers(self):
    response = {
      "owner": {"email": "john@example.com"},
      "labels": {"Code-Review": {
        u'default_value': 0,
        u'values': {u' 0': u'No score',
                    u'+1': u'Looks good to me',
                    u'-1': u"I would prefer that you didn't submit this"}
      }},
      "reviewers": {},
    }
    self.AssertOwnersWorks(
        approvers=set(),
        reviewers=set(),
        response=response,
        expected_output=
            'Missing LGTM from someone other than john@example.com\n')

    self.AssertOwnersWorks(
        approvers=set(),
        reviewers=set(),
        is_committing=False,
        response=response,
        expected_output='')

  def testCannedCheckOwners_NoIssueNoFiles(self):
    self.AssertOwnersWorks(issue=None,
        expected_output="OWNERS check failed: this CL has no Gerrit "
                        "change number, so we can't check it for approvals.\n")
    self.AssertOwnersWorks(issue=None, is_committing=False,
        expected_output="")

  def testCannedCheckOwners_NoIssue(self):
    self.AssertOwnersWorks(issue=None,
        uncovered_files=set(['foo']),
        expected_output="OWNERS check failed: this CL has no Gerrit "
                        "change number, so we can't check it for approvals.\n")
    self.AssertOwnersWorks(issue=None,
        is_committing=False,
        uncovered_files=set(['foo']),
        expected_output=re.compile(
            'Missing OWNER reviewers for these files:\n'
            '    foo\n', re.MULTILINE))

  def testCannedCheckOwners_NoIssueLocalReviewers(self):
    self.AssertOwnersWorks(issue=None,
        reviewers=set(['jane@example.com']),
        manually_specified_reviewers=['jane@example.com'],
        expected_output="OWNERS check failed: this CL has no Gerrit "
                        "change number, so we can't check it for approvals.\n")
    self.AssertOwnersWorks(issue=None,
        reviewers=set(['jane@example.com']),
        manually_specified_reviewers=['jane@example.com'],
        is_committing=False,
        expected_output='')

  def testCannedCheckOwners_NoIssueLocalReviewersDontInferEmailDomain(self):
    self.AssertOwnersWorks(issue=None,
        reviewers=set(['jane']),
        manually_specified_reviewers=['jane@example.com'],
        expected_output="OWNERS check failed: this CL has no Gerrit "
                        "change number, so we can't check it for approvals.\n")
    self.AssertOwnersWorks(issue=None,
        uncovered_files=set(['foo']),
        manually_specified_reviewers=['jane'],
        is_committing=False,
        expected_output=re.compile(
            'Missing OWNER reviewers for these files:\n'
            '    foo\n', re.MULTILINE))

  def testCannedCheckOwners_NoLGTM(self):
    self.AssertOwnersWorks(expected_output='Missing LGTM from someone '
                                           'other than john@example.com\n')
    self.AssertOwnersWorks(is_committing=False, expected_output='')

  def testCannedCheckOwners_OnlyOwnerLGTM(self):
    self.AssertOwnersWorks(approvers=set(['john@example.com']),
                           expected_output='Missing LGTM from someone '
                                           'other than john@example.com\n')
    self.AssertOwnersWorks(approvers=set(['john@example.com']),
                           is_committing=False,
                           expected_output='')

  def testCannedCheckOwners_TBR(self):
    self.AssertOwnersWorks(tbr=True,
        expected_output='--tbr was specified, skipping OWNERS check\n')
    self.AssertOwnersWorks(tbr=True, is_committing=False, expected_output='')

  def testCannedCheckOwners_TBROWNERSFile(self):
    self.AssertOwnersWorks(
        tbr=True, uncovered_files=set(['foo']),
        modified_file='foo/OWNERS',
        expected_output=re.compile(
            'Missing LGTM from an OWNER for these files:\n'
            '    foo\n'
            '.*TBR does not apply to changes that affect OWNERS files.',
            re.MULTILINE))

  def testCannedCheckOwners_WithoutOwnerLGTM(self):
    self.AssertOwnersWorks(uncovered_files=set(['foo']),
        expected_output='Missing LGTM from an OWNER for these files:\n'
                        '    foo\n')
    self.AssertOwnersWorks(uncovered_files=set(['foo']),
        is_committing=False,
        expected_output=re.compile(
            'Missing OWNER reviewers for these files:\n'
            '    foo\n', re.MULTILINE))

  def testCannedCheckOwners_WithLGTMs(self):
    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
                           uncovered_files=set())
    self.AssertOwnersWorks(approvers=set(['ben@example.com']),
                           is_committing=False,
                           uncovered_files=set())

  def testCannedRunUnitTests(self):
    change = presubmit.Change(
        'foo1', 'description1', self.fake_root_dir, None, 0, 0, None)
    input_api = self.MockInputApi(change, False)
    input_api.verbose = True
    unit_tests = ['allo', 'bar.py']
    cmd = ['bar.py', '--verbose']
    if input_api.platform == 'win32':
      cmd.insert(0, 'vpython.bat')
    else:
      cmd.insert(0, 'vpython')
    self.CommHelper(input_api, cmd, cwd=self.fake_root_dir, ret=(('', None), 1))
    self.CommHelper(input_api, ['allo', '--verbose'], cwd=self.fake_root_dir)

    self.mox.ReplayAll()
    results = presubmit_canned_checks.RunUnitTests(
        input_api,
        presubmit.OutputApi,
        unit_tests)
    self.assertEqual(2, len(results))
    self.assertEqual(
        presubmit.OutputApi.PresubmitNotifyResult, results[1].__class__)
    self.assertEqual(
        presubmit.OutputApi.PresubmitPromptWarning, results[0].__class__)
    self.checkstdout('')

  def testCannedRunUnitTestsInDirectory(self):
    change = presubmit.Change(
        'foo1', 'description1', self.fake_root_dir, None, 0, 0, None)
    input_api = self.MockInputApi(change, False)
    input_api.verbose = True
    input_api.logging = self.mox.CreateMock(logging)
    input_api.PresubmitLocalPath().AndReturn(self.fake_root_dir)
    path = presubmit.os.path.join(self.fake_root_dir, 'random_directory')
    input_api.os_listdir(path).AndReturn(['.', '..', 'a', 'b', 'c'])
    input_api.os_path.isfile = lambda x: not x.endswith('.')
    self.CommHelper(
        input_api,
        [presubmit.os.path.join('random_directory', 'b'), '--verbose'],
        cwd=self.fake_root_dir)
    input_api.logging.debug('Found 5 files, running 1 unit tests')

    self.mox.ReplayAll()
    results = presubmit_canned_checks.RunUnitTestsInDirectory(
        input_api,
        presubmit.OutputApi,
        'random_directory',
        whitelist=['^a$', '^b$'],
        blacklist=['a'])
    self.assertEqual(1, len(results))
    self.assertEqual(
        presubmit.OutputApi.PresubmitNotifyResult, results[0].__class__)
    self.checkstdout('')

  def testPanProjectChecks(self):
    # Make sure it accepts both list and tuples.
    change = presubmit.Change(
        'foo1', 'description1', self.fake_root_dir, None, 0, 0, None)
    input_api = self.MockInputApi(change, False)
    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    for _ in range(3):
      input_api.AffectedFiles(file_filter=mox.IgnoreArg(), include_deletes=False
          ).AndReturn([affected_file])
      affected_file.LocalPath()
      affected_file.NewContents().AndReturn('Hey!\nHo!\nHey!\nHo!\n\n')
    # CheckChangeHasNoTabs() calls _FindNewViolationsOfRule() which calls
    # ChangedContents().
    affected_file.ChangedContents().AndReturn([
        (0, 'Hey!\n'),
        (1, 'Ho!\n'),
        (2, 'Hey!\n'),
        (3, 'Ho!\n'),
        (4, '\n')])
    for _ in range(5):  # One for each ChangedContents().
      affected_file.LocalPath().AndReturn('hello.py')
    # CheckingLicense() calls AffectedSourceFiles() instead of AffectedFiles().
    input_api.AffectedSourceFiles(mox.IgnoreArg()).AndReturn([affected_file])
    input_api.ReadFile(affected_file, 'rb').AndReturn(
        'Hey!\nHo!\nHey!\nHo!\n\n')
    affected_file.LocalPath()

    self.mox.ReplayAll()
    results = presubmit_canned_checks.PanProjectChecks(
        input_api,
        presubmit.OutputApi,
        excluded_paths=None,
        text_files=None,
        license_header=None,
        project_name=None,
        owners_check=False)
    self.assertEqual(2, len(results))
    self.assertEqual(
        'Found line ending with white spaces in:', results[0]._message)
    self.checkstdout('')

  def testCheckCIPDManifest_file(self):
    input_api = self.MockInputApi(None, False)
    self.mox.ReplayAll()

    command = presubmit_canned_checks.CheckCIPDManifest(
        input_api, presubmit.OutputApi, path='/path/to/foo')
    self.assertEquals(command.cmd,
        ['cipd', 'ensure-file-verify', '-ensure-file', '/path/to/foo'])
    self.assertEquals(command.kwargs, {
        'stdin': subprocess.PIPE,
        'stdout': subprocess.PIPE,
        'stderr': subprocess.STDOUT,
    })

  def testCheckCIPDManifest_content(self):
    input_api = self.MockInputApi(None, False)
    input_api.verbose = True
    self.mox.ReplayAll()

    command = presubmit_canned_checks.CheckCIPDManifest(
        input_api, presubmit.OutputApi, content='manifest_content')
    self.assertEquals(command.cmd,
        ['cipd', 'ensure-file-verify', '-log-level', 'debug', '-ensure-file=-'])
    self.assertEquals(command.stdin, 'manifest_content')
    self.assertEquals(command.kwargs, {
        'stdin': subprocess.PIPE,
        'stdout': subprocess.PIPE,
        'stderr': subprocess.STDOUT,
    })

  def testCheckCIPDPackages(self):
    content = '\n'.join([
        '$VerifiedPlatform foo-bar',
        '$VerifiedPlatform baz-qux',
        'foo/bar/baz/${platform} version:ohaithere',
        'qux version:kthxbye',
    ])

    input_api = self.MockInputApi(None, False)
    self.mox.ReplayAll()

    command = presubmit_canned_checks.CheckCIPDPackages(
        input_api, presubmit.OutputApi,
        platforms=['foo-bar', 'baz-qux'],
        packages={
          'foo/bar/baz/${platform}': 'version:ohaithere',
          'qux': 'version:kthxbye',
        })
    self.assertEquals(command.cmd,
        ['cipd', 'ensure-file-verify', '-ensure-file=-'])
    self.assertEquals(command.stdin, content)
    self.assertEquals(command.kwargs, {
        'stdin': subprocess.PIPE,
        'stdout': subprocess.PIPE,
        'stderr': subprocess.STDOUT,
    })

  def testCannedCheckVPythonSpec(self):
    change = presubmit.Change('a', 'b', self.fake_root_dir, None, 0, 0, None)
    input_api = self.MockInputApi(change, False)

    affected_file = self.mox.CreateMock(presubmit.GitAffectedFile)
    affected_file.AbsoluteLocalPath().AndReturn('/path1/to/.vpython')
    input_api.AffectedTestableFiles(
        file_filter=mox.IgnoreArg()).AndReturn([affected_file])

    self.mox.ReplayAll()

    commands = presubmit_canned_checks.CheckVPythonSpec(
        input_api, presubmit.OutputApi)
    self.assertEqual(len(commands), 1)
    self.assertEqual(commands[0].name, 'Verify /path1/to/.vpython')
    self.assertEqual(commands[0].cmd, [
      'vpython',
      '-vpython-spec', '/path1/to/.vpython',
      '-vpython-tool', 'verify'
    ])
    self.assertDictEqual(
        commands[0].kwargs,
        {
            'stderr': input_api.subprocess.STDOUT,
            'stdout': input_api.subprocess.PIPE,
            'stdin': input_api.subprocess.PIPE,
        })
    self.assertEqual(commands[0].message, presubmit.OutputApi.PresubmitError)
    self.assertIsNone(commands[0].info)


if __name__ == '__main__':
  import unittest
  unittest.main()
