#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys

from catapult_build import temp_deployment_dir


def DevAppserver(paths, args):
  """Starts a dev server for an App Engine app.

  Args:
    paths: List of paths to files and directories that should be linked
        (or copied) in the deployment directory.
    args: List of additional arguments to pass to the dev server.
  """
  with temp_deployment_dir.TempDeploymentDir(paths) as temp_dir:
    print 'Running dev server on "%s".' % temp_dir

    script_path = _FindScriptInPath('dev_appserver.py')
    if not script_path:
      print 'This script requires the App Engine SDK to be in PATH.'
      sys.exit(1)

    subprocess.call([sys.executable, script_path] + args + [temp_dir])


def _FindScriptInPath(script_name):
  for path in os.environ['PATH'].split(os.pathsep):
    script_path = os.path.join(path, script_name)
    if os.path.exists(script_path):
      return script_path

  return None
