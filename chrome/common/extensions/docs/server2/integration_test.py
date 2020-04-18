#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run build_server so that files needed by tests are copied to the local
# third_party directory.
import build_server
build_server.main()

import json
import optparse
import os
import posixpath
import sys
import time
import unittest
import update_cache

from branch_utility import BranchUtility
from chroot_file_system import ChrootFileSystem
from extensions_paths import (
    CONTENT_PROVIDERS, CHROME_EXTENSIONS, PUBLIC_TEMPLATES)
from fake_fetchers import ConfigureFakeFetchers
from special_paths import SITE_VERIFICATION_FILE
from handler import Handler
from link_error_detector import LinkErrorDetector, StringifyBrokenLinks
from local_file_system import LocalFileSystem
from local_renderer import LocalRenderer
from path_util import AssertIsValid
from servlet import Request
from third_party.json_schema_compiler import json_parse
from test_util import (
    ChromiumPath, DisableLogging, EnableLogging, ReadFile, Server2Path)


# Arguments set up if __main__ specifies them.
_EXPLICIT_TEST_FILES = None
_REBASE = False
_VERBOSE = False


def _ToPosixPath(os_path):
  return os_path.replace(os.sep, '/')


def _FilterHidden(paths):
  '''Returns a list of the non-hidden paths from |paths|.
  '''
  # Hidden files start with a '.' but paths like './foo' and '../foo' are not
  # hidden.
  return [path for path in paths if (not path.startswith('.')) or
                                     path.startswith('./') or
                                     path.startswith('../')]


def _GetPublicFiles():
  '''Gets all public file paths mapped to their contents.
  '''
  def walk(path, prefix=''):
    path = ChromiumPath(path)
    public_files = {}
    for root, dirs, files in os.walk(path, topdown=True):
      relative_root = root[len(path):].lstrip(os.path.sep)
      dirs[:] = _FilterHidden(dirs)
      for filename in _FilterHidden(files):
        with open(os.path.join(root, filename), 'r') as f:
          request_path = posixpath.join(prefix, relative_root, filename)
          public_files[request_path] = f.read()
    return public_files

  # Public file locations are defined in content_providers.json, sort of. Epic
  # hack to pull them out; list all the files from the directories that
  # Chromium content providers ask for.
  public_files = {}
  content_providers = json_parse.Parse(ReadFile(CONTENT_PROVIDERS))
  for content_provider in content_providers.itervalues():
    if 'chromium' in content_provider:
      public_files.update(walk(content_provider['chromium']['dir'],
                               prefix=content_provider['serveFrom']))
  return public_files


class IntegrationTest(unittest.TestCase):
  def setUp(self):
    ConfigureFakeFetchers()

  @EnableLogging('info')
  def testUpdateAndPublicFiles(self):
    '''Runs update then requests every public file. Update needs to be run first
    because the public file requests are offline.
    '''
    if _EXPLICIT_TEST_FILES is not None:
      return

    print('Running update...')
    start_time = time.time()
    try:
      update_cache.UpdateCache()
    finally:
      print('Took %s seconds' % (time.time() - start_time))

    # TODO(kalman): Re-enable this, but it takes about an hour at the moment,
    # presumably because every page now has a lot of links on it from the
    # topnav.

    #print("Checking for broken links...")
    #start_time = time.time()
    #link_error_detector = LinkErrorDetector(
    #    # TODO(kalman): Use of ChrootFileSystem here indicates a hack. Fix.
    #    ChrootFileSystem(LocalFileSystem.Create(), CHROME_EXTENSIONS),
    #    lambda path: Handler(Request.ForTest(path)).Get(),
    #    'templates/public',
    #    ('extensions/index.html', 'apps/about_apps.html'))

    #broken_links = link_error_detector.GetBrokenLinks()
    #if broken_links:
    #  print('Found %d broken links.' % (
    #    len(broken_links)))
    #  if _VERBOSE:
    #    print(StringifyBrokenLinks(broken_links))

    #broken_links_set = set(broken_links)

    #known_broken_links_path = os.path.join(
    #    Server2Path('known_broken_links.json'))
    #try:
    #  with open(known_broken_links_path, 'r') as f:
    #    # The JSON file converts tuples and sets into lists, and for this
    #    # set union/difference logic they need to be converted back.
    #    known_broken_links = set(tuple(item) for item in json.load(f))
    #except IOError:
    #  known_broken_links = set()

    #newly_broken_links = broken_links_set - known_broken_links
    #fixed_links = known_broken_links - broken_links_set

    #print('Took %s seconds.' % (time.time() - start_time))

    #print('Searching for orphaned pages...')
    #start_time = time.time()
    #orphaned_pages = link_error_detector.GetOrphanedPages()
    #if orphaned_pages:
    #  # TODO(jshumway): Test should fail when orphaned pages are detected.
    #  print('Found %d orphaned pages:' % len(orphaned_pages))
    #  for page in orphaned_pages:
    #    print(page)
    #print('Took %s seconds.' % (time.time() - start_time))

    public_files = _GetPublicFiles()

    print('Rendering %s public files...' % len(public_files.keys()))
    start_time = time.time()
    try:
      for path, content in public_files.iteritems():
        AssertIsValid(path)
        if path.endswith('redirects.json'):
          continue

        # The non-example html and md files are served without their file
        # extensions.
        path_without_ext, ext = posixpath.splitext(path)
        if (ext in ('.html', '.md') and
            '/examples/' not in path and
            path != SITE_VERIFICATION_FILE):
          path = path_without_ext

        def check_result(response):
          is_ok = response.status == 200;
          is_redirect = response.status == 302;
          # TODO(dbertoni@chromium.org): Explore following redirects and/or
          # keeping an explicit list of files that expect 200 vs. 302.
          self.assertTrue(is_ok or is_redirect,
                          'Got %s when rendering %s' % (response.status, path))

          self.assertTrue(is_redirect or len(response.content),
              'Rendered content length was 0 when rendering %s' % path)

          # This is reaaaaally rough since usually these will be tiny templates
          # that render large files.
          self.assertTrue(is_redirect or
                          len(response.content) >= len(content) or
                          # Zip files may be served differently than stored
                          # locally.
                          path.endswith('.zip'),
              'Rendered content length was %s vs template content length %s '
              'when rendering %s' % (len(response.content), len(content), path))

        # TODO(kalman): Hack to avoid failing redirects like extensions/index
        # to extensions. Better fix would be to parse or whitelist the
        # redirects.json files as part of this test.
        if not path.endswith('/index'):
          check_result(Handler(Request.ForTest(path)).Get())

        if path.startswith(('apps/', 'extensions/')):
          # Make sure that adding the .html will temporarily redirect to
          # the path without the .html for APIs and articles.
          if '/examples/' not in path:
            redirect_response = Handler(Request.ForTest(path + '.html')).Get()
            self.assertEqual(
                ('/' + path, False), redirect_response.GetRedirect(),
                '%s.html did not (temporarily) redirect to %s (status %s)' %
                    (path, path, redirect_response.status))

          # Make sure including a channel will permanently redirect to the same
          # path without a channel.
          for channel in BranchUtility.GetAllChannelNames():
            redirect_response = Handler(
                Request.ForTest(posixpath.join(channel, path))).Get()
            self.assertEqual(
                ('/' + path, True),
                redirect_response.GetRedirect(),
                '%s/%s did not (permanently) redirect to %s (status %s)' %
                    (channel, path, path, redirect_response.status))

        # Samples are internationalized, test some locales.
        if path.endswith('/samples'):
          for lang in ('en-US', 'es', 'ar'):
            check_result(Handler(Request.ForTest(
                path,
                headers={'Accept-Language': '%s;q=0.8' % lang})).Get())
    finally:
      print('Took %s seconds' % (time.time() - start_time))

    #if _REBASE:
    #  print('Rebasing broken links with %s newly broken and %s fixed links.' %
    #        (len(newly_broken_links), len(fixed_links)))
    #  with open(known_broken_links_path, 'w') as f:
    #    json.dump(broken_links, f,
    #              indent=2, separators=(',', ': '), sort_keys=True)
    #else:
    #  if fixed_links or newly_broken_links:
    #    print('**********************************************\n'
    #          'CHANGE DETECTED IN BROKEN LINKS WITHOUT REBASE\n'
    #          '**********************************************')
    #    print('Found %s broken links, and some have changed. '
    #          'If this is acceptable or expected then run %s with the --rebase '
    #          'option.' % (len(broken_links), os.path.split(__file__)[-1]))
    #  elif broken_links:
    #    print('%s existing broken links' % len(broken_links))
    #  if fixed_links:
    #    print('%s broken links have been fixed:' % len(fixed_links))
    #    print(StringifyBrokenLinks(fixed_links))
    #  if newly_broken_links:
    #    print('There are %s new broken links:' % len(newly_broken_links))
    #    print(StringifyBrokenLinks(newly_broken_links))
    #    self.fail('See logging for details.')

  # TODO(kalman): Move this test elsewhere, it's not an integration test.
  # Perhaps like "presubmit_tests" or something.
  def testExplicitFiles(self):
    '''Tests just the files in _EXPLICIT_TEST_FILES.
    '''
    if _EXPLICIT_TEST_FILES is None:
      return
    for filename in _EXPLICIT_TEST_FILES:
      print('Rendering %s...' % filename)
      start_time = time.time()
      try:
        response = LocalRenderer.Render(_ToPosixPath(filename))
        self.assertTrue(response.status == 200 or response.status == 302,
                        'Got %s when rendering %s' % (response.status,
                                                      _ToPosixPath(filename)))
        self.assertTrue(response.content != '' or response.status == 302)
      finally:
        print('Took %s seconds' % (time.time() - start_time))

    # TODO(jshumway): Check page for broken links (currently prohibited by the
    # time it takes to render the pages).

  @DisableLogging('warning')
  def testFileNotFound(self):
    response = LocalRenderer.Render('/extensions/notfound')
    self.assertEqual(404, response.status)

  def testSiteVerificationFile(self):
    response = LocalRenderer.Render('/' + SITE_VERIFICATION_FILE)
    self.assertEqual(200, response.status)

if __name__ == '__main__':
  parser = optparse.OptionParser()
  parser.add_option('-a', '--all', action='store_true', default=False,
                    help='Render all pages, not just the one specified')
  parser.add_option('-r', '--rebase', action='store_true', default=False,
                    help='Rewrites the known_broken_links.json file with '
                         'the current set of broken links')
  parser.add_option('-v', '--verbose', action='store_true', default=False,
                    help='Show verbose output like currently broken links')
  (opts, args) = parser.parse_args()
  if not opts.all:
    _EXPLICIT_TEST_FILES = args
  _REBASE = opts.rebase
  _VERBOSE = opts.verbose
  # Kill sys.argv because we have our own flags.
  sys.argv = [sys.argv[0]]
  unittest.main()
