# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import sys

# This is the list of tests to run. It is a dictionary with the following
# fields:
#
# name (required): The name of the step, to show on the buildbot status page.
# path (required): The path to the executable which runs the tests.
# additional_args (optional): An array of optional arguments.
# uses_app_engine_sdk (optional): True if app engine SDK must be in PYTHONPATH.
# uses_sandbox_env (optional): True if CHROME_DEVEL_SANDBOX must be in
#   environment.
# disabled (optional): List of platforms the test is disabled on. May contain
#   'win', 'mac', 'linux', or 'android'.
# outputs_presentation_json (optional): If True, pass in --presentation-json
#   argument to the test executable to allow it to update the buildbot status
#   page. More details here:
# github.com/luci/recipes-py/blob/master/recipe_modules/generator_script/api.py
_CATAPULT_TESTS = [
    {
        'name': 'BattOr Smoke Tests',
        'path': 'common/battor/battor/battor_wrapper_devicetest.py',
        'disabled': ['android'],
    },
    {
        'name': 'BattOr Unit Tests',
        'path': 'common/battor/bin/run_py_tests',
        'disabled': ['android'],
    },
    {
        'name': 'Build Python Tests',
        'path': 'catapult_build/bin/run_py_tests',
        'disabled': ['android'],
    },
    {
        'name': 'Common Tests',
        'path': 'common/bin/run_tests',
    },
    {
        'name': 'Dashboard Dev Server Tests Canary',
        'path': 'dashboard/bin/run_dev_server_tests',
        'additional_args': [
            '--no-install-hooks',
            '--no-use-local-chrome',
            '--channel=canary'
        ],
        'outputs_presentation_json': True,
        'disabled': ['android'],
    },
    {
        'name': 'Dashboard Dev Server Tests Stable',
        'path': 'dashboard/bin/run_dev_server_tests',
        'additional_args': [
            '--no-install-hooks',
            '--no-use-local-chrome',
            '--channel=stable',
        ],
        'outputs_presentation_json': True,
        'disabled': ['android'],
    },
    {
        'name': 'Dashboard Python Tests',
        'path': 'dashboard/bin/run_py_tests',
        'additional_args': ['--no-install-hooks'],
        'uses_app_engine_sdk': True,
        'disabled': ['android'],
    },
    {
        'name': 'Dependency Manager Tests',
        'path': 'dependency_manager/bin/run_tests',
    },
    {
        'name': 'Devil Device Tests',
        'path': 'devil/bin/run_py_devicetests',
        'disabled': ['win', 'mac', 'linux']
    },
    {
        'name': 'Devil Python Tests',
        'path': 'devil/bin/run_py_tests',
        'disabled': ['mac', 'win'],
    },
    {
        'name': 'eslint Tests',
        'path': 'common/eslint/bin/run_tests',
        'disabled': ['android'],
    },
    {
        'name': 'Native Heap Symbolizer Tests',
        'path': 'tracing/bin/run_symbolizer_tests',
        'disabled': ['android'],
    },
    {
        'name': 'Py-vulcanize Tests',
        'path': 'common/py_vulcanize/bin/run_py_tests',
        'additional_args': ['--no-install-hooks'],
        'disabled': ['android'],
    },
    {
        'name': 'Systrace Tests',
        'path': 'systrace/bin/run_tests',
    },
    {
        'name': 'Snap-it Tests',
        'path': 'telemetry/bin/run_snap_it_unittest',
        'additional_args': [
            '--browser=reference',
        ],
        'uses_sandbox_env': True,
        'disabled': ['android'],
    },
    {
        'name': 'Telemetry Tests with Stable Browser (Desktop)',
        'path': 'catapult_build/fetch_telemetry_deps_and_run_tests',
        'additional_args': [
            '--browser=reference',
            '--start-xvfb'
        ],
        'uses_sandbox_env': True,
        'disabled': ['android'],
    },
    {
        'name': 'Telemetry Tests with Stable Browser (Android)',
        'path': 'catapult_build/fetch_telemetry_deps_and_run_tests',
        'additional_args': [
            '--browser=reference',
            '--device=android',
            '--jobs=1'
        ],
        'uses_sandbox_env': True,
        'disabled': ['win', 'mac', 'linux']
    },
    {
        'name': 'Telemetry Integration Tests with Stable Browser',
        'path': 'telemetry/bin/run_browser_tests',
        'additional_args': [
            'SimpleBrowserTest',
            '--browser=reference',
        ],
        'uses_sandbox_env': True,
        'disabled': ['android', 'linux'],  # TODO(nedn): enable this on linux
    },
    {
        'name': 'Tracing Dev Server Tests Canary',
        'path': 'tracing/bin/run_dev_server_tests',
        'additional_args': [
            '--no-install-hooks',
            '--no-use-local-chrome',
            '--channel=canary'
        ],
        'outputs_presentation_json': True,
        'disabled': ['android'],
    },
    {
        'name': 'Tracing Dev Server Tests Stable',
        'path': 'tracing/bin/run_dev_server_tests',
        'additional_args': [
            '--no-install-hooks',
            '--no-use-local-chrome',
            '--channel=stable',
        ],
        'outputs_presentation_json': True,
        'disabled': ['android'],
    },
    {
        'name': 'Tracing D8 Tests',
        'path': 'tracing/bin/run_vinn_tests',
        'disabled': ['android'],
    },
    {
        'name': 'Tracing Python Tests',
        'path': 'tracing/bin/run_py_tests',
        'additional_args': ['--no-install-hooks'],
        'disabled': ['android'],
    },
    {
        'name': 'Vinn Tests',
        'path': 'third_party/vinn/bin/run_tests',
        'disabled': ['android'],
    },
    {
        'name': 'NetLog Viewer Dev Server Tests',
        'path': 'netlog_viewer/bin/run_dev_server_tests',
        'additional_args': [
            '--no-install-hooks',
            '--no-use-local-chrome',
        ],
        'disabled': ['android', 'win', 'mac', 'linux'],
    },
]

_STALE_FILE_TYPES = ['.pyc', '.pseudo_lock']


def main(args=None):
  """Send list of test to run to recipes generator_script.

  See documentation at:
  github.com/luci/recipes-py/blob/master/recipe_modules/generator_script/api.py
  """
  parser = argparse.ArgumentParser(description='Run catapult tests.')
  parser.add_argument('--api-path-checkout', help='Path to catapult checkout')
  parser.add_argument('--app-engine-sdk-pythonpath',
                      help='PYTHONPATH to include app engine SDK path')
  parser.add_argument('--platform',
                      help='Platform name (linux, mac, or win)')
  parser.add_argument('--output-json', help='Output for buildbot status page')
  args = parser.parse_args(args)

  steps = [{
      # Always remove stale files first. Not listed as a test above
      # because it is a step and not a test, and must be first.
      'name': 'Remove Stale files',
      'cmd': ['python',
              os.path.join(args.api_path_checkout,
                           'catapult_build', 'remove_stale_files.py'),
              args.api_path_checkout, ','.join(_STALE_FILE_TYPES)]
  }]
  if args.platform == 'android':
    # On Android, we need to prepare the devices a bit before using them in
    # tests. These steps are not listed as tests above because they aren't
    # tests and because they must precede all tests.
    steps.extend([
        {
            'name': 'Android: Recover Devices',
            'cmd': ['python',
                    os.path.join(args.api_path_checkout, 'devil', 'devil',
                                 'android', 'tools', 'device_recovery.py')],
        },
        {
            'name': 'Android: Provision Devices',
            'cmd': ['python',
                    os.path.join(args.api_path_checkout, 'devil', 'devil',
                                 'android', 'tools', 'provision_devices.py')],
        },
        {
            'name': 'Android: Device Status',
            'cmd': ['python',
                    os.path.join(args.api_path_checkout, 'devil', 'devil',
                                 'android', 'tools', 'device_status.py')],
        },
    ])

  for test in _CATAPULT_TESTS:
    if args.platform in test.get('disabled', []):
      continue
    step = {
        'name': test['name'],
        'env': {}
    }
    step['cmd'] = ['python', os.path.join(args.api_path_checkout, test['path'])]
    if step['name'] == 'Systrace Tests':
      step['cmd'] += ['--device=' + args.platform]
    if test.get('additional_args'):
      step['cmd'] += test['additional_args']
    if test.get('uses_app_engine_sdk'):
      step['env']['PYTHONPATH'] = args.app_engine_sdk_pythonpath
    if test.get('uses_sandbox_env'):
      step['env']['CHROME_DEVEL_SANDBOX'] = '/opt/chromium/chrome_sandbox'
    if test.get('outputs_presentation_json'):
      step['outputs_presentation_json'] = True
    steps.append(step)
  with open(args.output_json, 'w') as outfile:
    json.dump(steps, outfile)


if __name__ == '__main__':
  main(sys.argv[1:])
