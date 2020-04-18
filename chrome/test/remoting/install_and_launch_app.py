#!/usr/bin/python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Chromedriver smoke-test that installs and launches a web-app.

  Args:
    driver_dir: Location of Chromedriver binary on local machine.
    profile_dir: A user-data-dir containing login token for the app-user.
    app_id: App ID of web-app in Chrome web-store.
    app_window_title: The title of the window that should come up on app launch.

    TODO(anandc): Reduce the # of parameters required from the command-line.
    Maybe read from a JSON file. Also, map appID to expected app window title.

  This script navigates to the app-detail page on Chrome Web Store for the
  specified app-id. From there, it then installs the app and launches it. It
  then checks if the resulting new window has the expected title.
"""

import argparse
import os
import shutil
import tempfile
import time

from selenium import webdriver
from selenium.webdriver.chrome.options import Options

CWS_URL = 'https://chrome.google.com/webstore/detail'
WEBSTORE_BUTTON_LABEL = 'webstore-test-button-label'
FREE_BUTTON_XPATH = (
    '//div[contains(@class, \"%s\") and text() = \"Free\"]' %
    (WEBSTORE_BUTTON_LABEL))
LAUNCH_BUTTON_XPATH = (
    '//div[contains(@class, \"%s\") and text() = \"Launch app\"]' %
    (WEBSTORE_BUTTON_LABEL))
WAIT_TIME = 2


def CreateTempProfileDir(source_dir):
  """Creates a temporary profile directory, for use by the test.

     This avoids modifying the input user-data-dir by actions that the test
     performs.

  Args:
    source_dir: The directory to copy and place in a temp folder.

  Returns:
    tmp_dir: Name of the temporary folder that was created.
    profile_dir: Name of the profile-dir under the tmp_dir.
  """

  tmp_dir = tempfile.mkdtemp()
  print 'Created folder %s' % (tmp_dir)
  profile_dir = os.path.join(tmp_dir, 'testuser')
  # Copy over previous created profile for this execution of Chrome Driver.
  shutil.copytree(source_dir, profile_dir)
  return tmp_dir, profile_dir


def ParseCmdLineArgs():
  """Parses command line arguments and returns them.

  Returns:
    args: Parse command line arguments.
  """
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-d', '--driver_dir', required=True,
      help='path to folder where Chromedriver has been installed.')
  parser.add_argument(
      '-p', '--profile_dir', required=True,
      help='path to user-data-dir with trusted-tester signed in.')
  parser.add_argument(
      '-a', '--app_id', required=True,
      help='app-id of web-store app being tested.')
  parser.add_argument(
      '-e', '--app_window_title', required=True,
      help='Title of the app window that we expect to come up.')

  # Use input json file if specified on command line.
  args = parser.parse_args()
  return args


def GetLinkAndWait(driver, link_to_get):
  """Navigates to the specified link.

  Args:
    driver: Active window for this Chromedriver instance.
    link_to_get: URL of the destination.
  """
  driver.get(link_to_get)
  # TODO(anandc): Is there any event or state we could wait on? For now,
  # we have hard-coded sleeps.
  time.sleep(WAIT_TIME)


def ClickAndWait(driver, button_xpath):
  """Clicks button at the specified XPath of the current document.

  Args:
    driver: Active window for this Chromedriver instance.
    button_xpath: XPath in this document to button we want to click.
  """
  button = driver.find_element_by_xpath(button_xpath)
  button.click()
  time.sleep(WAIT_TIME)


def WindowWithTitleExists(driver, title):
  """Verifies if one of the open windows has the specified title.

  Args:
    driver: Active window for this Chromedriver instance.
    title: Title of the window we are looking for.

  Returns:
    True if an open window in this session with the specified title was found.
    False otherwise.
  """
  for handle in driver.window_handles:
    driver.switch_to_window(handle)
    if driver.title == title:
      return True
  return False


def main():

  args = ParseCmdLineArgs()

  org_profile_dir = args.profile_dir
  print 'Creating temp-dir using profile-dir %s' % org_profile_dir
  tmp_dir, profile_dir = CreateTempProfileDir(org_profile_dir)

  options = Options()
  options.add_argument('--user-data-dir=' + profile_dir)
  # Suppress the confirmation dialog that comes up.
  # With M39, this flag will no longer work. See https://crbug/357774.
  # TODO(anandc): Work with a profile-dir that already has extension downloaded,
  # and also add support for loading extension from a local directory.
  options.add_argument('--apps-gallery-install-auto-confirm-for-tests=accept')
  driver = webdriver.Chrome(args.driver_dir, chrome_options=options)

  try:

    chrome_apps_link = 'chrome://apps'
    cws_app_detail_link = '%s/%s' % (CWS_URL, args.app_id)

    # Navigate to chrome:apps first.
    # TODO(anandc): Add check to make sure the app we are testing isn't already
    # added for this user.
    GetLinkAndWait(driver, chrome_apps_link)

    # Navigate to the app detail page at the Chrome Web Store.
    GetLinkAndWait(driver, cws_app_detail_link)
    # Get the page again, to get all controls. This seems to be a bug, either
    # in ChromeDriver, or the app-page. Without this additional GET, we don't
    # get all controls. Even sleeping for 5 seconds doesn't suffice.
    # TODO(anandc): Investigate why the page doesn't work with just 1 call.
    GetLinkAndWait(driver, cws_app_detail_link)

    # Install the app by clicking the button that says "Free".
    ClickAndWait(driver, FREE_BUTTON_XPATH)

    # We should now be at a new tab. Get its handle.
    current_tab = driver.window_handles[-1]
    # And switch to it.
    driver.switch_to_window(current_tab)

    # From this new tab, go to Chrome Apps
    # TODO(anandc): Add check to make sure the app we are testing is now added.
    GetLinkAndWait(driver, chrome_apps_link)

    # Back to the app detail page.
    GetLinkAndWait(driver, cws_app_detail_link)
    # Again, do this twice, for reasons noted above.
    GetLinkAndWait(driver, cws_app_detail_link)

    # Click to launch the newly installed app.
    ClickAndWait(driver, LAUNCH_BUTTON_XPATH)

    # For now, make sure the "connecting" dialog comes up.
    # TODO(anandc): Add more validation; ideally, wait for the separate app
    # window to appear.
    if WindowWithTitleExists(driver, args.app_window_title):
      print 'Web-App %s launched successfully.' % args.app_window_title
    else:
      print 'Web-app %s did not launch successfully.' % args.app_window_title

  except Exception, e:
    raise e
  finally:
    # Cleanup.
    print 'Deleting %s' % tmp_dir
    shutil.rmtree(profile_dir)
    os.rmdir(tmp_dir)
    driver.quit()


if __name__ == '__main__':
  main()
