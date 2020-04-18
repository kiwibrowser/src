# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from extensions_paths import PUBLIC_TEMPLATES
from instance_servlet import (
    InstanceServlet, InstanceServletRenderServletDelegate)
from link_error_detector import LinkErrorDetector, StringifyBrokenLinks
from servlet import Request, Response, Servlet


class BrokenLinkTester(object):
  '''Run link error detector tests.
  '''
  def __init__(self, server_instance, renderer):
    self.link_error_detector = LinkErrorDetector(
        server_instance.host_file_system_provider.GetMaster(),
        renderer,
        PUBLIC_TEMPLATES,
        root_pages=('extensions/index.html', 'apps/about_apps.html'))

  def TestBrokenLinks(self):
    broken_links = self.link_error_detector.GetBrokenLinks()
    return (
        len(broken_links),
        'Warning: Found %d broken links:\n%s' % (
            len(broken_links), StringifyBrokenLinks(broken_links)))

  def TestOrphanedPages(self):
    orphaned_pages = self.link_error_detector.GetOrphanedPages()
    return (
        len(orphaned_pages),
        'Warning: Found %d orphaned pages:\n%s' % (
            len(orphaned_pages), '\n'.join(orphaned_pages)))


class TestServlet(Servlet):
  '''Runs tests against the live server. Supports running all broken link
  detection tests, in parts or all at once.
  '''
  def __init__(self, request, delegate_for_test=None):
    Servlet.__init__(self, request)
    self._delegate = delegate_for_test or InstanceServlet.Delegate()

  def Get(self):
    link_error_tests = ('broken_links', 'orphaned_pages', 'link_errors')

    if not self._request.path in link_error_tests:
      return Response.NotFound('Test %s not found. Available tests are: %s' % (
          self._request.path, ','.join(link_error_tests)))

    constructor = InstanceServlet.GetConstructor(self._delegate)
    def renderer(path):
      return constructor(Request(path, '', self._request.headers)).Get()

    link_tester = BrokenLinkTester(
        InstanceServletRenderServletDelegate(
            self._delegate).CreateServerInstance(),
        renderer)
    if self._request.path == 'broken_links':
      errors, content = link_tester.TestBrokenLinks()
    elif self._request.path == 'orphaned_pages':
      errors, content = link_tester.TestOrphanedPages()
    else:
      link_errors, link_content = link_tester.TestBrokenLinks()
      orphaned_errors, orphaned_content = link_tester.TestOrphanedPages()
      errors = link_errors + orphaned_errors
      content = "%s\n%s" % (link_content, orphaned_content)

    if errors:
      return Response.InternalError(content=content)

    return Response.Ok(content="%s test passed." % self._request.path)
