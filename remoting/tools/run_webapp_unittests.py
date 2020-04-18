#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Launches the remoting webapp unit tests in chrome with the appropriate flags.
"""

import argparse
import os
import platform
import sys
import tempfile
import urllib

def GetChromePath():
  """Locates the chrome binary on the system."""
  chrome_path = ''
  if platform.system() == 'Darwin': # Darwin == MacOSX
    chrome_path = (
      '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome')
  elif platform.system() == 'Linux':
    chrome_path = '/usr/bin/google-chrome'
  else:
    # TODO(kelvinp): Support chrome path location on Windows.
    print 'Unsupported OS.'
  return chrome_path


def BuildTestPageUri(build_path, opt_module=None, opt_coverage=False):
  """Builds the Uri for the test page with params."""
  script_path = os.path.dirname(__file__)

  test_page_path = os.path.join(script_path,
      '../../' + build_path + '/remoting/unittests/unittests.html')
  test_page_path = 'file://' + os.path.abspath(test_page_path)

  test_page_params = {}
  if opt_coverage:
    test_page_params['coverage'] = 'true'
  if opt_module:
    test_page_params['module'] = opt_module
  if test_page_params:
    test_page_path = test_page_path + '?%s' % urllib.urlencode(test_page_params)
  return '"' + test_page_path + '"'


def BuildCommandLine(chrome_path, build_path, opt_module, opt_coverage):
  """Builds the command line to execute."""
  command = []
  command.append('"' + chrome_path + '"')
  command.append('--user-data-dir=' + tempfile.gettempdir())
  # The flag |--allow-file-access-from-files| is required so that we can open
  # JavaScript files using XHR and instrument them for code coverage.
  command.append(' --allow-file-access-from-files')
  test_page_path = BuildTestPageUri(build_path, opt_module, opt_coverage)
  command.append(test_page_path)
  return ' '.join(command)


def ParseArgs():
  parser = argparse.ArgumentParser()
  chrome_path = GetChromePath()

  parser.add_argument(
    '--chrome-path',
    help='The path of the chrome binary to run the test.',
    default=chrome_path)
  parser.add_argument(
    '--module',
    help='only run tests that belongs to MODULE')
  parser.add_argument(
    '--coverage',
    help='run the test with code coverage',
    action='store_true')
  parser.add_argument(
    '--build-path',
    help='The output build path for remoting. (out/Debug)',
    default='out/Debug')

  return parser.parse_args(sys.argv[1:])


def main():
  args = ParseArgs()
  command_line = ""

  if not os.path.exists(args.chrome_path):
    print 'Cannot locate the chrome binary in your system.'
    print 'Please use the flag --chrome_path=CHROME_PATH to specify the chrome '
    print 'binary to run the test.'
    return 1

  command_line = BuildCommandLine(
      args.chrome_path,
      args.build_path,
      args.module,
      args.coverage)
  os.system(command_line)
  return 0


if __name__ == '__main__':
  sys.exit(main())
