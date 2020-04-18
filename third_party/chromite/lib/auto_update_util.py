# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This file contains util functions for the auto-update lib."""

from __future__ import print_function

import re

from chromite.lib import cros_logging as logging


LSB_RELEASE = '/etc/lsb-release'


def GetChromeosBuildInfo(lsb_release_content=None, regex=None):
  """Get chromeos build info in device under test as string. None on fail.

  Args:
    lsb_release_content: A string represents the content of lsb-release.
        If the caller is from drone, it can pass in the file content here.
    regex: A regular expression, refers to which line this func tries to fetch
        from lsb_release_content.

  Returns:
    A kind of chromeos build info in device under test as string. None on fail.
  """
  if not lsb_release_content or not regex:
    return None

  for line in lsb_release_content.split('\n'):
    m = re.match(regex, line)
    if m:
      return m.group(1)

  return None

def VersionMatch(build_version, release_version):
  """Compare release version from lsb-release with cros-version label.

  build_version is a string based on build name. It is prefixed with builder
  info and branch ID, e.g., lumpy-release/R43-6809.0.0.
  release_version is retrieved from lsb-release.
  These two values might not match exactly.

  The method is designed to compare version for following 6 scenarios with
  samples of build version and expected release version:
  1. trybot non-release build (paladin, pre-cq or test-ap build).
  build version:   trybot-lumpy-paladin/R27-3837.0.0-b123
  release version: 3837.0.2013_03_21_1340

  2. trybot release build.
  build version:   trybot-lumpy-release/R27-3837.0.0-b456
  release version: 3837.0.0

  3. buildbot official release build.
  build version:   lumpy-release/R27-3837.0.0
  release version: 3837.0.0

  4. non-official paladin rc build.
  build version:   lumpy-paladin/R27-3878.0.0-rc7
  release version: 3837.0.0-rc7

  5. chrome-perf build.
  build version:   lumpy-chrome-perf/R28-3837.0.0-b2996
  release version: 3837.0.0

  6. pgo-generate build.
  build version:   lumpy-release-pgo-generate/R28-3837.0.0-b2996
  release version: 3837.0.0-pgo-generate

  TODO: This logic has a bug if a trybot paladin build failed to be
  installed in a DUT running an older trybot paladin build with same
  platform number, but different build number (-b###). So to conclusively
  determine if a tryjob paladin build is imaged successfully, we may need
  to find out the date string from update url.

  Args:
    build_version: Build name for cros version, e.g.
        peppy-release/R43-6809.0.0 or R43-6809.0.0
    release_version: Release version retrieved from lsb-release,
        e.g., 6809.0.0

  Returns:
    True if the values match, otherwise returns False.
  """
  # If the build is from release, CQ or PFQ builder, cros-version label must
  # be ended with release version in lsb-release.

  if build_version.endswith(release_version):
    return True

  # Remove R#- and -b# at the end of build version
  stripped_version = re.sub(r'(R\d+-|-b\d+)', '', build_version)
  # Trim the builder info, e.g., trybot-lumpy-paladin/
  stripped_version = stripped_version.split('/')[-1]

  # Add toolchain here since is_trybot_non_release_build cannot detect build
  # like 'trybot-sentry-llvm-toolchain/R56-8885.0.0-b943'.
  is_trybot_non_release_build = re.match(
      r'.*trybot-.+-(paladin|pre-cq|test-ap|toolchain)', build_version)

  # Replace date string with 0 in release_version
  release_version_no_date = re.sub(r'\d{4}_\d{2}_\d{2}_\d+', '0',
                                   release_version)
  has_date_string = release_version != release_version_no_date

  is_pgo_generate_build = re.match(r'.+-pgo-generate', build_version)

  # Remove |-pgo-generate| in release_version
  release_version_no_pgo = release_version.replace('-pgo-generate', '')
  has_pgo_generate = release_version != release_version_no_pgo

  if is_trybot_non_release_build:
    if not has_date_string:
      logging.error('A trybot paladin or pre-cq build is expected. '
                    'Version "%s" is not a paladin or pre-cq  build.',
                    release_version)
      return False
    return stripped_version == release_version_no_date
  elif is_pgo_generate_build:
    if not has_pgo_generate:
      logging.error('A pgo-generate build is expected. Version '
                    '"%s" is not a pgo-generate build.',
                    release_version)
      return False
    return stripped_version == release_version_no_pgo
  else:
    if has_date_string:
      logging.error('Unexpected date found in a non trybot paladin or '
                    'pre-cq build.')
      return False
    # Versioned build, i.e., rc or release build.
    return stripped_version == release_version
