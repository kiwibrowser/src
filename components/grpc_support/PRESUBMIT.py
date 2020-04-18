# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for components/grpc_support/.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook adds an extra line to the CL description in order to run Cronet
  tests in addition to CQ bots.
  """

  # TODO(https://crbug.com/712733): Remove this once Cronet bots are deployed on CQ.
  try_bots = ['master.tryserver.chromium.android:android_cronet_tester',
              'master.tryserver.chromium.mac:ios-simulator-cronet']

  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl, try_bots, 'Automatically added Cronet trybots to run tests on CQ.')
