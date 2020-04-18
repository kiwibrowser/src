# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for cros_mark_chrome_as_stable.py."""

from __future__ import print_function

import base64
import cStringIO
import mock
import os
from textwrap import dedent

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import portage_util
from chromite.scripts import cros_mark_chrome_as_stable


unstable_data = 'KEYWORDS=~x86 ~arm'
stable_data = 'KEYWORDS=x86 arm'
fake_svn_rev = '12345'
new_fake_svn_rev = '23456'


class _StubCommandResult(object):
  """Helper for mocking RunCommand results."""
  def __init__(self, msg):
    self.output = msg


class CrosMarkChromeAsStable(cros_test_lib.MockTempDirTestCase):
  """Tests for cros_mark_chrome_as_stable."""

  def setUp(self):
    """Setup vars and create mock dir."""
    self.tmp_overlay = os.path.join(self.tempdir, 'chromiumos-overlay')
    self.mock_chrome_dir = os.path.join(self.tmp_overlay, constants.CHROME_CP)
    os.makedirs(self.mock_chrome_dir)

    ebuild = os.path.join(self.mock_chrome_dir,
                          constants.CHROME_PN + '-%s.ebuild')
    self.unstable = ebuild % '9999'
    self.sticky_branch = '8.0.224'
    self.sticky_version = '%s.503' % self.sticky_branch
    self.sticky = ebuild % self.sticky_version
    self.sticky_rc_version = '%s.504' % self.sticky_branch
    self.sticky_rc = ebuild % (self.sticky_rc_version + '_rc-r1')
    self.latest_stable_version = '8.0.300.1'
    self.latest_stable = ebuild % (self.latest_stable_version + '_rc-r2')
    self.tot_stable_version = '9.0.305.0'
    self.tot_stable = ebuild % (self.tot_stable_version + '_alpha-r1')

    self.sticky_new_rc_version = '%s.520' % self.sticky_branch
    self.sticky_new_rc = ebuild % (self.sticky_new_rc_version + '_rc-r1')
    self.latest_new_version = '9.0.305.1'
    self.latest_new = ebuild % (self.latest_new_version + '_rc-r1')
    self.tot_new_version = '9.0.306.0'
    self.tot_new = ebuild % (self.tot_new_version + '_alpha-r1')

    osutils.WriteFile(self.unstable, unstable_data)
    osutils.WriteFile(self.sticky, stable_data)
    osutils.WriteFile(self.sticky_rc, stable_data)
    osutils.WriteFile(self.latest_stable, stable_data)
    # pylint: disable=protected-access
    osutils.WriteFile(
        self.tot_stable,
        '\n'.join((stable_data,
                   '%s=%s' % (cros_mark_chrome_as_stable._CHROME_SVN_TAG,
                              fake_svn_rev))))

  def testFindChromeCandidates(self):
    """Test creation of stable ebuilds from mock dir."""
    unstable, stable_ebuilds = cros_mark_chrome_as_stable.FindChromeCandidates(
        self.mock_chrome_dir)

    stable_ebuild_paths = [x.ebuild_path for x in stable_ebuilds]
    self.assertEqual(unstable.ebuild_path, self.unstable)
    self.assertEqual(len(stable_ebuilds), 4)
    self.assertTrue(self.sticky in stable_ebuild_paths)
    self.assertTrue(self.sticky_rc in stable_ebuild_paths)
    self.assertTrue(self.latest_stable in stable_ebuild_paths)
    self.assertTrue(self.tot_stable in stable_ebuild_paths)

  def _GetStableEBuilds(self):
    """Common helper to create a list of stable ebuilds."""
    return [
        cros_mark_chrome_as_stable.ChromeEBuild(self.sticky),
        cros_mark_chrome_as_stable.ChromeEBuild(self.sticky_rc),
        cros_mark_chrome_as_stable.ChromeEBuild(self.latest_stable),
        cros_mark_chrome_as_stable.ChromeEBuild(self.tot_stable),
    ]

  def testTOTFindChromeUprevCandidate(self):
    """Tests if we can find tot uprev candidate from our mock dir data."""
    stable_ebuilds = self._GetStableEBuilds()

    candidate = cros_mark_chrome_as_stable.FindChromeUprevCandidate(
        stable_ebuilds, constants.CHROME_REV_TOT,
        self.sticky_branch)

    self.assertEqual(candidate.ebuild_path, self.tot_stable)

  def testLatestFindChromeUprevCandidate(self):
    """Tests if we can find latest uprev candidate from our mock dir data."""
    stable_ebuilds = self._GetStableEBuilds()

    candidate = cros_mark_chrome_as_stable.FindChromeUprevCandidate(
        stable_ebuilds, constants.CHROME_REV_LATEST,
        self.sticky_branch)

    self.assertEqual(candidate.ebuild_path, self.latest_stable)

  def testStickyFindChromeUprevCandidate(self):
    """Tests if we can find sticky uprev candidate from our mock dir data."""
    stable_ebuilds = self._GetStableEBuilds()

    candidate = cros_mark_chrome_as_stable.FindChromeUprevCandidate(
        stable_ebuilds, constants.CHROME_REV_STICKY,
        self.sticky_branch)

    self.assertEqual(candidate.ebuild_path, self.sticky_rc)

  def testGetTipOfTrunkRevision(self):
    """Tests if we can get the latest svn revision from TOT."""
    A_URL = 'dorf://mink/delaane/forkat/sertiunu.ortg./desk'
    result = {'log': [{'commit': 'deadbeef' * 5}]}
    self.PatchObject(gob_util, 'FetchUrlJson', return_value=result)
    revision = gob_util.GetTipOfTrunkRevision(A_URL)
    self.assertEquals(revision, 'deadbeef' * 5)

  def testGetTipOfTrunkVersion(self):
    """Tests if we get the latest version from TOT."""
    TEST_URL = 'proto://host.org/path/to/repo'
    TEST_VERSION_CONTENTS = dedent('''\
        A=8
        B=0
        C=256
        D=0''')
    result = cStringIO.StringIO(base64.b64encode(TEST_VERSION_CONTENTS))
    self.PatchObject(gob_util, 'FetchUrl', return_value=result)
    # pylint: disable=protected-access
    version = cros_mark_chrome_as_stable._GetSpecificVersionUrl(
        TEST_URL, 'test-revision')
    self.assertEquals(version, '8.0.256.0')

  def testCheckIfChromeRightForOS(self):
    """Tests if we can find the chromeos build from our mock DEPS."""
    test_data1 = "buildspec_platforms:\n    'chromeos,',\n"
    test_data2 = "buildspec_platforms:\n    'android,',\n"
    expected_deps = cros_mark_chrome_as_stable.CheckIfChromeRightForOS(
        test_data1)
    unexpected_deps = cros_mark_chrome_as_stable.CheckIfChromeRightForOS(
        test_data2)
    self.assertTrue(expected_deps)
    self.assertFalse(unexpected_deps)

  def testGetLatestRelease(self):
    """Tests if we can find the latest release from our mock url data."""
    TEST_HOST = 'sores.chromium.org'
    TEST_URL = 'phthp://%s/tqs' % TEST_HOST
    TEST_TAGS = ['7.0.224.1', '7.0.224', '8.0.365.5', 'foo', 'bar-12.13.14.15']
    TEST_REFS_JSON = dict((tag, None) for tag in TEST_TAGS)
    TEST_BAD_DEPS_CONTENT = dedent('''\
        buildspec_platforms: 'TRS-80,',
        ''')
    TEST_GOOD_DEPS_CONTENT = dedent('''\
        buildspec_platforms: 'chromeos,',
        ''')

    self.PatchObject(gob_util, 'FetchUrl', side_effect=(
        cStringIO.StringIO(base64.b64encode(TEST_BAD_DEPS_CONTENT)),
        cStringIO.StringIO(base64.b64encode(TEST_GOOD_DEPS_CONTENT)),
    ))
    self.PatchObject(gob_util, 'FetchUrlJson', side_effect=(TEST_REFS_JSON,))
    release = cros_mark_chrome_as_stable.GetLatestRelease(TEST_URL)
    self.assertEqual('7.0.224.1', release)

  def testGetLatestStickyRelease(self):
    """Tests if we can find the latest sticky release from our mock url data."""
    TEST_HOST = 'sores.chromium.org'
    TEST_URL = 'phthp://%s/tqs' % TEST_HOST
    TEST_TAGS = ['7.0.224.2', '7.0.224', '7.0.365.5', 'foo', 'bar-12.13.14.15']
    TEST_REFS_JSON = dict((tag, None) for tag in TEST_TAGS)
    TEST_DEPS_CONTENT = dedent('''\
        buildspec_platforms: 'chromeos,',
        ''')

    self.PatchObject(gob_util, 'FetchUrl', side_effect=(
        cStringIO.StringIO(base64.b64encode(TEST_DEPS_CONTENT)),
    ))
    self.PatchObject(gob_util, 'FetchUrlJson', side_effect=(TEST_REFS_JSON,))
    release = cros_mark_chrome_as_stable.GetLatestRelease(TEST_URL, '7.0.224')
    self.assertEqual('7.0.224.2', release)

  def testLatestChromeRevisionListLink(self):
    """Tests link generation to rev lists.

    Verifies that we can generate a link to the revision list between the
    latest Chromium release and the last one we successfully built.
    """
    osutils.WriteFile(self.latest_new, stable_data)
    expected = cros_mark_chrome_as_stable.GetChromeRevisionLinkFromVersions(
        self.latest_stable_version, self.latest_new_version)
    made = cros_mark_chrome_as_stable.GetChromeRevisionListLink(
        cros_mark_chrome_as_stable.ChromeEBuild(self.latest_stable),
        cros_mark_chrome_as_stable.ChromeEBuild(self.latest_new),
        constants.CHROME_REV_LATEST)
    self.assertEqual(expected, made)

  def testStickyEBuild(self):
    """Tests if we can find the sticky ebuild from our mock directories."""
    # pylint: disable=protected-access
    stable_ebuilds = self._GetStableEBuilds()
    sticky_ebuild = cros_mark_chrome_as_stable._GetStickyEBuild(
        stable_ebuilds)
    self.assertEqual(sticky_ebuild.chrome_version, self.sticky_version)

  def testChromeEBuildInit(self):
    """Tests if the chrome_version is set correctly in a ChromeEBuild."""
    ebuild = cros_mark_chrome_as_stable.ChromeEBuild(self.sticky)
    self.assertEqual(ebuild.chrome_version, self.sticky_version)

  def _CommonMarkAsStableTest(self, chrome_rev, new_version, old_ebuild_path,
                              new_ebuild_path, commit_string_indicator):
    """Common function used for test functions for MarkChromeEBuildAsStable.

    This function stubs out others calls, and runs MarkChromeEBuildAsStable
    with the specified args.

    Args:
      chrome_rev: standard chrome_rev argument
      new_version: version we are revving up to
      old_ebuild_path: path to the stable ebuild
      new_ebuild_path: path to the to be created path
      commit_string_indicator: a string that the commit message must contain
    """
    self.PatchObject(cros_build_lib, 'RunCommand',
                     side_effect=Exception('should not be called'))
    self.PatchObject(portage_util.EBuild, 'GetCrosWorkonVars',
                     return_value=None)
    git_mock = self.PatchObject(git, 'RunGit')
    commit_mock = self.PatchObject(portage_util.EBuild, 'CommitChange')
    stable_candidate = cros_mark_chrome_as_stable.ChromeEBuild(old_ebuild_path)
    unstable_ebuild = cros_mark_chrome_as_stable.ChromeEBuild(self.unstable)
    chrome_pn = 'chromeos-chrome'
    chrome_version = new_version
    commit = None
    package_dir = self.mock_chrome_dir

    cros_mark_chrome_as_stable.MarkChromeEBuildAsStable(
        stable_candidate, unstable_ebuild, chrome_pn, chrome_rev,
        chrome_version, commit, package_dir)

    git_mock.assert_has_calls([
        mock.call(package_dir, ['add', new_ebuild_path]),
        mock.call(package_dir, ['rm', old_ebuild_path]),
    ])
    commit_mock.assert_called_with(
        partial_mock.HasString(commit_string_indicator),
        package_dir)

  def testStickyMarkAsStable(self):
    """Tests to see if we can mark chrome as stable for a new sticky release."""
    self._CommonMarkAsStableTest(
        constants.CHROME_REV_STICKY,
        self.sticky_new_rc_version, self.sticky_rc,
        self.sticky_new_rc, 'stable_release')

  def testLatestMarkAsStable(self):
    """Tests to see if we can mark chrome for a latest release."""
    self._CommonMarkAsStableTest(
        constants.CHROME_REV_LATEST,
        self.latest_new_version, self.latest_stable,
        self.latest_new, 'latest_release')

  def testTotMarkAsStable(self):
    """Tests to see if we can mark chrome for tot."""
    self._CommonMarkAsStableTest(
        constants.CHROME_REV_TOT,
        self.tot_new_version, self.tot_stable,
        self.tot_new, 'tot')
