# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test Chrome using chromedriver.

If the webdriver API or the chromedriver.exe binary can't be found this
becomes a no-op. This is to allow running locally while the waterfalls get
setup. Once all locations have been provisioned and are expected to contain
these items the checks should be removed to ensure this is run on each test
and fails if anything is incorrect.
"""

import argparse
import atexit
import contextlib
import logging
import os
import shutil
import sys
import tempfile
import time

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(THIS_DIR, '..', '..', '..')
WEBDRIVER_PATH = os.path.join(
    SRC_DIR, r'third_party', 'webdriver', 'pylib')
TEST_HTML_FILE = 'file://' + os.path.join(THIS_DIR, 'test_page.html')


# Try and import webdriver
sys.path.insert(0, WEBDRIVER_PATH)
try:
  from selenium import webdriver
  from selenium.webdriver import ChromeOptions
except ImportError:
  # If a system doesn't have the webdriver API this is a no-op phase
  logging.info(
    'Chromedriver API (selenium.webdriver) is not installed available in '
    '%s. Exiting test_chrome_with_chromedriver.py' % WEBDRIVER_PATH)
  sys.exit(0)


@contextlib.contextmanager
def CreateChromedriver(args):
  """Create a webdriver object ad close it after."""

  def DeleteWithRetry(path, func):
    # There seems to be a race condition on the bots that causes the paths
    # to not delete because they are being used. This allows up to 2 seconds
    # to delete
    for _ in xrange(4):
      try:
        return func(path)
      except WindowsError:
        time.sleep(0.5)
    raise

  driver = None
  user_data_dir = tempfile.mkdtemp()
  fd, log_file = tempfile.mkstemp()
  os.close(fd)
  chrome_options = ChromeOptions()
  chrome_options.binary_location = args.chrome_path
  chrome_options.add_argument('user-data-dir=' + user_data_dir)
  chrome_options.add_argument('log-file=' + log_file)
  chrome_options.add_argument('enable-logging')
  chrome_options.add_argument('v=1')
  try:
    driver = webdriver.Chrome(
      args.chromedriver_path,
      chrome_options=chrome_options)
    yield driver
  except:
    with open(log_file) as fh:
      logging.error(fh.read())
    raise
  finally:
    if driver:
      driver.quit()
    DeleteWithRetry(log_file, os.remove)
    DeleteWithRetry(user_data_dir, shutil.rmtree)


def main():
  """Main entry point."""
  parser = parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('-q', '--quiet', action='store_true', default=False,
                      help='Reduce test runner output')
  parser.add_argument(
    '--chromedriver-path', default='chromedriver.exe', metavar='FILENAME',
    help='Path to chromedriver')
  parser.add_argument(
    'chrome_path', metavar='FILENAME', help='Path to chrome installer')
  args = parser.parse_args()

  # This test is run from src, but this script is called with a cwd of
  # chrome/test/mini_installer, so relative paths need to be compensated for.
  if not os.path.exists(args.chromedriver_path):
    args.chromedriver_path = os.path.join(
      '..', '..', '..', args.chromedriver_path)
  if not os.path.exists(args.chrome_path):
    args.chrome_path = os.path.join(
      '..', '..', '..', args.chrome_path)

  logging.basicConfig(
    format='[%(asctime)s:%(filename)s(%(lineno)d)] %(message)s',
    datefmt='%m%d/%H%M%S', level=logging.ERROR if args.quiet else logging.INFO)

  if not args.chrome_path:
    logging.error('The path to the chrome binary is required.')
    return -1
  # Check that chromedriver is correct.
  if not os.path.exists(args.chromedriver_path):
    # If we can't find chromedriver exit as a no-op.
    logging.info(
      'Cant find %s. Exiting test_chrome_with_chromedriver',
      args.chromedriver_path)
    return 0
  with CreateChromedriver(args) as driver:
    driver.get(TEST_HTML_FILE)
    assert driver.title == 'Chromedriver Test Page', (
        'The page title was not correct.')
    element = driver.find_element_by_tag_name('body')
    assert element.text == 'This is the test page', (
        'The page body was not correct')
  return 0


if __name__ == '__main__':
  sys.exit(main())
