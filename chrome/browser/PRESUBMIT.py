# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for Chromium browser code."""

def _RunHistogramChecks(input_api, output_api, histogram_name):
  try:
    # Setup sys.path so that we can call histograms code.
    import sys
    original_sys_path = sys.path
    sys.path = sys.path + [input_api.os_path.join(
        input_api.change.RepositoryRoot(),
        'tools', 'metrics', 'histograms')]

    results = []

    import presubmit_bad_message_reasons
    results.extend(presubmit_bad_message_reasons.PrecheckBadMessage(input_api,
        output_api, histogram_name))

    import presubmit_scheme_histograms
    results.extend(presubmit_scheme_histograms.
                   PrecheckShouldAllowOpenURLEnums(input_api, output_api))

    return results
  except:
    return [output_api.PresubmitError('Could not verify histogram!')]
  finally:
    sys.path = original_sys_path


def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  return _RunHistogramChecks(input_api, output_api, "BadMessageReasonChrome")


def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)
