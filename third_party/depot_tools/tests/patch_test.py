#!/usr/bin/env python
# coding: utf-8
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for patch.py."""

import logging
import os
import posixpath
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testing_support.patches_data import GIT, RAW

import patch


class PatchTest(unittest.TestCase):
  def _check_patch(self,
      p,
      filename,
      diff,
      source_filename=None,
      is_binary=False,
      is_delete=False,
      is_git_diff=False,
      is_new=False,
      patchlevel=0,
      svn_properties=None,
      nb_hunks=None):
    self.assertEquals(p.filename, filename)
    self.assertEquals(p.source_filename, source_filename)
    self.assertEquals(p.is_binary, is_binary)
    self.assertEquals(p.is_delete, is_delete)
    if hasattr(p, 'is_git_diff'):
      self.assertEquals(p.is_git_diff, is_git_diff)
    self.assertEquals(p.is_new, is_new)
    if hasattr(p, 'patchlevel'):
      self.assertEquals(p.patchlevel, patchlevel)
    if diff:
      if is_binary:
        self.assertEquals(p.get(), diff)
      else:
        self.assertEquals(p.get(True), diff)
    if hasattr(p, 'hunks'):
      self.assertEquals(len(p.hunks), nb_hunks)
    else:
      self.assertEquals(None, nb_hunks)
    if hasattr(p, 'svn_properties'):
      self.assertEquals(p.svn_properties, svn_properties or [])

  def testFilePatchDelete(self):
    p = patch.FilePatchDelete('foo', False)
    self._check_patch(p, 'foo', None, is_delete=True)

  def testFilePatchDeleteBin(self):
    p = patch.FilePatchDelete('foo', True)
    self._check_patch(p, 'foo', None, is_delete=True, is_binary=True)

  def testFilePatchBinary(self):
    p = patch.FilePatchBinary('foo', 'data', [], is_new=False)
    self._check_patch(p, 'foo', 'data', is_binary=True)

  def testFilePatchBinaryNew(self):
    p = patch.FilePatchBinary('foo', 'data', [], is_new=True)
    self._check_patch(p, 'foo', 'data', is_binary=True, is_new=True)

  def testFilePatchDiff(self):
    p = patch.FilePatchDiff('chrome/file.cc', RAW.PATCH, [])
    self._check_patch(p, 'chrome/file.cc', RAW.PATCH, nb_hunks=1)

  def testDifferent(self):
    name = 'master/unittests/data/processes-summary.dat'
    p = patch.FilePatchDiff(name, RAW.DIFFERENT, [])
    self._check_patch(p, name, RAW.DIFFERENT, nb_hunks=1)

  def testFilePatchDiffHeaderMode(self):
    p = patch.FilePatchDiff('git_cl/git-cl', GIT.MODE_EXE, [])
    self._check_patch(
        p, 'git_cl/git-cl', GIT.MODE_EXE, is_git_diff=True, patchlevel=1,
        svn_properties=[('svn:executable', '.')], nb_hunks=0)

  def testFilePatchDiffHeaderModeIndex(self):
    p = patch.FilePatchDiff('git_cl/git-cl', GIT.MODE_EXE_JUNK, [])
    self._check_patch(
        p, 'git_cl/git-cl', GIT.MODE_EXE_JUNK, is_git_diff=True, patchlevel=1,
        svn_properties=[('svn:executable', '.')], nb_hunks=0)

  def testFilePatchDiffHeaderNotExecutable(self):
    p = patch.FilePatchDiff(
        'build/android/ant/create.js', GIT.NEW_NOT_EXECUTABLE, [])
    self._check_patch(
        p, 'build/android/ant/create.js', GIT.NEW_NOT_EXECUTABLE,
        is_git_diff=True, patchlevel=1, is_new=True,
        nb_hunks=1)

  def testFilePatchDiffSvnNew(self):
    # The code path is different for git and svn.
    p = patch.FilePatchDiff('foo', RAW.NEW, [])
    self._check_patch(p, 'foo', RAW.NEW, is_new=True, nb_hunks=1)

  def testFilePatchDiffGitNew(self):
    # The code path is different for git and svn.
    p = patch.FilePatchDiff('foo', GIT.NEW, [])
    self._check_patch(
        p, 'foo', GIT.NEW, is_new=True, is_git_diff=True, patchlevel=1,
        nb_hunks=1)

  def testSvn(self):
    # Should not throw.
    p = patch.FilePatchDiff('chrome/file.cc', RAW.PATCH, [])
    lines = RAW.PATCH.splitlines(True)
    header = ''.join(lines[:4])
    hunks = ''.join(lines[4:])
    self.assertEquals(header, p.diff_header)
    self.assertEquals(hunks, p.diff_hunks)
    self.assertEquals(RAW.PATCH, p.get(True))
    self.assertEquals(RAW.PATCH, p.get(False))

  def testSvnNew(self):
    p = patch.FilePatchDiff('chrome/file.cc', RAW.MINIMAL_NEW, [])
    self.assertEquals(RAW.MINIMAL_NEW, p.diff_header)
    self.assertEquals('', p.diff_hunks)
    self.assertEquals(RAW.MINIMAL_NEW, p.get(True))
    self.assertEquals(RAW.MINIMAL_NEW, p.get(False))

  def testSvnDelete(self):
    p = patch.FilePatchDiff('chrome/file.cc', RAW.MINIMAL_DELETE, [])
    self.assertEquals(RAW.MINIMAL_DELETE, p.diff_header)
    self.assertEquals('', p.diff_hunks)
    self.assertEquals(RAW.MINIMAL_DELETE, p.get(True))
    self.assertEquals(RAW.MINIMAL_DELETE, p.get(False))

  def testSvnRename(self):
    p = patch.FilePatchDiff('file_b', RAW.MINIMAL_RENAME, [])
    self.assertEquals(RAW.MINIMAL_RENAME, p.diff_header)
    self.assertEquals('', p.diff_hunks)
    self.assertEquals(RAW.MINIMAL_RENAME, p.get(True))
    self.assertEquals('--- file_b\n+++ file_b\n', p.get(False))

  def testRelPath(self):
    patches = patch.PatchSet([
        patch.FilePatchDiff('pp', GIT.COPY, []),
        patch.FilePatchDiff(
            'chromeos\\views/webui_menu_widget.h', GIT.RENAME_PARTIAL, []),
        patch.FilePatchDiff('tools/run_local_server.sh', GIT.RENAME, []),
        patch.FilePatchBinary('bar', 'data', [], is_new=False),
        patch.FilePatchDiff('chrome/file.cc', RAW.PATCH, []),
        patch.FilePatchDiff('foo', GIT.NEW, []),
        patch.FilePatchDelete('other/place/foo', True),
        patch.FilePatchDiff(
            'tools\\clang_check/README.chromium', GIT.DELETE, []),
    ])
    expected = [
        'pp',
        'chromeos/views/webui_menu_widget.h',
        'tools/run_local_server.sh',
        'bar',
        'chrome/file.cc',
        'foo',
        'other/place/foo',
        'tools/clang_check/README.chromium',
    ]
    self.assertEquals(expected, patches.filenames)

    # Test patch #4.
    orig_name = patches.patches[4].filename
    orig_source_name = patches.patches[4].source_filename or orig_name
    patches.set_relpath(os.path.join('a', 'bb'))
    # Expect posixpath all the time.
    expected = [posixpath.join('a', 'bb', x) for x in expected]
    self.assertEquals(expected, patches.filenames)
    # Make sure each header is updated accordingly.
    header = []
    new_name = posixpath.join('a', 'bb', orig_name)
    new_source_name = posixpath.join('a', 'bb', orig_source_name)
    for line in RAW.PATCH.splitlines(True):
      if line.startswith('@@'):
        break
      if line[:3] == '---':
        line = line.replace(orig_source_name, new_source_name)
      if line[:3] == '+++':
        line = line.replace(orig_name, new_name)
      header.append(line)
    header = ''.join(header)
    self.assertEquals(header, patches.patches[4].diff_header)

  def testRelPathEmpty(self):
    patches = patch.PatchSet([
        patch.FilePatchDiff('chrome\\file.cc', RAW.PATCH, []),
        patch.FilePatchDelete('other\\place\\foo', True),
    ])
    patches.set_relpath('')
    self.assertEquals(
        ['chrome/file.cc', 'other/place/foo'],
        [f.filename for f in patches])
    self.assertEquals([None, None], [f.source_filename for f in patches])

  def testBackSlash(self):
    mangled_patch = RAW.PATCH.replace('chrome/', 'chrome\\')
    patches = patch.PatchSet([
        patch.FilePatchDiff('chrome\\file.cc', mangled_patch, []),
        patch.FilePatchDelete('other\\place\\foo', True),
    ])
    expected = ['chrome/file.cc', 'other/place/foo']
    self.assertEquals(expected, patches.filenames)
    self.assertEquals(RAW.PATCH, patches.patches[0].get(True))
    self.assertEquals(RAW.PATCH, patches.patches[0].get(False))

  def testTwoHunks(self):
    name = 'chrome/app/generated_resources.grd'
    p = patch.FilePatchDiff(name, RAW.TWO_HUNKS, [])
    self._check_patch(p, name, RAW.TWO_HUNKS, nb_hunks=2)

  def testGitThreeHunks(self):
    p = patch.FilePatchDiff('presubmit_support.py', GIT.FOUR_HUNKS, [])
    self._check_patch(
        p, 'presubmit_support.py', GIT.FOUR_HUNKS, is_git_diff=True,
        patchlevel=1,
        nb_hunks=4)

  def testDelete(self):
    p = patch.FilePatchDiff('tools/clang_check/README.chromium', RAW.DELETE, [])
    self._check_patch(
        p, 'tools/clang_check/README.chromium', RAW.DELETE, is_delete=True,
        nb_hunks=1)

  def testDelete2(self):
    name = 'browser/extensions/extension_sidebar_api.cc'
    p = patch.FilePatchDiff(name, RAW.DELETE2, [])
    self._check_patch(p, name, RAW.DELETE2, is_delete=True, nb_hunks=1)

  def testGitDelete(self):
    p = patch.FilePatchDiff('tools/clang_check/README.chromium', GIT.DELETE, [])
    self._check_patch(
        p, 'tools/clang_check/README.chromium', GIT.DELETE, is_delete=True,
        is_git_diff=True, patchlevel=1, nb_hunks=1)

  def testGitRename(self):
    p = patch.FilePatchDiff('tools/run_local_server.sh', GIT.RENAME, [])
    self._check_patch(
        p,
        'tools/run_local_server.sh',
        GIT.RENAME,
        is_git_diff=True,
        patchlevel=1,
        source_filename='tools/run_local_server.PY',
        is_new=True,
        nb_hunks=0)

  def testGitRenamePartial(self):
    p = patch.FilePatchDiff(
        'chromeos/views/webui_menu_widget.h', GIT.RENAME_PARTIAL, [])
    self._check_patch(
        p,
        'chromeos/views/webui_menu_widget.h',
        GIT.RENAME_PARTIAL,
        source_filename='chromeos/views/DOMui_menu_widget.h',
        is_git_diff=True,
        patchlevel=1,
        is_new=True,
        nb_hunks=1)

  def testGitCopy(self):
    p = patch.FilePatchDiff('pp', GIT.COPY, [])
    self._check_patch(
        p, 'pp', GIT.COPY, is_git_diff=True, patchlevel=1,
        source_filename='PRESUBMIT.py', is_new=True, nb_hunks=0)

  def testOnlyHeader(self):
    p = patch.FilePatchDiff('file_a', RAW.MINIMAL, [])
    self._check_patch(p, 'file_a', RAW.MINIMAL, nb_hunks=0)

  def testSmallest(self):
    p = patch.FilePatchDiff('file_a', RAW.NEW_NOT_NULL, [])
    self._check_patch(p, 'file_a', RAW.NEW_NOT_NULL, is_new=True, nb_hunks=1)

  def testRenameOnlyHeader(self):
    p = patch.FilePatchDiff('file_b', RAW.MINIMAL_RENAME, [])
    self._check_patch(
        p, 'file_b', RAW.MINIMAL_RENAME, source_filename='file_a', is_new=True,
        nb_hunks=0)

  def testUnicodeFilenameGet(self):
    p = patch.FilePatchDiff(u'filé_b', RAW.RENAME_UTF8, [])
    self._check_patch(
        p, u'filé_b', RAW.RENAME_UTF8, source_filename=u'file_à', is_new=True,
        nb_hunks=1)
    self.assertTrue(isinstance(p.get(False), str))
    p.set_relpath('foo')
    self.assertTrue(isinstance(p.get(False), str))
    self.assertEquals(u'foo/file_à'.encode('utf-8'), p.source_filename_utf8)
    self.assertEquals(u'foo/file_à', p.source_filename)
    self.assertEquals(u'foo/filé_b'.encode('utf-8'), p.filename_utf8)
    self.assertEquals(u'foo/filé_b', p.filename)

  def testGitCopyPartial(self):
    p = patch.FilePatchDiff('wtf2', GIT.COPY_PARTIAL, [])
    self._check_patch(
        p, 'wtf2', GIT.COPY_PARTIAL, source_filename='wtf', is_git_diff=True,
        patchlevel=1, is_new=True, nb_hunks=1)

  def testGitCopyPartialAsSvn(self):
    p = patch.FilePatchDiff('wtf2', GIT.COPY_PARTIAL, [])
    # TODO(maruel): Improve processing.
    diff = (
        'diff --git a/wtf2 b/wtf22\n'
        'similarity index 98%\n'
        'copy from wtf2\n'
        'copy to wtf22\n'
        'index 79fbaf3..3560689 100755\n'
        '--- a/wtf2\n'
        '+++ b/wtf22\n'
        '@@ -1,4 +1,4 @@\n'
        '-#!/usr/bin/env python\n'
        '+#!/usr/bin/env python1.3\n'
        ' # Copyright (c) 2010 The Chromium Authors. All rights reserved.\n'
        ' # blah blah blah as\n'
        ' # found in the LICENSE file.\n')
    self.assertEquals(diff, p.get(False))

  def testGitNewExe(self):
    p = patch.FilePatchDiff('natsort_test.py', GIT.NEW_EXE, [])
    self._check_patch(
        p,
        'natsort_test.py',
        GIT.NEW_EXE,
        is_new=True,
        is_git_diff=True,
        patchlevel=1,
        svn_properties=[('svn:executable', '.')],
        nb_hunks=1)

  def testGitNewMode(self):
    p = patch.FilePatchDiff('natsort_test.py', GIT.NEW_MODE, [])
    self._check_patch(
        p, 'natsort_test.py', GIT.NEW_MODE, is_new=True, is_git_diff=True,
        patchlevel=1, nb_hunks=1)

  def testPatchsetOrder(self):
    # Deletes must be last.
    # File renames/move/copy must be first.
    patches = [
        patch.FilePatchDiff('chrome/file.cc', RAW.PATCH, []),
        patch.FilePatchDiff(
            'tools\\clang_check/README.chromium', GIT.DELETE, []),
        patch.FilePatchDiff('tools/run_local_server.sh', GIT.RENAME, []),
        patch.FilePatchDiff(
            'chromeos\\views/webui_menu_widget.h', GIT.RENAME_PARTIAL, []),
        patch.FilePatchDiff('pp', GIT.COPY, []),
        patch.FilePatchDiff('foo', GIT.NEW, []),
        patch.FilePatchDelete('other/place/foo', True),
        patch.FilePatchBinary('bar', 'data', [], is_new=False),
    ]
    expected = [
        'pp',
        'chromeos/views/webui_menu_widget.h',
        'tools/run_local_server.sh',
        'bar',
        'chrome/file.cc',
        'foo',
        'other/place/foo',
        'tools/clang_check/README.chromium',
    ]
    patchset = patch.PatchSet(patches)
    self.assertEquals(expected, patchset.filenames)

  def testGitPatch(self):
    p = patch.FilePatchDiff('chrome/file.cc', GIT.PATCH, [])
    self._check_patch(
        p, 'chrome/file.cc', GIT.PATCH, is_git_diff=True, patchlevel=1,
        nb_hunks=1)

  def testGitPatchShortHunkHeader(self):
    p = patch.FilePatchDiff(
        'chrome/browser/api/OWNERS', GIT.PATCH_SHORT_HUNK_HEADER, [])
    self._check_patch(
        p, 'chrome/browser/api/OWNERS', GIT.PATCH_SHORT_HUNK_HEADER,
        is_git_diff=True, patchlevel=1, nb_hunks=1)


class PatchTestFail(unittest.TestCase):
  # All patches that should throw.
  def testFilePatchDelete(self):
    self.assertFalse(hasattr(patch.FilePatchDelete('foo', False), 'get'))

  def testFilePatchDeleteBin(self):
    self.assertFalse(hasattr(patch.FilePatchDelete('foo', True), 'get'))

  def testFilePatchDiffBad(self):
    try:
      patch.FilePatchDiff('foo', 'data', [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffEmpty(self):
    try:
      patch.FilePatchDiff('foo', '', [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffNone(self):
    try:
      patch.FilePatchDiff('foo', None, [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchBadDiffName(self):
    try:
      patch.FilePatchDiff('foo', RAW.PATCH, [])
      self.fail()
    except patch.UnsupportedPatchFormat, e:
      self.assertEquals(
          "Can't process patch for file foo.\nUnexpected diff: chrome/file.cc.",
          str(e))

  def testFilePatchDiffBadHeader(self):
    try:
      diff = (
        '+++ b/foo\n'
        '@@ -0,0 +1 @@\n'
        '+bar\n')
      patch.FilePatchDiff('foo', diff, [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffBadGitHeader(self):
    try:
      diff = (
        'diff --git a/foo b/foo\n'
        '+++ b/foo\n'
        '@@ -0,0 +1 @@\n'
        '+bar\n')
      patch.FilePatchDiff('foo', diff, [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffBadHeaderReversed(self):
    try:
      diff = (
        '+++ b/foo\n'
        '--- b/foo\n'
        '@@ -0,0 +1 @@\n'
        '+bar\n')
      patch.FilePatchDiff('foo', diff, [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffGitBadHeaderReversed(self):
    try:
      diff = (
        'diff --git a/foo b/foo\n'
        '+++ b/foo\n'
        '--- b/foo\n'
        '@@ -0,0 +1 @@\n'
        '+bar\n')
      patch.FilePatchDiff('foo', diff, [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testFilePatchDiffInvalidGit(self):
    try:
      patch.FilePatchDiff('svn_utils_test.txt', (
        'diff --git a/tests/svn_utils_test_data/svn_utils_test.txt '
        'b/tests/svn_utils_test_data/svn_utils_test.txt\n'
        'index 0e4de76..8320059 100644\n'
        '--- a/svn_utils_test.txt\n'
        '+++ b/svn_utils_test.txt\n'
        '@@ -3,6 +3,7 @@ bb\n'
        'ccc\n'
        'dd\n'
        'e\n'
        '+FOO!\n'
        'ff\n'
        'ggg\n'
        'hh\n'),
        [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass
    try:
      patch.FilePatchDiff('svn_utils_test2.txt', (
        'diff --git a/svn_utils_test_data/svn_utils_test.txt '
        'b/svn_utils_test.txt\n'
        'index 0e4de76..8320059 100644\n'
        '--- a/svn_utils_test.txt\n'
        '+++ b/svn_utils_test.txt\n'
        '@@ -3,6 +3,7 @@ bb\n'
        'ccc\n'
        'dd\n'
        'e\n'
        '+FOO!\n'
        'ff\n'
        'ggg\n'
        'hh\n'),
        [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testRelPathBad(self):
    patches = patch.PatchSet([
        patch.FilePatchDiff('chrome\\file.cc', RAW.PATCH, []),
        patch.FilePatchDelete('other\\place\\foo', True),
    ])
    try:
      patches.set_relpath('..')
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testInverted(self):
    try:
      patch.FilePatchDiff(
        'file_a', '+++ file_a\n--- file_a\n@@ -0,0 +1 @@\n+foo\n', [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testInvertedOnlyHeader(self):
    try:
      patch.FilePatchDiff('file_a', '+++ file_a\n--- file_a\n', [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass

  def testBadHunkCommas(self):
    try:
      patch.FilePatchDiff(
        'file_a',
        '--- file_a\n'
        '+++ file_a\n'
        '@@ -0,,0 +1 @@\n'
        '+foo\n',
        [])
      self.fail()
    except patch.UnsupportedPatchFormat:
      pass


if __name__ == '__main__':
  logging.basicConfig(level=
      [logging.WARNING, logging.INFO, logging.DEBUG][
        min(2, sys.argv.count('-v'))])
  unittest.main()
