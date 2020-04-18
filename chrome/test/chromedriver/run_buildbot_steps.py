#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs all the buildbot steps for ChromeDriver except for update/compile."""

import bisect
import csv
import datetime
import glob
import json
import optparse
import os
import platform as platform_module
import re
import shutil
import StringIO
import sys
import tempfile
import time
import urllib2

_THIS_DIR = os.path.abspath(os.path.dirname(__file__))
GS_CHROMEDRIVER_BUCKET = 'gs://chromedriver'
GS_CHROMEDRIVER_DATA_BUCKET = 'gs://chromedriver-data'
GS_CHROMEDRIVER_RELEASE_URL = 'http://chromedriver.storage.googleapis.com'
GS_CONTINUOUS_URL = GS_CHROMEDRIVER_DATA_BUCKET + '/continuous'
GS_PREBUILTS_URL = GS_CHROMEDRIVER_DATA_BUCKET + '/prebuilts'
GS_SERVER_LOGS_URL = GS_CHROMEDRIVER_DATA_BUCKET + '/server_logs'
SERVER_LOGS_LINK = (
    'http://chromedriver-data.storage.googleapis.com/server_logs')
TEST_LOG_FORMAT = '%s_log.json'

SCRIPT_DIR = os.path.join(_THIS_DIR, os.pardir, os.pardir, os.pardir, os.pardir,
                          os.pardir, os.pardir, os.pardir, 'scripts')
SITE_CONFIG_DIR = os.path.join(_THIS_DIR, os.pardir, os.pardir, os.pardir,
                               os.pardir, os.pardir, os.pardir, os.pardir,
                               'site_config')
sys.path.append(SCRIPT_DIR)
sys.path.append(SITE_CONFIG_DIR)

import archive
import chrome_paths
from slave import gsutil_download
from slave import slave_utils
import util


def _ArchivePrebuilts(commit_position):
  """Uploads the prebuilts to google storage."""
  util.MarkBuildStepStart('archive prebuilts')
  zip_path = util.Zip(os.path.join(chrome_paths.GetBuildDir(['chromedriver']),
                                   'chromedriver'))
  if slave_utils.GSUtilCopy(
      zip_path,
      '%s/%s' % (GS_PREBUILTS_URL, 'r%s.zip' % commit_position)):
    util.MarkBuildStepError()


def _ArchiveServerLogs():
  """Uploads chromedriver server logs to google storage."""
  util.MarkBuildStepStart('archive chromedriver server logs')
  pathname_pattern = os.path.join(tempfile.gettempdir(), 'chromedriver_log_*')
  print 'archiving logs from: %s' % pathname_pattern
  for server_log in glob.glob(pathname_pattern):
    if os.path.isfile(server_log):
      base_name = os.path.basename(server_log)
      util.AddLink(base_name, '%s/%s' % (SERVER_LOGS_LINK, base_name))
      slave_utils.GSUtilCopy(
          server_log,
          '%s/%s' % (GS_SERVER_LOGS_URL, base_name),
          mimetype='text/plain')


def _DownloadPrebuilts():
  """Downloads the most recent prebuilts from google storage."""
  util.MarkBuildStepStart('Download latest chromedriver')

  zip_path = os.path.join(util.MakeTempDir(), 'build.zip')
  if gsutil_download.DownloadLatestFile(GS_PREBUILTS_URL,
                                        GS_PREBUILTS_URL + '/r',
                                        zip_path):
    util.MarkBuildStepError()

  util.Unzip(zip_path, chrome_paths.GetBuildDir(['host_forwarder']))


def _GetTestResultsLog(platform):
  """Gets the test results log for the given platform.

  Args:
    platform: The platform that the test results log is for.

  Returns:
    A dictionary where the keys are commit positions and the values are booleans
    indicating whether the tests passed.
  """
  (temp_fd, temp_log) = tempfile.mkstemp()
  os.close(temp_fd)
  log_name = TEST_LOG_FORMAT % platform
  result = slave_utils.GSUtilDownloadFile(
      '%s/%s' % (GS_CHROMEDRIVER_DATA_BUCKET, log_name), temp_log)
  if result:
    return {}
  with open(temp_log, 'rb') as log_file:
    json_dict = json.load(log_file)
  # Workaround for json encoding dictionary keys as strings.
  return dict([(int(v[0]), v[1]) for v in json_dict.items()])


def _PutTestResultsLog(platform, test_results_log):
  """Pushes the given test results log to google storage."""
  temp_dir = util.MakeTempDir()
  log_name = TEST_LOG_FORMAT % platform
  log_path = os.path.join(temp_dir, log_name)
  with open(log_path, 'wb') as log_file:
    json.dump(test_results_log, log_file)
  if slave_utils.GSUtilCopyFile(log_path, GS_CHROMEDRIVER_DATA_BUCKET):
    raise Exception('Failed to upload test results log to google storage')


def _UpdateTestResultsLog(platform, commit_position, passed):
  """Updates the test results log for the given platform.

  Args:
    platform: The platform name.
    commit_position: The commit position number.
    passed: Boolean indicating whether the tests passed at this commit position.
  """

  assert commit_position.isdigit(), 'The commit position must be a number'
  commit_position = int(commit_position)
  log = _GetTestResultsLog(platform)
  if len(log) > 500:
    del log[min(log.keys())]
  assert commit_position not in log, \
      'Results already exist for commit position %s' % commit_position
  log[commit_position] = bool(passed)
  _PutTestResultsLog(platform, log)


def _GetVersion():
  """Get the current chromedriver version."""
  with open(os.path.join(_THIS_DIR, 'VERSION'), 'r') as f:
    return f.read().strip()


def _GetSupportedChromeVersions():
  """Get the minimum and maximum supported Chrome versions.

  Returns:
    A tuple of the form (min_version, max_version).
  """
  # Minimum supported Chrome version is embedded as:
  # const int kMinimumSupportedChromeVersion[] = {27, 0, 1453, 0};
  with open(os.path.join(_THIS_DIR, 'chrome', 'version.cc'), 'r') as f:
    lines = f.readlines()
    chrome_min_version_line = [
        x for x in lines if 'kMinimumSupportedChromeVersion' in x]
  chrome_min_version = chrome_min_version_line[0].split('{')[1].split(',')[0]
  with open(os.path.join(chrome_paths.GetSrc(), 'chrome', 'VERSION'), 'r') as f:
    chrome_max_version = f.readlines()[0].split('=')[1].strip()
  return (chrome_min_version, chrome_max_version)


def _CommitPositionState(test_results_log, commit_position):
  """Check the state of tests at a given commit position.

  Considers tests as having passed at a commit position if they passed at
  revisions both before and after.

  Args:
    test_results_log: A test results log dictionary from _GetTestResultsLog().
    commit_position: The commit position to check at.

  Returns:
    'passed', 'failed', or 'unknown'
  """
  assert isinstance(commit_position, int), 'The commit position must be an int'
  keys = sorted(test_results_log.keys())
  # Return passed if the exact commit position passed on Android.
  if commit_position in test_results_log:
    return 'passed' if test_results_log[commit_position] else 'failed'
  # Tests were not run on this exact commit position on Android.
  index = bisect.bisect_right(keys, commit_position)
  # Tests have not yet run on Android at or above this commit position.
  if index == len(test_results_log):
    return 'unknown'
  # No log exists for any prior commit position, assume it failed.
  if index == 0:
    return 'failed'
  # Return passed if the commit position on both sides passed.
  if test_results_log[keys[index]] and test_results_log[keys[index - 1]]:
    return 'passed'
  return 'failed'


def _ArchiveGoodBuild(platform, commit_position):
  """Archive chromedriver binary if the build is green."""
  assert platform != 'android'
  util.MarkBuildStepStart('archive build')

  server_name = 'chromedriver'
  if util.IsWindows():
    server_name += '.exe'
  zip_path = util.Zip(os.path.join(chrome_paths.GetBuildDir([server_name]),
                                   server_name))

  build_name = 'chromedriver_%s_%s.%s.zip' % (
      platform, _GetVersion(), commit_position)
  build_url = '%s/%s' % (GS_CONTINUOUS_URL, build_name)
  if slave_utils.GSUtilCopy(zip_path, build_url):
    util.MarkBuildStepError()

  if util.IsWindows():
    zip_path = util.Zip(os.path.join(
        chrome_paths.GetBuildDir([server_name + '.pdb']), server_name + '.pdb'))
    pdb_name = 'chromedriver_%s_pdb_%s.%s.zip' % (
        platform, _GetVersion(), commit_position)
    pdb_url = '%s/%s' % (GS_CONTINUOUS_URL, pdb_name)
    if slave_utils.GSUtilCopy(zip_path, pdb_url):
      util.MarkBuildStepError()

  (latest_fd, latest_file) = tempfile.mkstemp()
  os.write(latest_fd, build_name)
  os.close(latest_fd)
  latest_url = '%s/latest_%s' % (GS_CONTINUOUS_URL, platform)
  if slave_utils.GSUtilCopy(latest_file, latest_url, mimetype='text/plain'):
    util.MarkBuildStepError()
  os.remove(latest_file)


def _WasReleased(version, platform):
  """Check if the specified version is released for the given platform."""
  result, _ = slave_utils.GSUtilListBucket(
      '%s/%s/chromedriver_%s.zip' % (GS_CHROMEDRIVER_BUCKET, version, platform),
      [])
  return result == 0


def _MaybeRelease(platform):
  """Releases a release candidate if conditions are right."""
  assert platform != 'android'

  version = _GetVersion()

  # Check if the current version has already been released.
  if _WasReleased(version, platform):
    return

  # Fetch Android test results.
  android_test_results = _GetTestResultsLog('android')

  # Fetch release candidates.
  result, output = slave_utils.GSUtilListBucket(
      '%s/chromedriver_%s_%s*' % (
          GS_CONTINUOUS_URL, platform, version),
      [])
  assert result == 0 and output, 'No release candidates found'
  candidate_pattern = re.compile(
      r'.*/chromedriver_%s_%s\.(\d+)\.zip$' % (platform, version))
  candidates = []
  for line in output.strip().split('\n'):
    result = candidate_pattern.match(line)
    if not result:
      print 'Ignored line "%s"' % line
      continue
    candidates.append(int(result.group(1)))

  # Release the latest candidate build that passed Android, if any.
  # In this way, if a hot fix is needed, we can delete the release from
  # the chromedriver bucket instead of bumping up the release version number.
  candidates.sort(reverse=True)
  for commit_position in candidates:
    android_result = _CommitPositionState(android_test_results, commit_position)
    if android_result == 'failed':
      print 'Android tests did not pass at commit position', commit_position
    elif android_result == 'passed':
      print 'Android tests passed at commit position', commit_position
      candidate = 'chromedriver_%s_%s.%s.zip' % (
          platform, version, commit_position)
      _Release('%s/%s' % (GS_CONTINUOUS_URL, candidate), version, platform)
      break
    else:
      print 'Android tests have not run at a commit position as recent as', \
          commit_position


def _Release(build, version, platform):
  """Releases the given candidate build."""
  release_name = 'chromedriver_%s.zip' % platform
  util.MarkBuildStepStart('releasing %s' % release_name)
  temp_dir = util.MakeTempDir()
  slave_utils.GSUtilCopy(build, temp_dir)
  zip_path = os.path.join(temp_dir, os.path.basename(build))

  if util.IsLinux():
    util.Unzip(zip_path, temp_dir)
    server_path = os.path.join(temp_dir, 'chromedriver')
    util.RunCommand(['strip', server_path])
    zip_path = util.Zip(server_path)

  slave_utils.GSUtilCopy(
      zip_path, '%s/%s/%s' % (GS_CHROMEDRIVER_BUCKET, version, release_name))

  _MaybeUpdateLatestRelease(version)


def _GetWebPageContent(url):
  """Return the content of the web page specified by the given url."""
  return urllib2.urlopen(url).read()


def _MaybeUpdateLatestRelease(version):
  """Update the file LATEST_RELEASE with the latest release version number."""
  latest_release_fname = 'LATEST_RELEASE'
  latest_release_url = '%s/%s' % (GS_CHROMEDRIVER_BUCKET, latest_release_fname)

  # Check if LATEST_RELEASE is up to date.
  latest_released_version = _GetWebPageContent(
      '%s/%s' % (GS_CHROMEDRIVER_RELEASE_URL, latest_release_fname))
  if version == latest_released_version:
    return

  # Check if chromedriver was released on all supported platforms.
  supported_platforms = ['linux64', 'mac64', 'win32']
  for platform in supported_platforms:
    if not _WasReleased(version, platform):
      return

  util.MarkBuildStepStart('updating LATEST_RELEASE to %s' % version)

  temp_latest_release_fname = tempfile.mkstemp()[1]
  with open(temp_latest_release_fname, 'w') as f:
    f.write(version)
  if slave_utils.GSUtilCopy(temp_latest_release_fname, latest_release_url,
                            mimetype='text/plain'):
    util.MarkBuildStepError()


def _WaitForLatestSnapshot(commit_position):
  util.MarkBuildStepStart('wait_for_snapshot')
  for attempt in range(0, 200):
    snapshot_position = archive.GetLatestSnapshotPosition()
    if commit_position is not None and snapshot_position is not None:
      if int(snapshot_position) >= int(commit_position):
        break
      util.PrintAndFlush('Waiting for snapshot >= %s, found %s' %
                         (commit_position, snapshot_position))
    time.sleep(60)
  if int(snapshot_position) < int(commit_position):
    raise Exception('Failed to find a snapshot version >= %s' % commit_position)
  util.PrintAndFlush('Got snapshot commit position %s' % snapshot_position)


def _AddToolsToPath(platform_name):
  """Add some tools like Ant and Java to PATH for testing steps to use."""
  paths = []
  error_message = ''
  if platform_name == 'win32':
    paths = [
        # Path to Ant and Java, required for the java acceptance tests.
        'C:\\Program Files (x86)\\Java\\ant\\bin',
        'C:\\Program Files (x86)\\Java\\jre\\bin',
    ]
    error_message = ('Java test steps will fail as expected and '
                     'they can be ignored.\n'
                     'Ant, Java or others might not be installed on bot.\n'
                     'Please refer to page "WATERFALL" on site '
                     'go/chromedriver.')
  if paths:
    util.MarkBuildStepStart('Add tools to PATH')
    path_missing = False
    for path in paths:
      if not os.path.isdir(path) or not os.listdir(path):
        print 'Directory "%s" is not found or empty.' % path
        path_missing = True
    if path_missing:
      print error_message
      util.MarkBuildStepError()
      return
    os.environ['PATH'] += os.pathsep + os.pathsep.join(paths)


def main():
  parser = optparse.OptionParser()
  parser.add_option(
      '', '--android-packages',
      help=('Comma separated list of application package names, '
            'if running tests on Android.'))
  parser.add_option(
      '-r', '--revision', help='Chromium git revision hash')
  parser.add_option(
      '', '--update-log', action='store_true',
      help='Update the test results log (only applicable to Android)')
  options, _ = parser.parse_args()

  bitness = '32'
  if util.Is64Bit():
    bitness = '64'
  platform = '%s%s' % (util.GetPlatformName(), bitness)
  if options.android_packages:
    platform = 'android'

  if not options.revision:
    commit_position = None
  else:
    commit_position = archive.GetCommitPositionFromGitHash(options.revision)
    if commit_position is None:
      raise Exception('Failed to convert revision to commit position')

  if platform == 'android':
    if not options.revision and options.update_log:
      parser.error('Must supply a --revision with --update-log')
    _DownloadPrebuilts()
  else:
    if not options.revision:
      parser.error('Must supply a --revision')
    if platform == 'linux64':
      _ArchivePrebuilts(commit_position)
    _WaitForLatestSnapshot(commit_position)

  _AddToolsToPath(platform)

  cmd = [
      sys.executable,
      os.path.join(_THIS_DIR, 'test', 'run_all_tests.py'),
  ]
  if platform == 'android':
    cmd.append('--android-packages=' + options.android_packages)

  passed = (util.RunCommand(cmd) == 0)

  _ArchiveServerLogs()

  if platform == 'android':
    if options.update_log:
      util.MarkBuildStepStart('update test result log')
      _UpdateTestResultsLog(platform, commit_position, passed)
  elif passed:
    _ArchiveGoodBuild(platform, commit_position)
    _MaybeRelease(platform)

  if not passed:
    # Make sure the build is red if there is some uncaught exception during
    # running run_all_tests.py.
    util.MarkBuildStepStart('run_all_tests.py')
    util.MarkBuildStepError()

  # Add a "cleanup" step so that errors from runtest.py or bb_device_steps.py
  # (which invoke this script) are kept in their own build step.
  util.MarkBuildStepStart('cleanup')

  return 0 if passed else 1


if __name__ == '__main__':
  sys.exit(main())
