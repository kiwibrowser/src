# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions specific to build slaves, shared by several buildbot scripts.
"""

import datetime
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

from common import chromium_utils

# These codes used to distinguish true errors from script warnings.
ERROR_EXIT_CODE = 1
WARNING_EXIT_CODE = 88


def WriteLogLines(logname, lines, perf=None):
  logname = logname.rstrip()
  lines = [line.rstrip() for line in lines]
  for line in lines:
    print '@@@STEP_LOG_LINE@%s@%s@@@' % (logname, line)
  if perf:
    perf = perf.rstrip()
    print '@@@STEP_LOG_END_PERF@%s@%s@@@' % (logname, perf)
  else:
    print '@@@STEP_LOG_END@%s@@@' % logname
