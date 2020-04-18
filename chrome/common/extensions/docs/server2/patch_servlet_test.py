#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from HTMLParser import HTMLParser
import unittest

from fake_fetchers import ConfigureFakeFetchers
from host_file_system_provider import HostFileSystemProvider
from patch_servlet import PatchServlet
from render_servlet import RenderServlet
from server_instance import ServerInstance
from servlet import Request
from test_branch_utility import TestBranchUtility
from test_util import DisableLogging



_ALLOWED_HOST = 'chrome-apps-doc.appspot.com'


def _CheckURLsArePatched(content, patch_servlet_path):
  errors = []
  class LinkChecker(HTMLParser):
    def handle_starttag(self, tag, attrs):
      if tag != 'a':
        return
      tag_description = '<a %s .../>' % ' '.join('%s="%s"' % (key, val)
                                                 for key, val in attrs)
      attrs = dict(attrs)
      if ('href' in attrs and
           attrs['href'].startswith('/') and
           not attrs['href'].startswith('/%s/' % patch_servlet_path)):
        errors.append('%s has an unqualified href' % tag_description)
  LinkChecker().feed(content)
  return errors


class _RenderServletDelegate(RenderServlet.Delegate):
  def CreateServerInstance(self):
    return ServerInstance.ForLocal()

class _PatchServletDelegate(RenderServlet.Delegate):
  def CreateBranchUtility(self, object_store_creator):
    return TestBranchUtility.CreateWithCannedData()

  def CreateHostFileSystemProvider(self, object_store_creator, **optargs):
    return HostFileSystemProvider.ForLocal(object_store_creator, **optargs)

  def CreateGithubFileSystemProvider(self, object_store_creator):
    return GithubFileSystemProvider.ForEmpty()


class PatchServletTest(unittest.TestCase):
  def setUp(self):
    ConfigureFakeFetchers()

  def _RenderWithPatch(self, path, issue):
    path_with_issue = '%s/%s' % (issue, path)
    return PatchServlet(Request.ForTest(path_with_issue, host=_ALLOWED_HOST),
                        _PatchServletDelegate()).Get()

  def _RenderWithoutPatch(self, path):
    return RenderServlet(Request.ForTest(path, host=_ALLOWED_HOST),
                         _RenderServletDelegate()).Get()

  def _RenderAndCheck(self, path, issue, expected_equal):
    '''Renders |path| with |issue| patched in and asserts that the result is
    the same as |expected_equal| modulo any links that get rewritten to
    "_patch/issue".
    '''
    patched_response = self._RenderWithPatch(path, issue)
    unpatched_response = self._RenderWithoutPatch(path)
    for header in ('Cache-Control', 'ETag'):
      patched_response.headers.pop(header, None)
      unpatched_response.headers.pop(header, None)
    unpatched_content = unpatched_response.content.ToString()

    # Check that all links in the patched content are qualified with
    # the patch URL, then strip them out for checking (in)equality.
    patched_content = patched_response.content.ToString()
    patch_servlet_path = '_patch/%s' % issue
    errors = _CheckURLsArePatched(patched_content, patch_servlet_path)
    self.assertFalse(errors,
        '%s\nFound errors:\n * %s' % (patched_content, '\n * '.join(errors)))
    patched_content = patched_content.replace('/%s' % patch_servlet_path, '')

    self.assertEqual(patched_response.status, unpatched_response.status)
    self.assertEqual(patched_response.headers, unpatched_response.headers)
    if expected_equal:
      self.assertEqual(patched_content, unpatched_content)
    else:
      self.assertNotEqual(patched_content, unpatched_content)

  def _RenderAndAssertEqual(self, path, issue):
    self._RenderAndCheck(path, issue, True)

  def _RenderAndAssertNotEqual(self, path, issue):
    self._RenderAndCheck(path, issue, False)

  @DisableLogging('warning')
  def _AssertNotFound(self, path, issue):
    response = self._RenderWithPatch(path, issue)
    self.assertEqual(response.status, 404,
        'Path %s with issue %s should have been removed for %s.' % (
            path, issue, response))

  def _AssertOk(self, path, issue):
    response = self._RenderWithPatch(path, issue)
    self.assertEqual(response.status, 200,
        'Failed to render path %s with issue %s.' % (path, issue))
    self.assertTrue(len(response.content.ToString()) > 0,
        'Rendered result for path %s with issue %s should not be empty.' %
        (path, issue))

  def _AssertRedirect(self, path, issue, redirect_path):
    response = self._RenderWithPatch(path, issue)
    self.assertEqual(302, response.status)
    self.assertEqual('/_patch/%s/%s' % (issue, redirect_path),
                     response.headers['Location'])

  def testRender(self):
    # '_patch' is not included in paths below because it's stripped by Handler.
    issue = '14096030'

    # TODO(kalman): Test with chrome_sidenav.json once the sidenav logic has
    # stabilised.

    # extensions/runtime.html is removed in the patch, should redirect to the
    # apps version.
    self._AssertRedirect('extensions/runtime', issue, 'apps/runtime')

    # apps/runtime.html is not removed.
    self._RenderAndAssertEqual('apps/runtime', issue)

    # test_foo.html is added in the patch.
    self._AssertOk('extensions/test_foo', issue)

    # Invalid issue number results in a 404.
    self._AssertNotFound('extensions/index', '11111')

  def testXssRedirect(self):
    def is_redirect(from_host, from_path, to_url):
      response = PatchServlet(Request.ForTest(from_path, host=from_host),
                              _PatchServletDelegate()).Get()
      redirect_url, _ = response.GetRedirect()
      if redirect_url is None:
        return (False, '%s/%s did not cause a redirect' % (
            from_host, from_path))
      if redirect_url != to_url:
        return (False, '%s/%s redirected to %s not %s' % (
            from_host, from_path, redirect_url, to_url))
      return (True, '%s/%s redirected to %s' % (
          from_host, from_path, redirect_url))
    self.assertTrue(*is_redirect('developer.chrome.com', '12345',
                                 'https://%s/_patch/12345' % _ALLOWED_HOST))
    self.assertTrue(*is_redirect('developers.google.com', '12345',
                                 'https://%s/_patch/12345' % _ALLOWED_HOST))
    self.assertFalse(*is_redirect('chrome-apps-doc.appspot.com', '12345',
                                  None))
    self.assertFalse(*is_redirect('some-other-app.appspot.com', '12345',
                                  None))

if __name__ == '__main__':
  unittest.main()
