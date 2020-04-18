# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting web_page_replay_go/.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os
import tempfile
import shutil

def _RunArgs(args, input_api, cwd, env):
  p = input_api.subprocess.Popen(args, stdout=input_api.subprocess.PIPE,
                                 stderr=input_api.subprocess.STDOUT,
                                 cwd=cwd, env=env)
  out, _ = p.communicate()
  return (out, p.returncode)


def _CommonChecks(input_api, output_api):
  """Performs common checks."""
  results = []
  if input_api.subprocess.call(
          "go  version",
          shell=True,
          stdout=input_api.subprocess.PIPE,
          stderr=input_api.subprocess.PIPE) != 0:
    results.append(output_api.PresubmitPromptOrNotify(
        'go binary is not found. Make sure to run unit tests if you change any '
        'Go files.'))
    return results

  # We want to build wpr from the local source. We do this by
  # making a temporary GOPATH that symlinks to our local directory.

  try:
    # Make a tmp GOPATH directory which points at the local repo.
    wpr_dir = input_api.PresubmitLocalPath()
    go_path_dir = tempfile.mkdtemp()
    repo_dir = os.path.join(go_path_dir, 'src/github.com/catapult-project')
    os.makedirs(repo_dir)
    os.symlink(os.path.abspath(os.path.join(wpr_dir, '..')),
               os.path.join(repo_dir, 'catapult'))

    # Preserve GOPATH, if it's defined, to avoid redownloading packages
    # that have already been downloaded.
    env = os.environ.copy()
    if 'GOPATH' in env:
      env['GOPATH'] = '%s:%s' % (env['GOPATH'], go_path_dir)
    else:
      env['GOPATH'] = go_path_dir

    cmd = ['go', 'get', '-d', './...']
    _RunArgs(cmd, input_api, wpr_dir, env)
    cmd = ['go', 'test',
           'github.com/catapult-project/catapult/web_page_replay_go/src/' +
           'webpagereplay']
    out, return_code = _RunArgs(cmd, input_api, wpr_dir, env)
    if return_code:
      results.append(output_api.PresubmitError(
          'webpagereplay tests failed.', long_text=out))
    print out

  finally:
    if go_path_dir:
      shutil.rmtree(go_path_dir)

  return results


def CheckChangeOnUpload(input_api, output_api):
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  return report


def CheckChangeOnCommit(input_api, output_api):
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  return report
