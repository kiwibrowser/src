#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""//testing/scripts wrapper for the network traffic annotation auditor checks.
This script is used to run traffic_annotation_auditor_tests.py on an FYI bot to
check that traffic_annotation_auditor has the same results when heuristics that
help it run fast and spam free on trybots are disabled."""

import json
import os
import sys


import common


def main_run(args):
  command_line = [
      sys.executable,
      os.path.join(common.SRC_DIR, 'tools', 'traffic_annotation', 'scripts',
                   'traffic_annotation_auditor_tests.py'),
      '--build-path',
      os.path.join(args.paths['checkout'], 'out', args.build_config_fs),
  ]
  rc = common.run_command(command_line)

  json.dump({
      'valid': True,
      'failures': ['Please refer to stdout for errors.'] if rc else [],
  }, args.output)

  return rc


def main_compile_targets(args):
  json.dump(['all'], args.output)


if __name__ == '__main__':
  funcs = {
    'run': main_run,
    'compile_targets': main_compile_targets,
  }
  sys.exit(common.run_script(sys.argv[1:], funcs))
