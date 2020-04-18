# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script to verify that XIB changes are done with the right version.

See http://dev.chromium.org/developers/design-documents/mac-xib-files for more
information.
"""

import re

# Minimum is Mac OS X 10.9.5 (13F34).
HUMAN_DARWIN_VERSION = '10.9.5'
ALLOWED_DARWIN_VERSION = 13  # Darwin 13 = 10.9.
MINIMUM_DARWIN_RELEASE = 'F'  # Release F = 10.9.5.

MINIMUM_IB_VERSION = 5053  # Xcode 5.1.
MAXIMUM_IB_VERSION = 5056  # Xcode 5.1.1.
HUMAN_IB_VERSION = '>= 5.1, <= 5.1.1'

SYSTEM_VERSION_RE = r'<document .* systemVersion="([0-9]{,2})([A-Z])([0-9]+)"'

IB_VERSION_RE = \
    r'<plugIn identifier="com\.apple\.InterfaceBuilder\.CocoaPlugin" ' + \
    'version="([0-9]+)"/>'

def _CheckXIBSystemAndXcodeVersions(input_api, output_api, error_type):
  affected_xibs = [x for x in input_api.AffectedFiles()
      if x.LocalPath().endswith('.xib')]

  incorrect_system_versions = []
  incorrect_ib_versions = []

  for xib in affected_xibs:
    if len(xib.NewContents()) == 0:
      continue

    system_version = None
    ib_version = None

    new_contents = xib.NewContents()
    if not new_contents:
      # Deleting files is always fine.
      continue

    for line in new_contents:
      m = re.search(SYSTEM_VERSION_RE, line)
      if m:
        system_version = (m.group(1), m.group(2), m.group(3))

      m = re.search(IB_VERSION_RE, line)
      if m:
        ib_version = m.group(1)

      if system_version is not None and ib_version is not None:
        break

    if system_version is None:
      incorrect_system_versions.append(xib.LocalPath())
      continue
    if int(system_version[0]) != ALLOWED_DARWIN_VERSION:
      incorrect_system_versions.append(xib.LocalPath())
      continue
    if system_version[1] < MINIMUM_DARWIN_RELEASE:
      incorrect_system_versions.append(xib.LocalPath())
      continue

    if ib_version is None or int(ib_version) < MINIMUM_IB_VERSION or \
       int(ib_version) > MAXIMUM_IB_VERSION:
      incorrect_ib_versions.append(xib.LocalPath())
      continue

  problems = []
  if incorrect_system_versions:
    problems.append(error_type(
        'XIB files need to be saved on Mac OS X ' + HUMAN_DARWIN_VERSION,
        items=incorrect_system_versions))
  if incorrect_ib_versions:
    problems.append(error_type(
        'XIB files need to be saved using Xcode version ' + HUMAN_IB_VERSION,
        items=incorrect_ib_versions))
  return problems

def CheckChangeOnUpload(input_api, output_api):
  # Allow uploads to happen even if the presubmit fails, so that contributors
  # can ask their reviewer or another person to re-save the XIBs for them.
  return _CheckXIBSystemAndXcodeVersions(input_api, output_api,
      error_type=output_api.PresubmitPromptWarning)

def CheckChangeOnCommit(input_api, output_api):
  return _CheckXIBSystemAndXcodeVersions(input_api, output_api,
      error_type=output_api.PresubmitError)
