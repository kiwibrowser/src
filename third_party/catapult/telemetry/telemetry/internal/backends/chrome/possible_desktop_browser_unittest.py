# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

from telemetry.internal.backends.chrome import desktop_browser_finder
from telemetry.internal.browser import browser_options

class PossibleDesktopBrowserTest(unittest.TestCase):
  def setUp(self):
    self._finder_options = browser_options.BrowserFinderOptions()
    self._finder_options.chrome_root = tempfile.mkdtemp()

  def tearDown(self):
    chrome_root = self._finder_options.chrome_root
    if chrome_root and os.path.exists(chrome_root):
      shutil.rmtree(self._finder_options.chrome_root, ignore_errors=True)
    profile_dir = self._finder_options.browser_options.profile_dir
    if profile_dir and os.path.exists(profile_dir):
      shutil.rmtree(self._finder_options.chrome_root, ignore_errors=True)

  def testCopyProfileFilesSimple(self):
    source_path = os.path.join(self._finder_options.chrome_root, 'AUTHORS')
    with open(source_path, 'w') as f:
      f.write('foo@chromium.org')

    self._finder_options.browser_options.profile_files_to_copy = [
        (source_path, 'AUTHORS')
    ]

    possible_desktop = desktop_browser_finder.PossibleDesktopBrowser(
        'stable', self._finder_options, None, None,
        False, self._finder_options.chrome_root)
    possible_desktop.SetUpEnvironment(self._finder_options.browser_options)

    profile = possible_desktop.profile_directory
    self.assertTrue(os.path.exists(os.path.join(profile, 'AUTHORS')))

  def testCopyProfileFilesRecursive(self):
    """ Ensure copied files can create directories if needed."""

    source_path = os.path.join(self._finder_options.chrome_root, 'AUTHORS')
    with open(source_path, 'w') as f:
      f.write('foo@chromium.org')
    self._finder_options.browser_options.profile_files_to_copy = [
        (source_path, 'path/to/AUTHORS')
    ]

    possible_desktop = desktop_browser_finder.PossibleDesktopBrowser(
        'stable', self._finder_options, None, None,
        False, self._finder_options.chrome_root)
    possible_desktop.SetUpEnvironment(self._finder_options.browser_options)

    profile = possible_desktop.profile_directory
    self.assertTrue(os.path.exists(os.path.join(profile, 'path/to/AUTHORS')))

  def testCopyProfileFilesWithSeedProfile(self):
    """ Ensure copied files can co-exist with a seeded profile."""

    source_path = os.path.join(self._finder_options.chrome_root, 'AUTHORS1')
    with open(source_path, 'w') as f:
      f.write('foo@chromium.org')

    source_profile = tempfile.mkdtemp()
    self._finder_options.browser_options.profile_dir = source_profile

    existing_path = os.path.join(
        self._finder_options.browser_options.profile_dir, 'AUTHORS2')
    with open(existing_path, 'w') as f:
      f.write('bar@chromium.org')

    self._finder_options.browser_options.profile_files_to_copy = [
        (source_path, 'AUTHORS2')
    ]
    possible_desktop = desktop_browser_finder.PossibleDesktopBrowser(
        'stable', self._finder_options, None, None,
        False, self._finder_options.chrome_root)
    possible_desktop.SetUpEnvironment(self._finder_options.browser_options)

    profile = possible_desktop.profile_directory
    self.assertTrue(os.path.join(profile, 'AUTHORS1'))
    self.assertTrue(os.path.join(profile, 'AUTHORS2'))

  def testCopyProfileFilesWithSeedProfileDoesNotOverwrite(self):
    """ Ensure copied files will not overwrite existing profile files."""

    source_path = os.path.join(self._finder_options.chrome_root, 'AUTHORS')
    with open(source_path, 'w') as f:
      f.write('foo@chromium.org')

    source_profile = tempfile.mkdtemp()
    self._finder_options.browser_options.profile_dir = source_profile

    existing_path = os.path.join(
        self._finder_options.browser_options.profile_dir, 'AUTHORS')
    with open(existing_path, 'w') as f:
      f.write('bar@chromium.org')

    self._finder_options.browser_options.profile_files_to_copy = [
        (source_path, 'AUTHORS')
    ]

    possible_desktop = desktop_browser_finder.PossibleDesktopBrowser(
        'stable', self._finder_options, None, None,
        False, self._finder_options.chrome_root)
    possible_desktop.SetUpEnvironment(self._finder_options.browser_options)

    profile = possible_desktop.profile_directory
    with open(os.path.join(profile, 'AUTHORS'), 'r') as f:
      self.assertEqual('bar@chromium.org', f.read())
