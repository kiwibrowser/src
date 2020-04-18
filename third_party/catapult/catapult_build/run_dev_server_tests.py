#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

from hooks import install

from py_utils import binary_manager
from py_utils import dependency_util
from py_utils import xvfb


# Path to dependency manager config containing chrome binary data.
CHROME_BINARIES_CONFIG = dependency_util.ChromeBinariesConfigPath()

CHROME_CONFIG_URL = (
    'https://code.google.com/p/chromium/codesearch#chromium/src/third_party/'
    'catapult/py_utils/py_utils/chrome_binaries.json')

# Default port to run on if not auto-assigning from OS
DEFAULT_PORT = '8111'

# Mapping of sys.platform -> platform-specific names and paths.
PLATFORM_MAPPING = {
    'linux2': {
        'omaha': 'linux',
        'prefix': 'Linux_x64',
        'zip_prefix': 'linux',
        'chromepath': 'chrome-linux/chrome'
    },
    'win32': {
        'omaha': 'win',
        'prefix': 'Win',
        'zip_prefix': 'win32',
        'chromepath': 'chrome-win32\\chrome.exe',
    },
    'darwin': {
        'omaha': 'mac',
        'prefix': 'Mac',
        'zip_prefix': 'mac',
        'chromepath': ('chrome-mac/Chromium.app/Contents/MacOS/Chromium'),
        'version_path': 'chrome-mac/Chromium.app/Contents/Versions/',
        'additional_paths': [
            ('chrome-mac/Chromium.app/Contents/Versions/%VERSION%/'
             'Chromium Helper.app/Contents/MacOS/Chromium Helper'),
        ],
    },
}


def IsDepotToolsPath(path):
  return os.path.isfile(os.path.join(path, 'gclient'))


def FindDepotTools():
  # Check if depot_tools is already in PYTHONPATH
  for path in sys.path:
    if path.rstrip(os.sep).endswith('depot_tools') and IsDepotToolsPath(path):
      return path

  # Check if depot_tools is in the path
  for path in os.environ['PATH'].split(os.pathsep):
    if IsDepotToolsPath(path):
      return path.rstrip(os.sep)

  return None


def GetLocalChromePath(path_from_command_line):
  if path_from_command_line:
    return path_from_command_line

  if sys.platform == 'darwin':  # Mac
    chrome_path = (
        '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome')
    if os.path.isfile(chrome_path):
      return chrome_path
  elif sys.platform.startswith('linux'):
    found = False
    try:
      with open(os.devnull, 'w') as devnull:
        found = subprocess.call(['google-chrome', '--version'],
                                stdout=devnull, stderr=devnull) == 0
    except OSError:
      pass
    if found:
      return 'google-chrome'
  elif sys.platform == 'win32':
    search_paths = [os.getenv('PROGRAMFILES(X86)'),
                    os.getenv('PROGRAMFILES'),
                    os.getenv('LOCALAPPDATA')]
    chrome_path = os.path.join('Google', 'Chrome', 'Application', 'chrome.exe')
    for search_path in search_paths:
      test_path = os.path.join(search_path, chrome_path)
      if os.path.isfile(test_path):
        return test_path
  return None


def Main(argv):
  try:
    parser = argparse.ArgumentParser(
        description='Run dev_server tests for a project.')
    parser.add_argument('--chrome_path', type=str,
                        help='Path to Chrome browser binary.')
    parser.add_argument('--no-use-local-chrome',
                        dest='use_local_chrome', action='store_false')
    parser.add_argument(
        '--no-install-hooks', dest='install_hooks', action='store_false')
    parser.add_argument('--tests', type=str,
                        help='Set of tests to run (tracing or perf_insights)')
    parser.add_argument('--channel', type=str, default='stable',
                        help='Chrome channel to run (stable or canary)')
    parser.add_argument('--presentation-json', type=str,
                        help='Recipe presentation-json output file path')
    parser.set_defaults(install_hooks=True)
    parser.set_defaults(use_local_chrome=True)
    args = parser.parse_args(argv[1:])

    if args.install_hooks:
      install.InstallHooks()

    user_data_dir = tempfile.mkdtemp()
    tmpdir = None
    xvfb_process = None

    server_path = os.path.join(os.path.dirname(
        os.path.abspath(__file__)), os.pardir, 'bin', 'run_dev_server')
    # TODO(anniesullie): Make OS selection of port work on Windows. See #1235.
    if sys.platform == 'win32':
      port = DEFAULT_PORT
    else:
      port = '0'
    server_command = [server_path, '--no-install-hooks', '--port', port]
    if sys.platform.startswith('win'):
      server_command = ['python.exe'] + server_command
    print "Starting dev_server..."
    server_process = subprocess.Popen(
        server_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        bufsize=1)
    time.sleep(1)
    if sys.platform != 'win32':
      output = server_process.stderr.readline()
      port = re.search(
          r'Now running on http://127.0.0.1:([\d]+)', output).group(1)

    chrome_info = None
    if args.use_local_chrome:
      chrome_path = GetLocalChromePath(args.chrome_path)
      if not chrome_path:
        logging.error('Could not find path to chrome.')
        sys.exit(1)
      chrome_info = 'with command `%s`' % chrome_path
    else:
      channel = args.channel
      if sys.platform == 'linux2' and channel == 'canary':
        channel = 'dev'
      assert channel in ['stable', 'beta', 'dev', 'canary']

      print 'Fetching the %s chrome binary via the binary_manager.' % channel
      chrome_manager = binary_manager.BinaryManager([CHROME_BINARIES_CONFIG])
      arch, os_name = dependency_util.GetOSAndArchForCurrentDesktopPlatform()
      chrome_path, version = chrome_manager.FetchPathWithVersion(
          'chrome_%s' % channel, arch, os_name)
      print 'Finished fetching the chrome binary to %s' % chrome_path
      if xvfb.ShouldStartXvfb():
        print 'Starting xvfb...'
        xvfb_process = xvfb.StartXvfb()
      chrome_info = 'version %s from channel %s' % (version, channel)
    chrome_command = [
        chrome_path,
        '--user-data-dir=%s' % user_data_dir,
        '--no-sandbox',
        '--no-experiments',
        '--no-first-run',
        '--noerrdialogs',
        '--window-size=1280,1024',
        ('http://localhost:%s/%s/tests.html?' % (port, args.tests)) +
        'headless=true&testTypeToRun=all',
    ]
    print "Starting Chrome %s..." % chrome_info
    chrome_process = subprocess.Popen(
        chrome_command, stdout=sys.stdout, stderr=sys.stderr)
    print 'chrome process command: %s' % ' '.join(chrome_command)
    print "Waiting for tests to finish..."
    server_out, server_err = server_process.communicate()
    print "Killing Chrome..."
    if sys.platform == 'win32':
      # Use taskkill on Windows to make sure Chrome and all subprocesses are
      # killed.
      subprocess.call(['taskkill', '/F', '/T', '/PID', str(chrome_process.pid)])
    else:
      chrome_process.kill()
    if server_process.returncode != 0:
      logging.error('Tests failed!')
      logging.error('Server stdout:\n%s', server_out)
      logging.error('Server stderr:\n%s', server_err)
    else:
      print server_out
    if args.presentation_json:
      with open(args.presentation_json, 'w') as recipe_out:
        # Add a link to the buildbot status for the step saying which version
        # of Chrome the test ran on. The actual linking feature is not used,
        # but there isn't a way to just add text.
        link_name = 'Chrome Version %s' % version
        presentation_info = {'links': {link_name: CHROME_CONFIG_URL}}
        json.dump(presentation_info, recipe_out)
  finally:
    # Wait for Chrome to be killed before deleting temp Chrome dir. Only have
    # this timing issue on Windows.
    if sys.platform == 'win32':
      time.sleep(5)
    if tmpdir:
      try:
        shutil.rmtree(tmpdir)
        shutil.rmtree(user_data_dir)
      except OSError as e:
        logging.error('Error cleaning up temp dirs %s and %s: %s',
                      tmpdir, user_data_dir, e)
    if xvfb_process:
      xvfb_process.kill()

  sys.exit(server_process.returncode)
