# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib
import logging
import posixpath
import traceback

from branch_utility import BranchUtility
from environment import IsPreviewServer
from file_system import FileNotFoundError
from redirector import Redirector
from servlet import Servlet, Response
from special_paths import SITE_VERIFICATION_FILE
from third_party.motemplate import Motemplate


def _MakeHeaders(content_type, etag=None):
  headers = {
    # See http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.9.1.
    'Cache-Control': 'public, max-age=0, no-cache',
    'Content-Type': content_type,
    'X-Frame-Options': 'sameorigin',
  }
  if etag is not None:
    headers['ETag'] = etag
  return headers


class RenderServlet(Servlet):
  '''Servlet which renders templates.
  '''

  class Delegate(object):
    def CreateServerInstance(self):
      raise NotImplementedError(self.__class__)

  def __init__(self, request, delegate):
    Servlet.__init__(self, request)
    self._delegate = delegate

  def Get(self):
    ''' Render the page for a request.
    '''
    path = self._request.path.lstrip('/')

    # The server used to be partitioned based on Chrome channel, but it isn't
    # anymore. Redirect from the old state.
    channel_name, path = BranchUtility.SplitChannelNameFromPath(path)
    if channel_name is not None:
      return Response.Redirect('/' + path, permanent=True)

    server_instance = self._delegate.CreateServerInstance()

    try:
      return self._GetSuccessResponse(path, server_instance)
    except FileNotFoundError:
      # Find the closest 404.html file and serve that, e.g. if the path is
      # extensions/manifest/typo.html then first look for
      # extensions/manifest/404.html, then extensions/404.html, then 404.html.
      #
      # Failing that just print 'Not Found' but that should preferrably never
      # happen, because it would look really bad.
      path_components = path.split('/')
      for i in xrange(len(path_components) - 1, -1, -1):
        try:
          path_404 = posixpath.join(*(path_components[0:i] + ['404']))
          response = self._GetSuccessResponse(path_404, server_instance)
          if response.status != 200:
            continue
          return Response.NotFound(response.content.ToString(),
                                   headers=response.headers)
        except FileNotFoundError: continue
      logging.warning('No 404.html found in %s' % path)
      return Response.NotFound('Not Found', headers=_MakeHeaders('text/plain'))

  def _GetSuccessResponse(self, request_path, server_instance):
    '''Returns the Response from trying to render |path| with
    |server_instance|.  If |path| isn't found then a FileNotFoundError will be
    raised, such that the only responses that will be returned from this method
    are Ok and Redirect.
    '''
    content_provider, serve_from, path = (
        server_instance.content_providers.GetByServeFrom(request_path))
    assert content_provider, 'No ContentProvider found for %s' % path

    redirect = Redirector(
        server_instance.compiled_fs_factory,
        content_provider.file_system).Redirect(self._request.host, path)
    if redirect is not None:
      # Absolute redirects stay absolute, relative redirects are relative to
      # |serve_from|; all redirects eventually need to be *served* as absolute.
      if not redirect.startswith(('/', 'http://', 'https://')):
        redirect = '/' + posixpath.join(serve_from, redirect)
      return Response.Redirect(redirect, permanent=False)

    canonical_path = content_provider.GetCanonicalPath(path)
    if canonical_path != path:
      redirect_path = posixpath.join(serve_from, canonical_path)
      return Response.Redirect('/' + redirect_path, permanent=False)

    if request_path.endswith('/'):
      # Directory request hasn't been redirected by now. Default behaviour is
      # to redirect as though it were a file.
      return Response.Redirect('/' + request_path.rstrip('/'),
                               permanent=False)

    content_and_type = content_provider.GetContentAndType(path).Get()
    if not content_and_type.content:
      logging.error('%s had empty content' % path)

    content = content_and_type.content
    if isinstance(content, Motemplate):
      template_content, template_warnings = (
          server_instance.template_renderer.Render(content, self._request))
      # HACK: the site verification file (google2ed...) doesn't have a title.
      content, doc_warnings = server_instance.document_renderer.Render(
          template_content,
          path,
          render_title=path != SITE_VERIFICATION_FILE)
      warnings = template_warnings + doc_warnings
      if warnings:
        sep = '\n - '
        logging.warning('Rendering %s:%s%s' % (path, sep, sep.join(warnings)))
      # Content was dynamic. The new etag is a hash of the content.
      etag = None
    elif content_and_type.version is not None:
      # Content was static. The new etag is the version of the content. Hash it
      # to make sure it's valid.
      etag = '"%s"' % hashlib.md5(str(content_and_type.version)).hexdigest()
    else:
      # Sometimes non-dynamic content does not have a version, for example
      # .zip files. The new etag is a hash of the content.
      etag = None

    content_type = content_and_type.content_type
    if isinstance(content, unicode):
      content = content.encode('utf-8')
      content_type += '; charset=utf-8'

    if etag is None:
      # Note: we're using md5 as a convenient and fast-enough way to identify
      # content. It's not intended to be cryptographic in any way, and this
      # is *not* what etags is for. That's what SSL is for, this is unrelated.
      etag = '"%s"' % hashlib.md5(content).hexdigest()

    headers = _MakeHeaders(content_type, etag=etag)
    if etag == self._request.headers.get('If-None-Match'):
      return Response.NotModified('Not Modified', headers=headers)
    return Response.Ok(content, headers=headers)
