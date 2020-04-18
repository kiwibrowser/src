#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import sys


import common


PERF_DASHBOARD_URL = 'https://chromeperf.appspot.com'


def create_argparser():
  parser = argparse.ArgumentParser()
  parser.add_argument('--platform')
  parser.add_argument('--perf-id')
  parser.add_argument('--results-url', default=PERF_DASHBOARD_URL)
  return parser


def main_run(script_args):
  parser = create_argparser()
  parser.add_argument('prefix')
  args = parser.parse_args(script_args.args)

  with common.temporary_file() as tempfile_path:
    runtest_args = [
      '--test-type', 'sizes',
      '--run-python-script',
    ]
    if args.perf_id:
      runtest_args.extend([
          '--perf-id', args.perf_id,
          '--results-url=%s' % args.results_url,
          '--perf-dashboard-id=sizes',
          '--annotate=graphing',
      ])
    sizes_cmd = [
        os.path.join(
            common.SRC_DIR, 'infra', 'scripts', 'legacy', 'scripts', 'slave',
            'chromium', 'sizes.py'),
        '--json', tempfile_path
    ]
    if args.platform:
      sizes_cmd.extend(['--platform', args.platform])
    rc = common.run_runtest(script_args, runtest_args + sizes_cmd)
    with open(tempfile_path) as f:
      results = json.load(f)

  with open(os.path.join(common.SRC_DIR, 'tools', 'perf_expectations',
                         'perf_expectations.json')) as f:
    perf_expectations = json.load(f)

  valid = (rc == 0)
  failures = []

  for name, result in results.iteritems():
    fqtn = '%s/%s/%s' % (args.prefix, name, result['identifier'])
    if fqtn not in perf_expectations:
      continue

    if perf_expectations[fqtn]['type'] != 'absolute':
      print 'ERROR: perf expectation %r is not yet supported' % fqtn
      valid = False
      continue

    actual = result['value']
    expected = perf_expectations[fqtn]['regress']
    better = perf_expectations[fqtn]['better']
    check_result = ((actual <= expected) if better == 'lower'
                    else (actual >= expected))

    if not check_result:
      failures.append(fqtn)
      print 'FAILED %s: actual %s, expected %s, better %s' % (
          fqtn, actual, expected, better)

  json.dump({
      'valid': valid,
      'failures': failures,
  }, script_args.output)

  # sizes.py itself doesn't fail on regressions.
  if failures and rc == 0:
    rc = 1

  return rc


def main_compile_targets(script_args):
  parser = create_argparser()
  args = parser.parse_args(script_args.args)

  _COMPILE_TARGETS = {
    'android-cronet': ['cronet'],
    'android-webview': ['libwebviewchromium'],
  }

  json.dump(_COMPILE_TARGETS.get(args.platform, ['chrome']),
            script_args.output)


if __name__ == '__main__':
  funcs = {
    'run': main_run,
    'compile_targets': main_compile_targets,
  }
  sys.exit(common.run_script(sys.argv[1:], funcs))
