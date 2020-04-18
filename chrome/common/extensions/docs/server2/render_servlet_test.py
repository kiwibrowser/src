#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from extensions_paths import EXAMPLES, PUBLIC_TEMPLATES, STATIC_DOCS
from local_file_system import LocalFileSystem
from render_servlet import RenderServlet
from server_instance import ServerInstance
from servlet import Request, Response
from test_util import ReadFile


class _RenderServletDelegate(RenderServlet.Delegate):
  def CreateServerInstance(self):
    return ServerInstance.ForTest(LocalFileSystem.Create())


class RenderServletTest(unittest.TestCase):
  def _Render(self, path, headers=None, host=None):
    return RenderServlet(Request.ForTest(path, headers=headers, host=host),
                         _RenderServletDelegate()).Get()

  def testExtensionAppRedirect(self):
    self.assertEqual(
        Response.Redirect('/apps/storage', permanent=False),
        self._Render('storage'))

  def testChannelRedirect(self):
    for channel in ('stable', 'beta', 'dev', 'master'):
      self.assertEqual(
          Response.Redirect('/extensions/storage', permanent=True),
          self._Render('%s/extensions/storage' % channel))

  def testOldHostsRedirect(self):
    self.assertEqual(
        Response.Redirect('https://developer.chrome.com/extensions',
            permanent=False),
        self._Render('/chrome/extensions', host='code.google.com'))
    self.assertEqual(
        Response.Redirect('https://developer.chrome.com/extensions',
            permanent=False),
        self._Render('/chrome/extensions', host='code.google.com'))
    self.assertEqual(
        Response.Redirect('https://developer.chrome.com/devtools/docs/network',
            permanent=False),
        self._Render('/chrome/devtools/docs/network', host='code.google.com'))

  def testNotFound(self):
    def create_404_response(real_path):
      real_404 = self._Render(real_path)
      self.assertEqual(200, real_404.status)
      real_404.status = 404
      return real_404

    root_404 = create_404_response('404')
    extensions_404 = create_404_response('extensions/404')
    apps_404 = create_404_response('apps/404')

    self.assertEqual(root_404, self._Render('not_found'))
    self.assertEqual(root_404, self._Render('not_found/not_found'))

    self.assertEqual(extensions_404, self._Render('extensions/not_found'))
    self.assertEqual(
        extensions_404, self._Render('extensions/manifest/not_found'))
    self.assertEqual(
        extensions_404,
        self._Render('extensions/manifest/not_found/not_found'))

    self.assertEqual(apps_404, self._Render('apps/not_found'))
    self.assertEqual(apps_404, self._Render('apps/manifest/not_found'))
    self.assertEqual(
        apps_404, self._Render('apps/manifest/not_found/not_found'))

  def testSampleFile(self):
    sample_file = 'extensions/talking_alarm_clock/background.js'
    response = self._Render('extensions/examples/%s' % sample_file)
    self.assertEqual(200, response.status)
    self.assertTrue(response.headers['Content-Type'] in (
        'application/javascript; charset=utf-8',
        'application/x-javascript; charset=utf-8'))
    self.assertEqual(ReadFile('%s%s' % (EXAMPLES, sample_file)),
                     response.content.ToString())

  def testSampleZip(self):
    sample_dir = 'extensions/talking_alarm_clock'
    response = self._Render('extensions/examples/%s.zip' % sample_dir)
    self.assertEqual(200, response.status)
    self.assertEqual('application/zip', response.headers['Content-Type'])

  def testStaticFile(self):
    static_file = 'css/out/site.css'
    response = self._Render('static/%s' % static_file)
    self.assertEqual(200, response.status)
    self.assertEqual('text/css; charset=utf-8',
                     response.headers['Content-Type'])
    self.assertEqual(ReadFile('%s%s' % (STATIC_DOCS, static_file)),
                     response.content.ToString())

  def testHtmlTemplate(self):
    html_file = 'extensions/storage'
    response = self._Render(html_file)
    self.assertEqual(200, response.status)
    self.assertEqual('text/html; charset=utf-8',
                     response.headers.get('Content-Type'))
    # Can't really test rendering all that well.
    self.assertTrue(len(response.content) >
                    len(ReadFile('%s%s.html' % (PUBLIC_TEMPLATES, html_file))))

  def testIndexRender(self):
    self.assertEqual(200, self._Render('extensions').status)
    self.assertEqual(('/extensions', False),
                     self._Render('extensions/index').GetRedirect())

  def testOtherRedirectsJsonRedirect(self):
    response = self._Render('apps/webview_tag')
    self.assertEqual(('/apps/tags/webview', False),
                     response.GetRedirect())

  def testDirectories(self):
    # Directories should be redirected to a URL that doesn't end in a '/'
    # whether or not that exists.
    self.assertEqual(('/dir', False), self._Render('dir/').GetRedirect())

  def testEtags(self):
    def test_path(path, content_type):
      # Render without etag.
      response = self._Render(path)
      self.assertEqual(200, response.status)
      etag = response.headers.get('ETag')
      self.assertTrue(etag is not None)

      # Render with an If-None-Match which doesn't match.
      response = self._Render(path, headers={
        'If-None-Match': '"fake etag"',
      })
      self.assertEqual(200, response.status)
      self.assertEqual(content_type, response.headers.get('Content-Type'))
      self.assertEqual(etag, response.headers.get('ETag'))

      # Render with the correct matching If-None-Match.
      response = self._Render(path, headers={
        'If-None-Match': etag,
      })
      self.assertEqual(304, response.status)
      self.assertEqual('Not Modified', response.content.ToString())
      self.assertEqual(content_type, response.headers.get('Content-Type'))
      self.assertEqual(etag, response.headers.get('ETag'))

    # Test with a static path and a dynamic path.
    test_path('static/css/out/site.css', 'text/css; charset=utf-8')
    test_path('extensions/storage', 'text/html; charset=utf-8')


if __name__ == '__main__':
  unittest.main()
