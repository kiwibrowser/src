# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fast alternative to `emerge-$BOARD autotest-all`

Simple script to be run inside the chroot. Used as a fast approximation of
emerge-$board autotest-all, by simply rsync'ing changes from trunk to sysroot.
"""

from __future__ import print_function

import argparse
import glob
import os
import re
import sys
from collections import namedtuple

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import portage_util

if cros_build_lib.IsInsideChroot():
  # Only import portage after we've checked that we're inside the chroot.
  import portage


INCLUDE_PATTERNS_FILENAME = 'autotest-quickmerge-includepatterns'
AUTOTEST_PROJECT_NAME = 'chromiumos/third_party/autotest'
AUTOTEST_EBUILD = 'chromeos-base/autotest'
DOWNGRADE_EBUILDS = ['chromeos-base/autotest']

IGNORE_SUBDIRS = ['ExternalSource',
                  'logs',
                  'results',
                  'site-packages']

# Data structure describing a single rsync filesystem change.
#
# change_description: An 11 character string, the rsync change description
#                     for the particular file.
# absolute_path: The absolute path of the created or modified file.
ItemizedChange = namedtuple('ItemizedChange', ['change_description',
                                               'absolute_path'])

# Data structure describing the rsync new/modified files or directories.
#
# new_files: A list of ItemizedChange objects for new files.
# modified_files: A list of ItemizedChange objects for modified files.
# new_directories: A list of ItemizedChange objects for new directories.
ItemizedChangeReport = namedtuple('ItemizedChangeReport',
                                  ['new_files', 'modified_files',
                                   'new_directories'])


class PortagePackageAPIError(Exception):
  """Exception thrown when unable to retrieve a portage package API."""


def GetStalePackageNames(change_list, autotest_sysroot):
  """Given a rsync change report, returns the names of stale test packages.

  This function pulls out test package names for client-side tests, stored
  within the client/site_tests directory tree, that had any files added or
  modified and for whom any existing bzipped test packages may now be stale.

  Args:
    change_list: A list of ItemizedChange objects corresponding to changed
                 or modified files.
    autotest_sysroot: Absolute path of autotest in the sysroot,
                      e.g. '/build/lumpy/usr/local/build/autotest'

  Returns:
    A list of test package names, eg ['factory_Leds', 'login_UserPolicyKeys'].
    May contain duplicate entries if multiple files within a test directory
    were modified.
  """
  exp = os.path.abspath(autotest_sysroot) + r'/client/site_tests/(.*?)/.*'
  matches = [re.match(exp, change.absolute_path) for change in change_list]
  return [match.group(1) for match in matches if match]


def ItemizeChangesFromRsyncOutput(rsync_output, destination_path):
  """Convert the output of an rsync with `-i` to a ItemizedChangeReport object.

  Args:
    rsync_output: String stdout of rsync command that was run with `-i` option.
    destination_path: String absolute path of the destination directory for the
                      rsync operations. This argument is necessary because
                      rsync's output only gives the relative path of
                      touched/added files.

  Returns:
    ItemizedChangeReport object giving the absolute paths of files that were
    created or modified by rsync.
  """
  modified_matches = re.findall(r'([.>]f[^+]{9}) (.*)', rsync_output)
  new_matches = re.findall(r'(>f\+{9}) (.*)', rsync_output)
  new_symlink_matches = re.findall(r'(cL\+{9}) (.*) -> .*', rsync_output)
  new_dir_matches = re.findall(r'(cd\+{9}) (.*)', rsync_output)

  absolute_modified = [ItemizedChange(c, os.path.join(destination_path, f))
                       for (c, f) in modified_matches]

  # Note: new symlinks are treated as new files.
  absolute_new = [ItemizedChange(c, os.path.join(destination_path, f))
                  for (c, f) in new_matches + new_symlink_matches]

  absolute_new_dir = [ItemizedChange(c, os.path.join(destination_path, f))
                      for (c, f) in new_dir_matches]

  return ItemizedChangeReport(new_files=absolute_new,
                              modified_files=absolute_modified,
                              new_directories=absolute_new_dir)


def GetPackageAPI(portage_root, package_cp):
  """Gets portage API handles for the given package.

  Args:
    portage_root: Root directory of portage tree. Eg '/' or '/build/lumpy'
    package_cp: A string similar to 'chromeos-base/autotest-tests'.

  Returns:
    Returns (package, vartree) tuple, where
      package is of type portage.dbapi.vartree.dblink
      vartree is of type portage.dbapi.vartree.vartree
  """
  if portage_root is None:
    # pylint: disable=no-member
    portage_root = portage.root
  # Ensure that portage_root ends with trailing slash.
  portage_root = os.path.join(portage_root, '')

  # Create a vartree object corresponding to portage_root.
  trees = portage.create_trees(portage_root, portage_root)
  vartree = trees[portage_root]['vartree']

  # List the matching installed packages in cpv format.
  matching_packages = vartree.dbapi.cp_list(package_cp)

  if not matching_packages:
    raise PortagePackageAPIError('No matching package for %s in portage_root '
                                 '%s' % (package_cp, portage_root))

  if len(matching_packages) > 1:
    raise PortagePackageAPIError('Too many matching packages for %s in '
                                 'portage_root %s' % (package_cp,
                                                      portage_root))

  # Convert string match to package dblink.
  package_cpv = matching_packages[0]
  package_split = portage_util.SplitCPV(package_cpv)
  # pylint: disable=no-member
  package = portage.dblink(package_split.category,
                           package_split.pv, settings=vartree.settings,
                           vartree=vartree)

  return package, vartree


def DowngradePackageVersion(portage_root, package_cp,
                            downgrade_to_version='0'):
  """Downgrade the specified portage package version.

  Args:
    portage_root: Root directory of portage tree. Eg '/' or '/build/lumpy'
    package_cp: A string similar to 'chromeos-base/autotest-tests'.
    downgrade_to_version: String version to downgrade to. Default: '0'

  Returns:
    True on success. False on failure (nonzero return code from `mv` command).
  """
  try:
    package, _ = GetPackageAPI(portage_root, package_cp)
  except PortagePackageAPIError:
    # Unable to fetch a corresponding portage package API for this
    # package_cp (either no such package, or name ambigious and matches).
    # So, just fail out.
    return False

  source_directory = package.dbdir
  destination_path = os.path.join(
      package.dbroot, package_cp + '-' + downgrade_to_version)
  if os.path.abspath(source_directory) == os.path.abspath(destination_path):
    return True
  command = ['mv', source_directory, destination_path]
  code = cros_build_lib.SudoRunCommand(command, error_code_ok=True).returncode
  return code == 0


def UpdatePackageContents(change_report, package_cp, portage_root=None):
  """Add newly created files/directors to package contents.

  Given an ItemizedChangeReport, add the newly created files and directories
  to the CONTENTS of an installed portage package, such that these files are
  considered owned by that package.

  Args:
    change_report: ItemizedChangeReport object for the changes to be
                   made to the package.
    package_cp: A string similar to 'chromeos-base/autotest-tests' giving
                the package category and name of the package to be altered.
    portage_root: Portage root path, corresponding to the board that
                  we are working on. Defaults to '/'
  """
  package, vartree = GetPackageAPI(portage_root, package_cp)

  # Append new contents to package contents dictionary.
  contents = package.getcontents().copy()
  for _, filename in change_report.new_files:
    contents.setdefault(filename, (u'obj', '0', '0'))
  for _, dirname in change_report.new_directories:
    # Strip trailing slashes if present.
    contents.setdefault(dirname.rstrip('/'), (u'dir',))

  # Write new contents dictionary to file.
  vartree.dbapi.writeContentsToContentsFile(package, contents)


def RemoveBzipPackages(autotest_sysroot):
  """Remove all bzipped test/dep/profiler packages from sysroot autotest.

  Args:
    autotest_sysroot: Absolute path of autotest in the sysroot,
                      e.g. '/build/lumpy/usr/local/build/autotest'
  """
  osutils.RmDir(os.path.join(autotest_sysroot, 'packages'),
                ignore_missing=True)
  osutils.SafeUnlink(os.path.join(autotest_sysroot, 'packages.checksum'))


def RsyncQuickmerge(source_path, sysroot_autotest_path,
                    include_pattern_file=None, pretend=False,
                    overwrite=False):
  """Run rsync quickmerge command, with specified arguments.

  Command will take form `rsync -a [options] --exclude=**.pyc
                         --exclude=**.pyo
                         [optional --include-from include_pattern_file]
                         --exclude=* [source_path] [sysroot_autotest_path]`

  Args:
    source_path: Directory to rsync from.
    sysroot_autotest_path: Directory to rsync too.
    include_pattern_file: Optional pattern of files to include in rsync.
    pretend: True to use the '-n' option to rsync, to perform dry run.
    overwrite: True to omit '-u' option, overwrite all files in sysroot,
               not just older files.

  Returns:
    The cros_build_lib.CommandResult object resulting from the rsync command.
  """
  command = ['rsync', '-a']

  # For existing files, preserve destination permissions. This ensures that
  # existing files end up with the file permissions set by the ebuilds.
  # If this script copies over a file that does not exist in the destination
  # tree, it will set the least restrictive permissions allowed in the
  # destination tree. This could happen if the file copied is not installed by
  # *any* ebuild, or if the ebuild that installs the file was never emerged.
  command += ['--no-p', '--chmod=ugo=rwX']

  if pretend:
    command += ['-n']

  if not overwrite:
    command += ['-u']

  command += ['-i']

  command += ['--exclude=**.pyc']
  command += ['--exclude=**.pyo']

  # Exclude files with a specific substring in their name, because
  # they create an ambiguous itemized report. (see unit test file for details)
  command += ['--exclude=** -> *']

  if include_pattern_file:
    command += ['--include-from=%s' % include_pattern_file]

  command += ['--exclude=*']

  # Some tests use symlinks. Follow these.
  command += ['-L']

  command += [source_path, sysroot_autotest_path]

  return cros_build_lib.SudoRunCommand(command, redirect_stdout=True)


def ParseArguments(argv):
  """Parse command line arguments

  Returns:
    parsed arguments.
  """
  parser = argparse.ArgumentParser(description='Perform a fast approximation '
                                   'to emerge-$board autotest-all, by '
                                   'rsyncing source tree to sysroot.')

  default_board = cros_build_lib.GetDefaultBoard()
  parser.add_argument('--board', metavar='BOARD', default=default_board,
                      help='Board to perform quickmerge for. Default: ' +
                      (default_board or 'Not configured.'))
  parser.add_argument('--pretend', action='store_true',
                      help='Dry run only, do not modify sysroot autotest.')
  parser.add_argument('--overwrite', action='store_true',
                      help='Overwrite existing files even if newer.')
  parser.add_argument('--force', action='store_true',
                      help=argparse.SUPPRESS)
  parser.add_argument('--verbose', action='store_true',
                      help='Print detailed change report.')

  # Used only if test_that is calling autotest_quickmerge and has detected that
  # the sysroot autotest path is still in usr/local/autotest (ie the build
  # pre-dates https://chromium-review.googlesource.com/#/c/62880/ )
  parser.add_argument('--legacy_path', action='store_true',
                      help=argparse.SUPPRESS)

  return parser.parse_args(argv)


def main(argv):
  cros_build_lib.AssertInsideChroot()

  args = ParseArguments(argv)

  if os.geteuid() != 0:
    try:
      cros_build_lib.SudoRunCommand([sys.executable] + sys.argv)
    except cros_build_lib.RunCommandError:
      return 1
    return 0

  if not args.board:
    print('No board specified. Aborting.')
    return 1

  manifest = git.ManifestCheckout.Cached(constants.SOURCE_ROOT)
  checkout = manifest.FindCheckout(AUTOTEST_PROJECT_NAME)
  brillo_autotest_src_path = os.path.join(checkout.GetPath(absolute=True), '')

  script_path = os.path.dirname(__file__)
  include_pattern_file = os.path.join(script_path, INCLUDE_PATTERNS_FILENAME)

  # TODO: Determine the following string programatically.
  sysroot_path = os.path.join('/build', args.board, '')
  sysroot_autotest_path = os.path.join(sysroot_path,
                                       constants.AUTOTEST_BUILD_PATH, '')
  if args.legacy_path:
    sysroot_autotest_path = os.path.join(sysroot_path, 'usr/local/autotest',
                                         '')

  # Generate the list of source paths to copy.
  src_paths = {os.path.abspath(brillo_autotest_src_path)}
  for quickmerge_file in glob.glob(os.path.join(sysroot_autotest_path,
                                                'quickmerge', '*', '*')):
    try:
      path = osutils.ReadFile(quickmerge_file).strip()
      if path and os.path.exists(path):
        src_paths.add(os.path.abspath(path))
    except IOError:
      logging.error('Could not quickmerge for project: %s',
                    os.path.basename(quickmerge_file))

  num_new_files = 0
  num_modified_files = 0
  for src_path in src_paths:
    rsync_output = RsyncQuickmerge(src_path +'/', sysroot_autotest_path,
                                   include_pattern_file, args.pretend,
                                   args.overwrite)

    if args.verbose:
      logging.info(rsync_output.output)
    change_report = ItemizeChangesFromRsyncOutput(rsync_output.output,
                                                  sysroot_autotest_path)
    num_new_files = num_new_files + len(change_report.new_files)
    num_modified_files = num_modified_files + len(change_report.modified_files)
    if not args.pretend:
      logging.info('Updating portage database.')
      UpdatePackageContents(change_report, AUTOTEST_EBUILD, sysroot_path)

  if not args.pretend:
    for logfile in glob.glob(os.path.join(sysroot_autotest_path, 'packages',
                                          '*.log')):
      try:
        # Open file in a try-except block, for atomicity, instead of
        # doing existence check.
        with open(logfile, 'r') as f:
          package_cp = f.readline().strip()
          DOWNGRADE_EBUILDS.append(package_cp)
      except IOError:
        pass

    for ebuild in DOWNGRADE_EBUILDS:
      if not DowngradePackageVersion(sysroot_path, ebuild):
        logging.warning('Unable to downgrade package %s version number.',
                        ebuild)
    RemoveBzipPackages(sysroot_autotest_path)

    sentinel_filename = os.path.join(sysroot_autotest_path,
                                     '.quickmerge_sentinel')
    cros_build_lib.RunCommand(['touch', sentinel_filename])

  if args.pretend:
    logging.info('The following message is pretend only. No filesystem '
                 'changes made.')
  logging.info('Quickmerge complete. Created or modified %s files.',
               num_new_files + num_modified_files)

  return 0
