# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for native_client_sdk.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""


def CommonChecks(input_api, output_api):
  output = []
  disabled_warnings = [
    'F0401',  # Unable to import module
    'R0401',  # Cyclic import
    'W0613',  # Unused argument
    'W0403',  # relative import warnings
    'E1103',  # subprocess.communicate() generates these :(
    'R0201',  # method could be function (doesn't reference self)
  ]
  black_list = [
    r'src[\\\/]build_tools[\\\/]tests[\\\/].*',
    r'src[\\\/]build_tools[\\\/]sdk_tools[\\\/]third_party[\\\/].*',
    r'src[\\\/]doc[\\\/]*',
    r'src[\\\/]gonacl_appengine[\\\/]*',
  ]
  canned = input_api.canned_checks
  output.extend(canned.RunPylint(input_api, output_api, black_list=black_list,
                disabled_warnings=disabled_warnings))
  return output


def CheckChangeOnUpload(input_api, output_api):
  return CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CommonChecks(input_api, output_api)


def GetPreferredTryMasters(project, change):
  return {
    'master.tryserver.chromium.linux': {
      'linux_nacl_sdk': set(['defaulttests']),
      'linux_nacl_sdk_build': set(['defaulttests']),
    },
    'master.tryserver.chromium.win': {
      'win_nacl_sdk': set(['defaulttests']),
      'win_nacl_sdk_build': set(['defaulttests']),
    },
    'master.tryserver.chromium.mac': {
      'mac_nacl_sdk': set(['defaulttests']),
      'mac_nacl_sdk_build': set(['defaulttests']),
    }
  }
