# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

import os


def _LicenseHeader(input_api):
  """Returns the license header regexp."""
  # Accept any year number from 2003 to the current year
  current_year = int(input_api.time.strftime('%Y'))
  allowed_years = (str(s) for s in reversed(xrange(2003, current_year + 1)))
  years_re = '(' + '|'.join(allowed_years) + ')'
  license_header = (
      r'.*? Copyright( \(c\))? %(year)s The WebRTC [Pp]roject [Aa]uthors\. '
        r'All [Rr]ights [Rr]eserved\.\n'
      r'.*?\n'
      r'.*? Use of this source code is governed by a BSD-style license\n'
      r'.*? that can be found in the LICENSE file in the root of the source\n'
      r'.*? tree\. An additional intellectual property rights grant can be '
        r'found\n'
      r'.*? in the file PATENTS\.  All contributing project authors may\n'
      r'.*? be found in the AUTHORS file in the root of the source tree\.\n'
  ) % {
      'year': years_re,
  }
  return license_header

def _CheckValgrindFiles(input_api, output_api):
  """Check that valgrind-webrtc.gni contains all existing files."""
  valgrind_dir = os.path.join('tools_webrtc', 'valgrind')
  with open(os.path.join('valgrind', 'valgrind-webrtc.gni')) as f:
    valgrind_webrtc = f.read()

  results = []
  for f in input_api.AffectedFiles():
    f = f.LocalPath()
    if (f.startswith(valgrind_dir)
        and f not in valgrind_webrtc
        and not f.endswith('valgrind-webrtc.gni')):
      results.append(' * %s\n' % f)

  if results:
    results = [output_api.PresubmitError(
      'The following files are not listed in '
      'tools_webrtc/valgrind/valgrind-webrt.gni. Please add them, so they can '
      'be isolated and uploaded to swarming:\n' +
      ''.join(file_path for file_path in results))]
  return results

def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  results = []
  results.extend(input_api.canned_checks.CheckLicense(
      input_api, output_api, _LicenseHeader(input_api)))
  results.extend(_CheckValgrindFiles(input_api, output_api))
  return results

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results

def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results
