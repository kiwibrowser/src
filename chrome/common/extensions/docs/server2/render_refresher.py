# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import posixpath

from custom_logger import CustomLogger
from extensions_paths import EXAMPLES
from file_system_util import CreateURLsFromPaths
from future import All, Future
from render_servlet import RenderServlet
from special_paths import SITE_VERIFICATION_FILE
from timer import Timer


_SUPPORTED_TARGETS = {
  'examples': (EXAMPLES, 'extensions/examples'),
}


_log = CustomLogger('render_refresher')


class _SingletonRenderServletDelegate(RenderServlet.Delegate):
  def __init__(self, server_instance):
    self._server_instance = server_instance

  def CreateServerInstance(self):
    return self._server_instance


def _RequestEachItem(title, items, request_callback):
  '''Runs a task |request_callback| named |title| for each item in |items|.
  |request_callback| must take an item and return a servlet response.
  Returns true if every item was successfully run, false if any return a
  non-200 response or raise an exception.
  '''
  _log.info('%s: starting', title)
  success_count, failure_count = 0, 0
  timer = Timer()
  try:
    for i, item in enumerate(items):
      def error_message(detail):
        return '%s: error rendering %s (%s of %s): %s' % (
            title, item, i + 1, len(items), detail)
      try:
        response = request_callback(item)
        if response.status == 200:
          success_count += 1
        else:
          _log.error(error_message('response status %s' % response.status))
          failure_count += 1
      except Exception as e:
        _log.error(error_message(traceback.format_exc()))
        failure_count += 1
        if IsDeadlineExceededError(e): raise
  finally:
    _log.info('%s: rendered %s of %s with %s failures in %s',
        title, success_count, len(items), failure_count,
        timer.Stop().FormatElapsed())
  return success_count == len(items)


class RenderRefresher(object):
  '''Used to refresh any set of renderable resources. Currently only supports
  assets related to extensions examples.'''
  def __init__(self, server_instance, request):
    self._server_instance = server_instance
    self._request = request

  def Refresh(self):
    def render(path):
      request = Request(path, self._request.host, self._request.headers)
      delegate = _SingletonRenderServletDelegate(self._server_instance)
      return RenderServlet(request, delegate).Get()

    def request_files_in_dir(path, prefix='', strip_ext=None):
      '''Requests every file found under |path| in this host file system, with
      a request prefix of |prefix|. |strip_ext| is an optional list of file
      extensions that should be stripped from paths before requesting.
      '''
      def maybe_strip_ext(name):
        if name == SITE_VERIFICATION_FILE or not strip_ext:
          return name
        base, ext = posixpath.splitext(name)
        return base if ext in strip_ext else name
      files = [maybe_strip_ext(name)
               for name, _ in CreateURLsFromPaths(master_fs, path, prefix)]
      return _RequestEachItem(path, files, render)

    return All(request_files_in_dir(dir, prefix=prefix)
               for dir, prefix in _SUPPORTED_TARGETS.itervalues())

