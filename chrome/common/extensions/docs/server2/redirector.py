# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath
from urlparse import urlsplit

from file_system import FileNotFoundError
from future import All
from path_util import Segment, Join, SplitParent

class Redirector(object):
  def __init__(self, compiled_fs_factory, file_system):
    self._file_system = file_system
    self._cache = compiled_fs_factory.ForJson(file_system)

  def Redirect(self, host, path):
    ''' Check if a path should be redirected, first according to host
    redirection rules, then from rules in redirects.json files.

    Returns the path that should be redirected to, or None if no redirection
    should occur.
    '''
    return self._RedirectOldHosts(host, path) or self._RedirectFromConfig(path)

  def _RedirectFromConfig(self, url):
    ''' Look up redirects.json file in the directory hierarchy of |url|.
    Directory-level redirects occur first, followed by the specific file
    redirects. Returns the URL to the redirect, if any exist, or None.
    '''
    dirname, filename = posixpath.split(url)
    redirected_dirname = self._RedirectDirectory(dirname)

    # Set up default return value.
    default_redirect = None
    if redirected_dirname != dirname:
      default_redirect = posixpath.normpath(Join(redirected_dirname, filename))

    try:
      rules = self._cache.GetFromFile(
        posixpath.normpath(Join(redirected_dirname,
                                          'redirects.json'))).Get()
    except FileNotFoundError:
      return default_redirect

    redirect = rules.get(filename)
    if redirect is None:
      return default_redirect
    if (redirect.startswith('/') or
        urlsplit(redirect).scheme in ('http', 'https')):
      return redirect

    return posixpath.normpath(Join(redirected_dirname, redirect))

  def _RedirectDirectory(self, real_url):
    ''' Returns the final redirected directory after all directory hops.
    If there is a circular redirection, it skips the redirection that would
    cause the infinite loop.
    If no redirection rule is matched, the base directory is returned.
    '''
    seen_redirects = set()

    def lookup_redirect(url):
      sub_url = url

      for sub_url, _ in Segment(url):
        for base, filename in Segment(sub_url):
          try:
            redirects = self._cache.GetFromFile(posixpath.normpath(
                posixpath.join(base, 'redirects.json'))).Get()
          except FileNotFoundError:
            continue

          redirect = redirects.get(posixpath.join(filename, '...'))

          if redirect is None:
            continue

          redirect = Join(base, redirect.rstrip('...'))

          # Avoid infinite redirection loops by breaking if seen before.
          if redirect in seen_redirects:
            break
          seen_redirects.add(redirect)
          return lookup_redirect(
              Join(redirect, posixpath.relpath(url, sub_url)))
      return url

    return lookup_redirect(real_url)

  def _RedirectOldHosts(self, host, path):
    ''' Redirect paths from the old code.google.com to the new
    developer.chrome.com, retaining elements like the channel and https, if
    used.
    '''
    if host != 'code.google.com':
      return None

    path = path.split('/')
    if path and path[0] == 'chrome':
      path.pop(0)

    return 'https://developer.chrome.com/' + posixpath.join(*path)

  def Refresh(self):
    ''' Load files during a cron run.
    '''
    futures = []
    for root, dirs, files in self._file_system.Walk(''):
      if 'redirects.json' in files:
        futures.append(self._cache.GetFromFile(Join(root, 'redirects.json')))
    return All(futures)
