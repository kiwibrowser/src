# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for loman.py"""

from __future__ import print_function

import os
import xml.etree.ElementTree as ElementTree

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.scripts import loman


class RunGitMock(partial_mock.PartialCmdMock):
  """Partial mock for git.RunGit."""
  TARGET = 'chromite.lib.git'
  ATTRS = ('RunGit',)
  DEFAULT_ATTR = 'RunGit'

  def RunGit(self, _git_repo, cmd, _retry=True, **kwargs):
    return self._results['RunGit'].LookupResult(
        (cmd,), hook_args=(cmd,), hook_kwargs=kwargs)


class ParserTest(cros_test_lib.OutputTestCase):
  """Tests for the CLI parser."""

  def setUp(self):
    self.parser = loman.GetParser()

  def testNoCommand(self):
    """Require a command at least."""
    with self.OutputCapturer():
      self.assertRaises(SystemExit, self.parser.parse_args, [])

  def testBadCommand(self):
    """Reject unknown commands."""
    with self.OutputCapturer():
      self.assertRaises(SystemExit, self.parser.parse_args, ['flyaway'])

  def testAddCommand(self):
    """Verify basic add command behavior."""
    with self.OutputCapturer():
      self.parser.parse_args(['add', '--workon', 'project'])
      self.parser.parse_args(['add', 'project', 'path', '--remote', 'foo'])


class ManifestTest(cros_test_lib.TempDirTestCase):
  """Tests that need a real .repo/ manifest layout."""

  def setUp(self):
    # The loman code looks for the repo root, so make one, and chdir there.
    os.chdir(self.tempdir)

    for d in ('repo', 'manifests', 'manifests.git'):
      osutils.SafeMakedirs(os.path.join('.repo', d))

    for m in ('default.xml', 'full.xml', 'minilayout.xml'):
      osutils.Touch(os.path.join('.repo', 'manifests', m))

    self._SetManifest('default.xml')

  def _SetManifest(self, manifest):
    """Set active manifest to point to |manifest|."""
    source = os.path.join('.repo', 'manifest.xml')
    target = os.path.join('manifests', manifest)
    osutils.SafeUnlink(source)
    os.symlink(target, source)


class AddTest(cros_test_lib.MockOutputTestCase, ManifestTest):
  """Tests for the add command."""

  def testRejectBadCommands(self):
    """Reject bad invocations."""
    bad_cmds = (
        # Missing path.
        ['add'],
        # Extra project.
        ['add', '--workon', 'path', 'project'],
        # Missing --remote.
        ['add', 'path', 'project'],
        # Missing project.
        ['add', 'path', '--remote', 'remote'],
    )
    with self.OutputCapturer():
      for cmd in bad_cmds:
        self.assertRaises(SystemExit, loman.main, cmd)


class NoMiniayoutTest(cros_test_lib.MockOutputTestCase, ManifestTest):
  """Check deprecated minilayout setups are detected."""

  def setUp(self):
    self._SetManifest('minilayout.xml')

  def testMiniLayoutDetected(self):
    """Check error is raised when repo is setup with minilayout."""

    class _Error(Exception):
      """Stub for test."""

    self.PatchObject(loman, '_AssertNotMiniLayout', side_effect=_Error)
    cmd = ['add', '-w', 'foo']
    with self.OutputCapturer():
      self.assertRaises(_Error, loman.main, cmd)


class IncludeXmlTest(cros_test_lib.MockOutputTestCase, ManifestTest):
  """End to End tests for reading and producing XML trees."""

  PROJECT = 'chromiumos/repohooks'

  def setUp(self):
    INCLUDING_XML = 'including.xml'
    INCLUDED_XML = 'included.xml'
    osutils.WriteFile(os.path.join('.repo', 'manifests', INCLUDING_XML),
                      """
<manifest>
    <include name="%s" />
    <project remote="cros-internal" path="crostools" groups="br" name="ct" />
</manifest>""" % (INCLUDED_XML,))

    osutils.WriteFile(os.path.join('.repo', 'manifests', INCLUDED_XML),
                      """
<manifest>
    <default remote="cros" revision="HEAD" />
    <remote name="cros" />
    <remote name="cros-internal" />
    <project path="src/repohooks" name="%s" groups="minilayout,bt" />
</manifest>""" % (self.PROJECT,))
    self._SetManifest(INCLUDING_XML)

    self.git_mock = self.StartPatcher(RunGitMock())
    self.git_mock.AddCmdResult(
        ['symbolic-ref', '-q', 'HEAD'], output='default')
    self.git_mock.AddCmdResult(
        ['config', '--get-regexp', 'branch\\.default\\.(remote|merge)'],
        output='branch.default.merge firmware-branch')

    self.git_mock.AddCmdResult(
        ['config',
         '-f', os.path.join(self.tempdir, '.repo', 'manifests.git', 'config'),
         '--get', 'manifest.groups'], output='group1,group2')

  def testAddExistingProject(self):
    """Add an existing project, check no local_manifest.xml are created."""
    self.git_mock.AddCmdResult(
        ['config',
         '-f', os.path.join(self.tempdir, '.repo', 'manifests.git', 'config'),
         'manifest.groups',
         'minilayout,platform-linux,group1,group2,name:%s' % (self.PROJECT,)])
    cmd = ['add', '-w', self.PROJECT]
    with self.OutputCapturer():
      self.assertEqual(loman.main(cmd), 0)
    self.assertNotExists(os.path.join('.repo', 'local_manifest.xml'))

  def testAddNewProject(self):
    """Add new project to the repo.

    Check local_manifest.xml is created and valid.
    """
    new_project = 'project'
    self.git_mock.AddCmdResult(
        ['config',
         '-f', os.path.join(self.tempdir, '.repo', 'manifests.git', 'config'),
         'manifest.groups',
         'minilayout,platform-linux,group1,group2,name:%s' % (new_project,)],)
    cmd = ['add', new_project, 'path', '-r', 'remote']
    with self.OutputCapturer():
      self.assertEqual(loman.main(cmd), 0)
    expected_local_manifest_nodes = ElementTree.fromstring("""
<manifest>
  <project name="project" path="path" remote="remote" workon="False" />
</manifest>""")
    with open(os.path.join('.repo', 'local_manifest.xml')) as f:
      local_manifest_nodes = ElementTree.fromstring(f.read())

    # Read project, check for failure.
    self.assertEqual(ElementTree.tostring(expected_local_manifest_nodes),
                     ElementTree.tostring(local_manifest_nodes))

    # Check that re-adding triggers error.
    cmd = ['add', new_project, 'path', '-r', 'remote']
    with self.OutputCapturer() as output:
      self.assertRaises(SystemExit, loman.main, cmd)
      self.assertIn('conflicts with', '\n'.join(output.GetStderrLines()))
