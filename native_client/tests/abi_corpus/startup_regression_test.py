#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import re
import sys
import tempfile

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
import pynacl.file_tools

import corpus_errors
import corpus_utils


def TestAppStartup(options, crx_path, app_path, profile_path):
  """Run the validator on a nexe, check if the result is expected.

  Args:
    options: bag of options.
    crx_path: path to the crx.
    app_path: path to the extracted crx.
    profile_path: path to a temporary profile dir.
  """
  expected_result = corpus_errors.ExpectedCrxResult(crx_path)
  try:
    manifest = corpus_utils.LoadManifest(app_path)
    start_path = manifest.get('app').get('launch').get('local_path')
  except:
    if expected_result.Matches(corpus_errors.BAD_MANIFEST):
      return True
    else:
      print "'%s': %s," % (
          corpus_utils.Sha1FromFilename(crx_path), corpus_errors.BAD_MANIFEST)
      return False
  start_url = 'chrome-extension://%s/%s' % (
      corpus_utils.ChromeAppIdFromPath(app_path), start_path)
  cmd = [options.browser,
      '--disable-web-resources',
      '--disable-features=NetworkPrediction',
      '--no-first-run',
      '--no-default-browser-check',
      '--enable-logging',
      '--log-level=1',
      '--safebrowsing-disable-auto-update',
      '--enable-nacl',
      '--load-extension=' + app_path,
      '--user-data-dir=' + profile_path, start_url]
  process_stdout, process_stderr, retcode = corpus_utils.RunWithTimeout(
      cmd, options.duration)
  # Check for errors we don't like.
  result = corpus_errors.GOOD
  errs = re.findall(':ERROR:[^\n]+', process_stderr)
  for err in errs:
    if 'extension_prefs.cc' in err or 'gles2_cmd_decoder.cc' in err:
      continue
    if 'Extension error: Could not load extension from' in err:
      result = result.Merge(corpus_errors.COULD_NOT_LOAD)
      continue
    if 'NaCl process exited with' in process_stderr:
      result = result.Merge(corpus_errors.MODULE_CRASHED)
      continue
    result = result.Merge(corpus_errors.CrxResult(
      'custom', custom=err, precidence=20))
  if 'NaClMakePcrelThunk:' not in process_stderr:
    result = result.Merge(corpus_errors.MODULE_DIDNT_START)
  # Check if result is what we expect.
  if not expected_result.Matches(result):
    print "'%s': %s," % (corpus_utils.Sha1FromFilename(crx_path), result)
    if options.verbose:
      print '-' * 70
      print 'Return code: %s' % retcode
      print '>>> STDOUT'
      print process_stdout
      print '>>> STDERR'
      print process_stderr
      print '-' * 70
    return False
  return True


def TestApps(options, work_dir):
  """Test a browser on a corpus of crxs.

  Args:
    options: bag of options.
    work_dir: directory to operate in.
  """
  profile_path = os.path.join(work_dir, 'profile_temp')
  app_path = os.path.join(work_dir, 'app_temp')

  list_filename = os.path.join(work_dir, 'naclapps.all')
  filenames = corpus_utils.DownloadCorpusList(list_filename)
  filenames = [f for f in filenames if f.endswith('.crx')]
  progress = corpus_utils.Progress(len(filenames))
  for filename in filenames:
    progress.Tally()
    corpus_utils.PrimeCache(options.cache_dir, filename)
    # Stop here if downloading only.
    if options.download_only:
      continue
    # Stop here if isn't a bad app but testing only bad apps.
    if options.bad_only and corpus_errors.ExpectedCrxResult(filename) == 'GOOD':
      continue
    # Unzip the app.
    corpus_utils.ExtractFromCache(options.cache_dir, filename, app_path)
    try:
      progress.Result(
          TestAppStartup(options, filename, app_path, profile_path))
    finally:
      pynacl.file_tools.RemoveDirectoryIfPresent(app_path)
      pynacl.file_tools.RemoveDirectoryIfPresent(profile_path)
  progress.Summary()


def Main():
  # TODO(bradnelson): Stderr scraping doesn't work on windows, find another
  # way if we want to run there.
  if sys.platform in ['cygwin', 'win32']:
    raise Exception('Platform not yet supported')

  parser = optparse.OptionParser()
  parser.add_option(
      '--download-only', dest='download_only',
      default=False, action='store_true',
      help='download to cache without running the tests')
  parser.add_option(
      '--duration', dest='duration', default=30,
      help='how long to run each app for')
  parser.add_option(
      '--browser', dest='browser',
      help='browser to run')
  parser.add_option(
      '-v', '--verbose', dest='verbose', default=False, action='store_true',
      help='run in verbose mode')
  parser.add_option(
      '--bad-only', dest='bad_only', default=False, action='store_true',
      help='test only known bad apps')
  parser.add_option(
      '--cache-dir', dest='cache_dir',
      default=corpus_utils.DefaultCacheDirectory(),
      help='directory to cache downloads in')
  options, args = parser.parse_args()
  if args:
    parser.error('unused arguments')
  if not options.download_only:
    if not options.browser:
      parser.error('no browser specified')

  work_dir = tempfile.mkdtemp(suffix='startup_crxs', prefix='tmp')
  work_dir = os.path.realpath(work_dir)
  try:
    TestApps(options, work_dir)
  finally:
    pynacl.file_tools.RemoveDirectoryIfPresent(work_dir)


if __name__ == '__main__':
  Main()
