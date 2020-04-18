#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
BUILD_DIR = os.path.join(NACL_DIR, 'build')

PACKAGE_VERSION_DIR = os.path.join(BUILD_DIR, 'package_version')
PACKAGE_VERSION_SCRIPT = os.path.join(PACKAGE_VERSION_DIR, 'package_version.py')


def UploadPackages(filename, is_try, is_sanitizer=False):
  """ Upload packages to Google Storage.

  Args:
    filename: File to read package descriptions from.
    is_try: True if the run is for a trybot, False if for a real buildbot.
    is_sanitizer: True if building with a sanitizer.
  """
  print '@@@BUILD_STEP upload_package_info@@@'
  sys.stdout.flush()

  buildbot_buildername = os.getenv('BUILDBOT_BUILDERNAME', None)
  if buildbot_buildername is None:
    print 'Error - could not obtain buildbot builder name'
    sys.exit(1)
  if not is_try:
    buildbot_revision = os.getenv('BUILDBOT_GOT_REVISION', None)
    if buildbot_revision is None:
      print 'Error - Could not obtain buildbot revision number'
      sys.exit(1)
    if is_sanitizer:
      upload_rev = '%s/%s' % (buildbot_buildername, buildbot_revision)
    else:
      upload_rev = buildbot_revision
    upload_args = []
  else:
    buildbot_buildnumber = os.getenv('BUILDBOT_BUILDNUMBER', None)
    if buildbot_buildnumber is None:
      print 'Error - could not obtain buildbot build number'
      sys.exit(1)
    upload_rev = '%s/%s' % (buildbot_buildername, buildbot_buildnumber)
    upload_args = ['--cloud-bucket', 'nativeclient-trybot/packages']

  with open(filename, 'rt') as f:
    for package_file in f.readlines():
      package_file = package_file.strip()
      pkg_name, pkg_ext = os.path.splitext(os.path.basename(package_file))
      pkg_target = os.path.basename(os.path.dirname(package_file))
      full_package_name = '%s/%s' % (pkg_target, pkg_name)

      subprocess.check_call([sys.executable,
                             PACKAGE_VERSION_SCRIPT] +
                            upload_args +
                            ['--annotate',
                             'upload',
                             '--skip-missing',
                             '--upload-package', full_package_name,
                             '--revision', upload_rev,
                             '--package-file', package_file])

def ExtractPackages(filename, overlay_packages=True, skip_missing=True):
  """ Extracts packages into the standard toolchain directory.

  Args:
    filename: File to read package descriptions from.
    overlay_packages: Uses packages overlaid on top of default packages.
    skip_missing: If not overlaying packages, do not error on missing tar files.
  """
  print '@@@BUILD_STEP extract_packages@@@'
  sys.stdout.flush()

  platform = pynacl.platform.GetOS()
  with open(filename, 'rt') as f:
    for package_file in f.readlines():
      package_file = package_file.strip()
      pkg_target_dir = os.path.dirname(package_file)
      tar_dir = os.path.dirname(pkg_target_dir)

      pkg_name, pkg_ext = os.path.splitext(os.path.basename(package_file))
      pkg_target = os.path.basename(pkg_target_dir)

      # Do not extract other platforms
      if not pkg_target.startswith(platform):
        continue

      full_package_name = '%s/%s' % (pkg_target, pkg_name)

      package_version_args = []
      extract_args = []

      if overlay_packages:
        extract_args += ['--overlay-tar-dir', tar_dir]
      else:
        package_version_args += ['--tar-dir', tar_dir]

      if skip_missing:
        extract_args += ['--skip-missing']

      cmd_args = ([sys.executable,
                  PACKAGE_VERSION_SCRIPT,
                  '--annotate',
                  '--packages', full_package_name] +
                  package_version_args +
                  ['extract'] +
                  extract_args)

      print 'Executing:', cmd_args
      subprocess.check_call([sys.executable,
                             PACKAGE_VERSION_SCRIPT,
                             '--annotate',
                             '--packages', full_package_name] +
                             package_version_args +
                             ['extract'] +
                             extract_args)

def main(args):
  parser = argparse.ArgumentParser()

  command_parser = parser.add_subparsers(title='command', dest='command')

  extract_cmd_parser = command_parser.add_parser('extract')
  extract_cmd_parser.add_argument(
    '--overlay-packages', dest='overlay_packages',
    action='store_true', default=False,
    help='Overlay packages on top of default packages')
  extract_cmd_parser.add_argument(
    '--skip-missing', dest='skip_missing',
    action='store_true', default=False,
    help='Skip missing packages upon extraction.')
  extract_cmd_parser.add_argument(
    '--packages', dest='packages_file',
    metavar='FILE', required=True,
    help='Packages file outputed by toolchain_build.')

  arguments = parser.parse_args(args)

  if arguments.command == 'extract':
    ExtractPackages(arguments.packages_file,
                    overlay_packages=arguments.overlay_packages,
                    skip_missing=arguments.skip_missing)
    return 0

  print 'Unknown Command:', arguments.command
  return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
