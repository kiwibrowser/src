# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for autotest_quickmerge."""

from __future__ import print_function

import mock
import types

from chromite.lib import cros_test_lib
from chromite.scripts import autotest_quickmerge


RSYNC_TEST_OUTPUT = """.d..t...... ./
>f..t...... touched file with spaces
>f..t...... touched_file
>f.st...... modified_contents_file
.f...p..... modified_permissions_file
.f....o.... modified_owner_file
>f+++++++++ new_file
cL+++++++++ new_symlink -> directory_a/new_file_in_directory
.d..t...... directory_a/
>f+++++++++ directory_a/new_file_in_directory
>f..t...... directory_a/touched_file_in_directory
cd+++++++++ new_empty_directory/
.d..t...... touched_empty_directory/"""
# The output format of rsync's itemized changes has a few unusual cases
# that are ambiguous. For instance, if the operation involved creating a
# symbolic link named "a -> b" to a file named "c", the rsync output would be:
# cL+++++++++ a -> b -> c
# which is indistinguishable from the output for creating a symbolic link named
# "a" to a file named "b -> c".
# Since there is no easy resolution to this ambiguity, and it seems like a case
# that would rarely or never be encountered in the wild, rsync quickmerge
# will exclude all files which contain the substring " -> " in their name.

RSYNC_TEST_OUTPUT_FOR_PACKAGE_UPDATE = \
""">f..t...... client/ardvark.py
.d..t...... client/site_tests/
>f+++++++++ client/site_tests/nothing.py
.d..t...... client/site_tests/factory_Leds/
>f+++++++++ client/site_tests/factory_Leds/factory_Leds2.py
>f..tpog... client/site_tests/login_UserPolicyKeys/control
>f..tpog... client/site_tests/login_UserPolicyKeys/login_UserPolicyKeys.py
>f..t...... client/site_tests/platform_Cryptohome/platform_Cryptohome.py
>f..tpog... server/site_tests/security_DbusFuzzServer/control
>f..t.og... utils/coverage_suite.py
.d..t...... client/site_tests/power_Thermal/
cd+++++++++ client/site_tests/power_Thermal/a/
cd+++++++++ client/site_tests/power_Thermal/a/b/
cd+++++++++ client/site_tests/power_Thermal/a/b/c/
>f+++++++++ client/site_tests/power_Thermal/a/b/c/d.py"""

RSYNC_TEST_DESTINATION_PATH = '/foo/bar/'

TEST_PACKAGE_CP = 'a_cute/little_puppy'
TEST_PACKAGE_CPV = 'a_cute/little_puppy-3.14159'
TEST_PACKAGE_C = 'a_cute'
TEST_PACKAGE_PV = 'little_puppy-3.14159'
TEST_PORTAGE_ROOT = '/bib/bob/'
TEST_PACKAGE_OLDCONTENTS = {
    u'/by/the/prickling/of/my/thumbs': (u'obj', '1234', '4321'),
    u'/something/wicked/this/way/comes': (u'dir',)
}


class ItemizeChangesFromRsyncOutput(cros_test_lib.TestCase):
  """Test autotest_quickmerge.ItemizeChangesFromRsyncOutput."""

  def testItemizeChangesFromRsyncOutput(self):
    """Test that rsync output parser returns correct FileMutations."""
    expected_new = set(
        [('>f+++++++++', '/foo/bar/new_file'),
         ('>f+++++++++', '/foo/bar/directory_a/new_file_in_directory'),
         ('cL+++++++++', '/foo/bar/new_symlink')])

    expected_mod = set(
        [('>f..t......', '/foo/bar/touched file with spaces'),
         ('>f..t......', '/foo/bar/touched_file'),
         ('>f.st......', '/foo/bar/modified_contents_file'),
         ('.f...p.....', '/foo/bar/modified_permissions_file'),
         ('.f....o....', '/foo/bar/modified_owner_file'),
         ('>f..t......', '/foo/bar/directory_a/touched_file_in_directory')])

    expected_dir = set([('cd+++++++++', '/foo/bar/new_empty_directory/')])

    report = autotest_quickmerge.ItemizeChangesFromRsyncOutput(
        RSYNC_TEST_OUTPUT, RSYNC_TEST_DESTINATION_PATH)

    self.assertEqual(expected_new, set(report.new_files))
    self.assertEqual(expected_mod, set(report.modified_files))
    self.assertEqual(expected_dir, set(report.new_directories))


class PackageNameParsingTest(cros_test_lib.TestCase):
  """Test autotest_quickmerge.GetStalePackageNames."""

  def testGetStalePackageNames(self):
    autotest_sysroot = '/an/arbitrary/path/'
    change_report = autotest_quickmerge.ItemizeChangesFromRsyncOutput(
        RSYNC_TEST_OUTPUT_FOR_PACKAGE_UPDATE, autotest_sysroot)
    package_matches = autotest_quickmerge.GetStalePackageNames(
        change_report.modified_files + change_report.new_files,
        autotest_sysroot)
    expected_set = set(['factory_Leds', 'login_UserPolicyKeys',
                        'platform_Cryptohome', 'power_Thermal'])
    self.assertEqual(set(package_matches), expected_set)


class RsyncCommandTest(cros_test_lib.RunCommandTestCase):
  """Test autotest_quickmerge.RsyncQuickmerge."""

  def testRsyncQuickmergeCommand(self):
    """Test that RsyncQuickMerge makes correct call to SudoRunCommand"""
    include_file_name = 'an_include_file_name'
    source_path = 'a_source_path'
    sysroot_path = 'a_sysroot_path'

    expected_command = ['rsync', '-a', '-n', '-u', '-i',
                        '--exclude=**.pyc', '--exclude=**.pyo',
                        '--exclude=** -> *',
                        '--include-from=%s' % include_file_name,
                        '--exclude=*',
                        source_path,
                        sysroot_path]

    autotest_quickmerge.RsyncQuickmerge(source_path, sysroot_path,
                                        include_file_name,
                                        pretend=True,
                                        overwrite=False)

    self.assertCommandContains(expected_command)


class PortageManipulationsTest(cros_test_lib.MockTestCase):
  """Test usage of autotest_quickmerge.portage."""

  def testUpdatePackageContents(self):
    """Test that UpdatePackageContents makes the correct calls to portage."""
    autotest_quickmerge.portage = mock.MagicMock()
    portage = autotest_quickmerge.portage

    portage.root = TEST_PORTAGE_ROOT

    mock_vartree = mock.MagicMock()
    mock_vartree.settings = {'an arbitrary' : 'dictionary'}
    mock_tree = {TEST_PORTAGE_ROOT : {'vartree' : mock_vartree}}
    portage.create_trees.return_value = mock_tree

    mock_vartree.dbapi = mock.MagicMock()
    mock_vartree.dbapi.cp_list.return_value = [TEST_PACKAGE_CPV]

    mock_package = mock.MagicMock()
    portage.dblink.return_value = mock_package  # pylint: disable=no-member
    mock_package.getcontents.return_value = TEST_PACKAGE_OLDCONTENTS

    EXPECTED_NEW_ENTRIES = {
        '/foo/bar/new_empty_directory': (u'dir',),
        '/foo/bar/directory_a/new_file_in_directory': (u'obj', '0', '0'),
        '/foo/bar/new_file': (u'obj', '0', '0'),
        '/foo/bar/new_symlink': (u'obj', '0', '0')
    }
    RESULT_DICIONARY = TEST_PACKAGE_OLDCONTENTS.copy()
    RESULT_DICIONARY.update(EXPECTED_NEW_ENTRIES)

    mock_vartree.dbapi.writeContentsToContentsFile(mock_package,
                                                   RESULT_DICIONARY)

    change_report = autotest_quickmerge.ItemizeChangesFromRsyncOutput(
        RSYNC_TEST_OUTPUT, RSYNC_TEST_DESTINATION_PATH)
    autotest_quickmerge.UpdatePackageContents(change_report, TEST_PACKAGE_CP,
                                              TEST_PORTAGE_ROOT)


class PortageAPITest(cros_test_lib.TestCase):
  """Ensures that required portage API exists."""

  def runTest(self):
    try:
      import portage
    except ImportError:
      self.skipTest('Portage not available in test environment. Re-run test '
                    'in chroot.')
    try:
      # pylint: disable=no-member
      f = portage.vardbapi.writeContentsToContentsFile
    except AttributeError:
      self.fail('Required writeContentsToContentsFile function does '
                'not exist.')

    self.assertIsInstance(f, types.UnboundMethodType,
                          'Required writeContentsToContentsFile is not '
                          'a function.')
