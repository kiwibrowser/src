# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import contextlib
import io
import json
import os
import logging
import subprocess
import sys
import tempfile
import time


SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.abspath(
    os.path.join(SCRIPT_DIR, os.path.pardir, os.path.pardir))


# run_web_tests.py returns the number of failures as the return
# code, but caps the return code at 101 to avoid overflow or colliding
# with reserved values from the shell.
MAX_FAILURES_EXIT_STATUS = 101


# Exit code to indicate infrastructure issue.
INFRA_FAILURE_EXIT_CODE = 87


def run_script(argv, funcs):
  def parse_json(path):
    with open(path) as f:
      return json.load(f)
  parser = argparse.ArgumentParser()
  # TODO(phajdan.jr): Make build-config-fs required after passing it in recipe.
  parser.add_argument('--build-config-fs')
  parser.add_argument('--paths', type=parse_json, default={})
  # Properties describe the environment of the build, and are the same per
  # script invocation.
  parser.add_argument('--properties', type=parse_json, default={})
  # Args contains per-invocation arguments that potentially change the
  # behavior of the script.
  parser.add_argument('--args', type=parse_json, default=[])

  parser.add_argument(
      '--use-src-side-runtest-py', action='store_true',
      help='Use the src-side copy of runtest.py, as opposed to the build-side '
           'one')

  subparsers = parser.add_subparsers()

  run_parser = subparsers.add_parser('run')
  run_parser.add_argument(
      '--output', type=argparse.FileType('w'), required=True)
  run_parser.add_argument('--filter-file', type=argparse.FileType('r'))
  run_parser.set_defaults(func=funcs['run'])

  run_parser = subparsers.add_parser('compile_targets')
  run_parser.add_argument(
      '--output', type=argparse.FileType('w'), required=True)
  run_parser.set_defaults(func=funcs['compile_targets'])

  args = parser.parse_args(argv)
  return args.func(args)


def run_command(argv, env=None, cwd=None):
  print 'Running %r in %r (env: %r)' % (argv, cwd, env)
  rc = subprocess.call(argv, env=env, cwd=cwd)
  print 'Command %r returned exit code %d' % (argv, rc)
  return rc


def run_runtest(cmd_args, runtest_args):
  env = os.environ.copy()
  env['CHROME_HEADLESS'] = '1'

  if cmd_args.use_src_side_runtest_py:
    cmd = [
      sys.executable,
      os.path.join(
          cmd_args.paths['checkout'], 'infra', 'scripts', 'runtest_wrapper.py'),
      '--',
    ]
  else:
    cmd = [
      sys.executable,
      cmd_args.paths['runit.py'],
      '--show-path',
      '--with-third-party-lib',
      sys.executable,
      cmd_args.paths['runtest.py'],
    ]
  return run_command(cmd + [
      '--target', cmd_args.build_config_fs,
      '--xvfb',
      '--builder-name', cmd_args.properties['buildername'],
      '--slave-name', cmd_args.properties['slavename'],
      '--build-number', str(cmd_args.properties['buildnumber']),
      '--build-properties', json.dumps(cmd_args.properties),
  ] + runtest_args, env=env)


@contextlib.contextmanager
def temporary_file():
  fd, path = tempfile.mkstemp()
  os.close(fd)
  try:
    yield path
  finally:
    os.remove(path)


def parse_common_test_results(json_results, test_separator='/'):
  def convert_trie_to_flat_paths(trie, prefix=None):
    # Also see blinkpy.web_tests.layout_package.json_results_generator
    result = {}
    for name, data in trie.iteritems():
      if prefix:
        name = prefix + test_separator + name
      if len(data) and not 'actual' in data and not 'expected' in data:
        result.update(convert_trie_to_flat_paths(data, name))
      else:
        result[name] = data
    return result

  results = {
    'passes': {},
    'unexpected_passes': {},
    'failures': {},
    'unexpected_failures': {},
    'flakes': {},
    'unexpected_flakes': {},
  }

  # TODO(dpranke): crbug.com/357866 - we should simplify the handling of
  # both the return code and parsing the actual results, below.

  passing_statuses = ('PASS', 'SLOW', 'NEEDSREBASELINE',
                        'NEEDSMANUALREBASELINE')

  for test, result in convert_trie_to_flat_paths(
      json_results['tests']).iteritems():
    key = 'unexpected_' if result.get('is_unexpected') else ''
    data = result['actual']
    actual_results = data.split()
    last_result = actual_results[-1]
    expected_results = result['expected'].split()

    if (len(actual_results) > 1 and
        (last_result in expected_results or last_result in passing_statuses)):
      key += 'flakes'
    elif last_result in passing_statuses:
      key += 'passes'
      # TODO(dpranke): crbug.com/357867 ...  Why are we assigning result
      # instead of actual_result here. Do we even need these things to be
      # hashes, or just lists?
      data = result
    else:
      key += 'failures'
    results[key][test] = data

  return results


def extract_filter_list(filter_list):
  """Helper for isolated script test wrappers. Parses the
  --isolated-script-test-filter command line argument. Currently, double-colon
  ('::') is used as the separator between test names, because a single colon may
  be used in the names of perf benchmarks, which contain URLs.
  """
  return filter_list.split('::')


def run_integration_test(script_to_run, extra_args, log_file, output):
  integration_test_res = subprocess.call(
      [sys.executable, script_to_run] + extra_args)

  with open(log_file) as f:
    failures = json.load(f)
  json.dump({
      'valid': integration_test_res == 0,
      'failures': failures,
  }, output)

  return integration_test_res
