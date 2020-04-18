# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Presubmit script for //testing/buildbot/filters.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook adds extra try bots to the CL description in order to run network
  service tests in addition to CQ try bots.
  """
  def affects_gn_checker(f):
    return 'mojo.fyi.network_' in f.LocalPath()
  if not change.AffectedFiles(file_filter=affects_gn_checker):
    return []
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'master.tryserver.chromium.linux:linux_mojo'
    ],
    'Automatically added network service trybots to run tests on CQ.')

