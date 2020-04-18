# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dashboard.common import namespaced_stored_object


_REPOSITORIES_KEY = 'repositories'
_URLS_TO_NAMES_KEY = 'repository_urls_to_names'


def RepositoryUrl(name):
  """Returns the URL of a repository, given its short name.

  If a repository moved locations or has multiple locations, a repository can
  have multiple URLs. The returned URL should be the current canonical one.

  Args:
    name: The short name of the repository.

  Returns:
    A URL string, not including '.git'.
  """
  repositories = namespaced_stored_object.Get(_REPOSITORIES_KEY)
  # We have the 'repository_url' key in case we want to add more fields later.
  return repositories[name]['repository_url']


def Repository(url, add_if_missing=False):
  """Returns the short repository name, given its URL.

  By default, the short repository name is the last part of the URL.
  E.g. "https://chromium.googlesource.com/v8/v8": "v8"
  In some cases this is ambiguous, so the names can be manually adjusted.
  E.g. "../chromium/src": "chromium" and "../breakpad/breakpad/src": "breakpad"

  If a repository moved locations or has multiple locations, multiple URLs can
  map to the same name. This should only be done if they are exact mirrors and
  have the same git hashes.
  "https://webrtc.googlesource.com/src": "webrtc"
  "https://webrtc.googlesource.com/src/webrtc": "old_webrtc"
  "https://chromium.googlesource.com/external/webrtc/trunk/webrtc": "old_webrtc"

  Internally, all repositories are stored by short name, which always maps to
  the current canonical URL, so old URLs are automatically "upconverted".

  Args:
    url: The repository URL.
    add_if_missing: If True, also attempts to add the URL to the database with
      the default name mapping. Throws an exception if there's a name collision.

  Returns:
    The short name as a string.

  Raises:
    AssertionError: add_if_missing is True and there's a name collision.
  """
  if url.endswith('.git'):
    url = url[:-4]

  urls_to_names = namespaced_stored_object.Get(_URLS_TO_NAMES_KEY)
  try:
    return urls_to_names[url]
  except KeyError:
    if add_if_missing:
      return _AddRepository(url)
    raise


def _AddRepository(url):
  """Add a repository URL to the database with the default name mapping.

  The default short repository name is the last part of the URL.

  Returns:
    The short repository name.

  Raises:
    AssertionError: The default name is already in the database.
  """
  name = url.split('/')[-1]

  # Add to main repositories dict.
  repositories = namespaced_stored_object.Get(_REPOSITORIES_KEY)
  if name in repositories:
    raise AssertionError("Attempted to add a repository that's already in the "
                         'Datastore: %s: %s' % (name, url))
  repositories[name] = {'repository_url': url}
  namespaced_stored_object.Set(_REPOSITORIES_KEY, repositories)

  # Add to URL -> name mapping dict.
  urls_to_names = namespaced_stored_object.Get(_URLS_TO_NAMES_KEY)
  urls_to_names[url] = name
  namespaced_stored_object.Set(_URLS_TO_NAMES_KEY, urls_to_names)

  return name
