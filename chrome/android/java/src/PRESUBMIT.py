# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for Android Java code.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.

This presubmit checks for the following:
  - No new calls to Notification.Builder or NotificationCompat.Builder
    constructors. Callers should use ChromeNotificationBuilder instead.
"""

import re

NEW_NOTIFICATION_BUILDER_RE = re.compile(
    r'\bnew\sNotification(Compat)?\.Builder\b')

COMMENT_RE = re.compile(r'^\s*(//|/\*|\*)')

def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)

def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  result = []
  result.extend(_CheckNotificationConstructors(input_api, output_api))
  # Add more checks here
  return result

def _CheckNotificationConstructors(input_api, output_api):
  # "Blacklist" because the following files are excluded from the check.
  blacklist = (
      'chrome/android/java/src/org/chromium/chrome/browser/notifications/'
          'NotificationBuilder.java',
      'chrome/android/java/src/org/chromium/chrome/browser/notifications/'
          'NotificationCompatBuilder.java'
  )
  problems = []
  sources = lambda x: input_api.FilterSourceFile(
      x, white_list=(r'\.java$',), black_list=blacklist)
  for f in input_api.AffectedFiles(include_deletes=False,
                                   file_filter=sources):
    for line_number, line in f.ChangedContents():
      if (NEW_NOTIFICATION_BUILDER_RE.search(line)
          and not COMMENT_RE.search(line)):
        problems.append(
          '  %s:%d\n    \t%s' % (f.LocalPath(), line_number, line.strip()))
  if problems:
    return [output_api.PresubmitError(
'''
Android Notification Construction Check failed:
  Your new code added one or more calls to the Notification.Builder and/or
  NotificationCompat.Builder constructors, listed below.

  This is banned, please construct notifications using
  NotificationBuilderFactory.createChromeNotificationBuilder instead,
  specifying a channel for use on Android O.

  See https://crbug.com/678670 for more information.
''',
      problems)]
  return []
