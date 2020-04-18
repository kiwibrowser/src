#!/usr/bin/env python2.7
#
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is just a wrapper script around monochrome_apk_checker.py that
# understands and uses the isolated-script arguments

import argparse
import json
import sys
import subprocess
import time

def main():
  parser = argparse.ArgumentParser(prog='monochrome_apk_checker_wrapper')

  parser.add_argument('--script',
                      required=True,
                      help='The path to the monochrome_apk_checker.py script')
  parser.add_argument('--isolated-script-test-output',
                      required=True)
  # ignored, but required to satisfy the isolated_script interface.
  parser.add_argument('--isolated-script-test-perf-output')
  args, extra = parser.parse_known_args(sys.argv[1:])

  cmd = [args.script] + extra

  start_time = time.time()
  ret = subprocess.call(cmd)
  success = ret == 0

  # Schema is at //docs/testing/json_test_results_format.md
  with open(args.isolated_script_test_output, 'w') as fp:
    json.dump({
      'version': 3,
      'interrupted': False,
      'path_delimiter': '/',
      'seconds_since_epoch': start_time,
      'num_failures_by_type': {
        'PASS': int(success),
        'FAIL': int(not success),
      },
      'tests': {
        'monochrome_apk_checker': {
          'expected': 'PASS',
          'actual': 'PASS' if success else 'FAIL',
        }
      }
    }, fp)

  return ret


if __name__ == '__main__':
  sys.exit(main())
