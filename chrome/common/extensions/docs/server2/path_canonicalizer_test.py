#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath
import unittest

from extensions_paths import PUBLIC_TEMPLATES, SERVER2
from local_file_system import LocalFileSystem
from test_file_system import TestFileSystem
from object_store_creator import ObjectStoreCreator
from path_canonicalizer import PathCanonicalizer
from special_paths import SITE_VERIFICATION_FILE


class PathCanonicalizerTest(unittest.TestCase):
  def setUp(self):
    self._path_canonicalizer = PathCanonicalizer(
        LocalFileSystem.Create(PUBLIC_TEMPLATES),
        ObjectStoreCreator.ForTest(),
        ('.html', '.md'))

  def testSpecifyCorrectly(self):
    self._AssertIdentity('extensions/browserAction')
    self._AssertIdentity('extensions/storage')
    self._AssertIdentity('extensions/blah')
    self._AssertIdentity('extensions/index')
    self._AssertIdentity('extensions/whats_new')
    self._AssertIdentity('apps/storage')
    self._AssertIdentity('apps/bluetooth')
    self._AssertIdentity('apps/blah')
    self._AssertIdentity('apps/tags/webview')

  def testSpecifyIncorrectly(self):
    self._AssertRedirectWithDefaultExtensions(
        'extensions/browserAction', 'apps/browserAction')
    self._AssertRedirectWithDefaultExtensions(
        'extensions/browserAction', 'apps/extensions/browserAction')
    self._AssertRedirectWithDefaultExtensions(
        'apps/bluetooth', 'extensions/bluetooth')
    self._AssertRedirectWithDefaultExtensions(
        'apps/bluetooth', 'extensions/apps/bluetooth')
    self._AssertRedirectWithDefaultExtensions(
        'extensions/index', 'apps/index')
    self._AssertRedirectWithDefaultExtensions(
        'extensions/browserAction', 'static/browserAction')
    self._AssertRedirectWithDefaultExtensions(
        'apps/tags/webview', 'apps/webview')
    self._AssertRedirectWithDefaultExtensions(
        'apps/tags/webview', 'extensions/webview')
    self._AssertRedirectWithDefaultExtensions(
        'apps/tags/webview', 'extensions/tags/webview')

    # These are a little trickier because storage.html is in both directories.
    # They must canonicalize to the closest match.
    self._AssertRedirectWithDefaultExtensions(
        'extensions/storage', 'extensions/apps/storage')
    self._AssertRedirectWithDefaultExtensions(
        'apps/storage', 'apps/extensions/storage')

  def testUnspecified(self):
    self._AssertRedirectWithDefaultExtensions(
        'extensions/browserAction', 'browserAction')
    self._AssertRedirectWithDefaultExtensions(
        'apps/bluetooth', 'bluetooth')
    # Default happens to be apps because it's first alphabetically.
    self._AssertRedirectWithDefaultExtensions(
        'apps/storage', 'storage')
    # Nonexistent APIs should be left alone.
    self._AssertIdentity('blah.html')

  def testDirectories(self):
    # Directories can be canonicalized too!
    self._AssertIdentity('apps/')
    self._AssertIdentity('apps/tags/')
    self._AssertIdentity('extensions/')
    # No trailing slash should be treated as files not directories, at least
    # at least according to PathCanonicalizer.
    self._AssertRedirect('extensions/apps', 'apps')
    self._AssertRedirect('extensions', 'extensions')
    # Just as tolerant of spelling mistakes.
    self._AssertRedirect('apps/', 'Apps/')
    self._AssertRedirect('apps/tags/', 'Apps/TAGS/')
    self._AssertRedirect('extensions/', 'Extensions/')
    # Find directories in the correct place.
    self._AssertRedirect('apps/tags/', 'tags/')
    self._AssertRedirect('apps/tags/', 'extensions/tags/')

  def testSpellingErrors(self):
    for spelme in ('browseraction', 'browseraction.htm', 'BrowserAction',
                   'BrowserAction.html', 'browseraction.html', 'Browseraction',
                   'browser-action', 'Browser.action.html', 'browser_action',
                   'browser-action.html', 'Browser_Action.html'):
      self._AssertRedirect('extensions/browserAction', spelme)
      self._AssertRedirect('extensions/browserAction', 'extensions/%s' % spelme)
      self._AssertRedirect('extensions/browserAction', 'apps/%s' % spelme)

  def testNonDefaultExtensions(self):
    # The only example currently of a file with a non-default extension is
    # the redirects.json file. That shouldn't have its extension stripped since
    # it's not in the default extensions.
    self._AssertIdentity('redirects.json')
    self._AssertRedirect('redirects.json', 'redirects')
    self._AssertRedirect('redirects.json', 'redirects.html')
    self._AssertRedirect('redirects.json', 'redirects.js')
    self._AssertRedirect('redirects.json', 'redirects.md')

  def testSiteVerificationFile(self):
    # The site verification file should not redirect.
    self._AssertIdentity(SITE_VERIFICATION_FILE)
    self._AssertRedirect(SITE_VERIFICATION_FILE,
                         posixpath.splitext(SITE_VERIFICATION_FILE)[0])

  def testDotSeparated(self):
    self._AssertIdentity('extensions/devtools_inspectedWindow')
    self._AssertRedirect('extensions/devtools_inspectedWindow',
                         'extensions/devtools.inspectedWindow')

  def testUnderscoreSeparated(self):
    file_system = TestFileSystem({
      'pepper_dev': {
        'c': {
          'index.html': ''
        }
      },
      'pepper_stable': {
        'c': {
          'index.html': ''
        }
      }
    })
    self._path_canonicalizer = PathCanonicalizer(
        file_system,
        ObjectStoreCreator.ForTest(),
        ('.html', '.md'))
    self._AssertIdentity('pepper_stable/c/index')
    self._AssertRedirect('pepper_stable/c/index',
                         'pepper_stable/c/index.html')

  def _AssertIdentity(self, path):
    self._AssertRedirect(path, path)

  def _AssertRedirect(self, to, from_):
    self.assertEqual(to, self._path_canonicalizer.Canonicalize(from_))

  def _AssertRedirectWithDefaultExtensions(self, to, from_):
    for ext in ('', '.html', '.md'):
      self._AssertRedirect(
          to, self._path_canonicalizer.Canonicalize(from_ + ext))


if __name__ == '__main__':
  unittest.main()
