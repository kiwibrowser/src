# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This retrieves the latest warnings from the Chrome /analyze build machine, and
does a diff.
This script is intended to be run from retrieve_latest_warnings.bat which
fills out the functionality.
"""

import glob
import os
import subprocess
import sys
import urllib

if len(sys.argv) < 2:
  print 'Missing build number.'
  sys.exit(10)

build_number = int(sys.argv[1])

base_url = 'http://build.chromium.org/p/chromium.fyi/builders/' + \
  'Chromium%20Windows%20Analyze/'

print 'Finding recent builds on %s' % base_url
base_data = urllib.urlopen(base_url).read()
recent_off = base_data.find('Recent Builds:')
build_marker = 'success</td>    <td><a href="' + \
               '../../builders/Chromium%20Windows%20Analyze/builds/'
# For some reason I couldn't get regular expressions to work on this data.
latest_build_off = base_data.find(build_marker, recent_off) + len(build_marker)
if latest_build_off < len(build_marker):
  print 'Couldn\'t find successful build.'
  sys.exit(10)
latest_end_off = base_data.find('"', latest_build_off)
latest_build_str = base_data[latest_build_off:latest_end_off]
max_build_number = int(latest_build_str)
if build_number > max_build_number:
  print 'Requested build number (%d) is too high. Maximum is %d.' % \
        (build_number, max_build_number)
  sys.exit(10)
# Treat negative numbers specially
if sys.argv[1][0] == '-':
  build_number = max_build_number + build_number
  if build_number < 0:
    build_number = 0
  print 'Retrieving build number %d of %d' % (build_number, max_build_number)

# Found the last summary results in the current directory
results = glob.glob('analyze*_summary.txt')
results.sort()
previous = '%04d' % (build_number - 1)
if results:
  possible_previous = results[-1][7:11]
  if int(possible_previous) == build_number:
    if len(results) > 1:
      previous = results[-2][7:11]
  else:
    previous = possible_previous

data_descriptor = 'chromium/bb/chromium.fyi/Chromium_Windows_Analyze/' + \
                  '%d/+/recipes/steps/compile/0/stdout' % build_number
revision_url = base_url + 'builds/' + str(build_number)

# Retrieve the revision
revisionData = urllib.urlopen(revision_url).read()
key = 'Got Revision</td><td>'
off = revisionData.find(key) + len(key)
if off > len(key):
  revision = revisionData[off: off + 40]
  print 'Revision is "%s"' % revision
  print 'Environment variables can be set with set_analyze_revision.bat'
  payload = 'set ANALYZE_REVISION=%s\r\n' % revision
  payload += 'set ANALYZE_BUILD_NUMBER=%04d\r\n' % build_number
  payload += 'set ANALYZE_PREV_BUILD_NUMBER=%s\r\n' % previous
  open('set_analyze_revision.bat', 'wt').write(payload)

  # Retrieve the raw warning data
  print 'Retrieving raw build results. Please wait.'
  # Results are now retrieved using logdog. Instructions on how to install the
  # logdog tool can be found here:
  # https://bugs.chromium.org/p/chromium/issues/detail?id=698429#c1
  # In particular, from c:\src\logdog_cipd_root:
  # > depot_tools\cipd init
  # > depot_tools\cipd install infra/tools/luci/logdog/logdog/windows-amd64
  command = r'c:\src\logdog_cipd_root\logdog.exe cat %s' % data_descriptor
  data = str(subprocess.check_output(command))
  if 'ANALYZE_REPO' in os.environ:
    source_path = r'e:\b\build\slave\chromium_windows_analyze\build\src'
    dest_path = os.path.join(os.environ['ANALYZE_REPO'], 'src')
    data = data.replace(source_path, dest_path)
  output_name = 'analyze%04d_full.txt' % build_number
  open(output_name, 'w').write(data)
  print 'Done. Data is in %s' % output_name
else:
  print 'No revision information found!'
