# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess


def LaunchXvfb():
  """Launches Xvfb for running Chrome in headless mode, and returns the
  subprocess."""
  xvfb_cmd = ['Xvfb', ':99', '-screen', '0', '1600x1200x24']
  return subprocess.Popen(xvfb_cmd, stdout=open(os.devnull, 'wb'),
                          stderr=subprocess.STDOUT)


def GetChromeEnvironment():
  """Returns the environment for Chrome to run in headless mode with Xvfb."""
  return {'DISPLAY': 'localhost:99'}
