# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
from cStringIO import StringIO
import posixpath
import traceback
from zipfile import ZipFile

import appengine_blobstore as blobstore
from appengine_url_fetcher import AppEngineUrlFetcher
from appengine_wrappers import urlfetch
from docs_server_utils import StringIdentity
from file_system import FileNotFoundError, FileSystem, FileSystemError, StatInfo
from future import Future
from object_store_creator import ObjectStoreCreator
from path_util import AssertIsDirectory, IsDirectory
import url_constants


_GITHUB_REPOS_NAMESPACE = 'GithubRepos'


def _LoadCredentials(object_store_creator):
  '''Returns (username, password) from |password_store|.
  '''
  password_store = object_store_creator.Create(
      GithubFileSystem,
      app_version=None,
      category='password',
      start_empty=False)
  password_data = password_store.GetMulti(('username', 'password')).Get()
  return password_data.get('username'), password_data.get('password')


class _GithubZipFile(object):
  '''A view of a ZipFile with a more convenient interface which ignores the
  'zipball' prefix that all paths have. The zip files that come straight from
  GitHub have paths like ['zipball/foo.txt', 'zipball/bar.txt'] but we only
  care about ['foo.txt', 'bar.txt'].
  '''

  @classmethod
  def Create(cls, repo_name, blob):
    try:
      zipball = ZipFile(StringIO(blob))
    except:
      logging.warning('zipball "%s" is not a valid zip' % repo_name)
      return None

    if not zipball.namelist():
      logging.warning('zipball "%s" is empty' % repo_name)
      return None

    name_prefix = None  # probably 'zipball'
    paths = []
    for name in zipball.namelist():
      prefix, path = name.split('/', 1)
      if name_prefix and prefix != name_prefix:
        logging.warning('zipball "%s" has names with inconsistent prefix: %s' %
                        (repo_name, zipball.namelist()))
        return None
      name_prefix = prefix
      paths.append(path)
    return cls(zipball, name_prefix, paths)

  def __init__(self, zipball, name_prefix, paths):
    self._zipball = zipball
    self._name_prefix = name_prefix
    self._paths = paths

  def Paths(self):
    '''Return all file paths in this zip file.
    '''
    return self._paths

  def List(self, path):
    '''Returns all files within a directory at |path|. Not recursive. Paths
    are returned relative to |path|.
    '''
    AssertIsDirectory(path)
    return [p[len(path):] for p in self._paths
            if p != path and
               p.startswith(path) and
               '/' not in p[len(path):].rstrip('/')]

  def Read(self, path):
    '''Returns the contents of |path|. Raises a KeyError if it doesn't exist.
    '''
    return self._zipball.read(posixpath.join(self._name_prefix, path))


class GithubFileSystem(FileSystem):
  '''Allows reading from a github.com repository.
  '''
  @staticmethod
  def Create(owner, repo, object_store_creator):
    '''Creates a GithubFileSystem that corresponds to a single github repository
    specified by |owner| and |repo|.
    '''
    return GithubFileSystem(
        url_constants.GITHUB_REPOS,
        owner,
        repo,
        object_store_creator,
        AppEngineUrlFetcher)

  @staticmethod
  def ForTest(repo, fake_fetcher, path=None, object_store_creator=None):
    '''Creates a GithubFileSystem that can be used for testing. It reads zip
    files and commit data from server2/test_data/github_file_system/test_owner
    instead of github.com. It reads from files specified by |repo|.
    '''
    return GithubFileSystem(
        path if path is not None else 'test_data/github_file_system',
        'test_owner',
        repo,
        object_store_creator or ObjectStoreCreator.ForTest(),
        fake_fetcher)

  def __init__(self, base_url, owner, repo, object_store_creator, Fetcher):
    self._repo_key = posixpath.join(owner, repo)
    self._repo_url = posixpath.join(base_url, owner, repo)
    self._username, self._password = _LoadCredentials(object_store_creator)
    self._blobstore = blobstore.AppEngineBlobstore()
    self._fetcher = Fetcher(self._repo_url)
    # Stores whether the github is up to date. This will either be True or
    # empty, the emptiness most likely due to this being a cron run.
    self._up_to_date_cache = object_store_creator.Create(
        GithubFileSystem, category='up-to-date')
    # Caches the zip file's stat. Overrides start_empty=False and use
    # |self._up_to_date_cache| to determine whether we need to refresh.
    self._stat_cache = object_store_creator.Create(
        GithubFileSystem, category='stat-cache', start_empty=False)

    # Created lazily in |_EnsureRepoZip|.
    self._repo_zip = None

  def _EnsureRepoZip(self):
    '''Initializes |self._repo_zip| if it hasn't already been (i.e. if
    _EnsureRepoZip has never been called before). In that case |self._repo_zip|
    will be set to a Future of _GithubZipFile and the fetch process started,
    whether that be from a blobstore or if necessary all the way from GitHub.
    '''
    if self._repo_zip is not None:
      return

    repo_key, repo_url, username, password = (
        self._repo_key, self._repo_url, self._username, self._password)

    def fetch_from_blobstore(version):
      '''Returns a Future which resolves to the _GithubZipFile for this repo
      fetched from blobstore.
      '''
      blob = None
      try:
        blob = self._blobstore.Get(repo_url, _GITHUB_REPOS_NAMESPACE)
      except blobstore.BlobNotFoundError:
        pass

      if blob is None:
        logging.warning('No blob for %s found in datastore' % repo_key)
        return fetch_from_github(version)

      repo_zip = _GithubZipFile.Create(repo_key, blob)
      if repo_zip is None:
        logging.warning('Blob for %s was corrupted in blobstore!?' % repo_key)
        return fetch_from_github(version)

      return Future(value=repo_zip)

    def fetch_from_github(version):
      '''Returns a Future which resolves to the _GithubZipFile for this repo
      fetched new from GitHub, then writes it to blobstore and |version| to the
      stat caches.
      '''
      def get_zip(github_zip):
        try:
          blob = github_zip.content
        except urlfetch.DownloadError:
          raise FileSystemError('Failed to download repo %s file from %s' %
                                (repo_key, repo_url))

        repo_zip = _GithubZipFile.Create(repo_key, blob)
        if repo_zip is None:
          raise FileSystemError('Blob for %s was fetched corrupted from %s' %
                                (repo_key, repo_url))

        self._blobstore.Set(self._repo_url, blob, _GITHUB_REPOS_NAMESPACE)
        self._up_to_date_cache.Set(repo_key, True)
        self._stat_cache.Set(repo_key, version)
        return repo_zip
      return self._fetcher.FetchAsync(
          'zipball', username=username, password=password).Then(get_zip)

    # To decide whether we need to re-stat, and from there whether to re-fetch,
    # make use of ObjectStore's start-empty configuration. If
    # |object_store_creator| is configured to start empty then our creator
    # wants to refresh (e.g. running a cron), so fetch the live stat from
    # GitHub. If the stat hasn't changed since last time then no reason to
    # re-fetch from GitHub, just take from blobstore.

    cached_version = self._stat_cache.Get(repo_key).Get()
    if self._up_to_date_cache.Get(repo_key).Get() is None:
      # This is either a cron or an instance where a cron has never been run.
      live_version = self._FetchLiveVersion(username, password)
      if cached_version != live_version:
        # Note: branch intentionally triggered if |cached_version| is None.
        logging.info('%s has changed, fetching from GitHub.' % repo_url)
        self._repo_zip = fetch_from_github(live_version)
      else:
        # Already up to date. Fetch from blobstore. No need to set up-to-date
        # to True here since it'll already be set for instances, and it'll
        # never be set for crons.
        logging.info('%s is up to date.' % repo_url)
        self._repo_zip = fetch_from_blobstore(cached_version)
    else:
      # Instance where cron has been run. It should be in blobstore.
      self._repo_zip = fetch_from_blobstore(cached_version)

    assert self._repo_zip is not None

  def _FetchLiveVersion(self, username, password):
    '''Fetches the current repository version from github.com and returns it.
    The version is a 'sha' hash value.
    '''
    # TODO(kalman): Do this asynchronously (use FetchAsync).
    result = self._fetcher.Fetch(
        'commits/HEAD', username=username, password=password)

    try:
      return json.loads(result.content)['sha']
    except (KeyError, ValueError):
      raise FileSystemError('Error parsing JSON from repo %s: %s' %
                            (self._repo_url, traceback.format_exc()))

  def Refresh(self):
    return self.ReadSingle('')

  def Read(self, paths, skip_not_found=False):
    '''Returns a directory mapping |paths| to the contents of the file at each
    path. If path ends with a '/', it is treated as a directory and is mapped to
    a list of filenames in that directory.
    '''
    self._EnsureRepoZip()
    def read(repo_zip):
      reads = {}
      for path in paths:
        if path not in repo_zip.Paths():
          raise FileNotFoundError('"%s": %s not found' % (self._repo_key, path))
        if IsDirectory(path):
          reads[path] = repo_zip.List(path)
        else:
          reads[path] = repo_zip.Read(path)
      return reads
    return self._repo_zip.Then(read)

  def Stat(self, path):
    '''Stats |path| returning its version as as StatInfo object. If |path| ends
    with a '/', it is assumed to be a directory and the StatInfo object returned
    includes child_versions for all paths in the directory.

    File paths do not include the name of the zip file, which is arbitrary and
    useless to consumers.

    Because the repository will only be downloaded once per server version, all
    stat versions are always 0.
    '''
    self._EnsureRepoZip()
    repo_zip = self._repo_zip.Get()

    if path not in repo_zip.Paths():
      raise FileNotFoundError('"%s" does not contain file "%s"' %
                              (self._repo_key, path))

    version = self._stat_cache.Get(self._repo_key).Get()
    assert version is not None, ('There was a zipball in datastore; there '
                                 'should be a version cached for it')

    stat_info = StatInfo(version)
    if IsDirectory(path):
      stat_info.child_versions = dict((p, StatInfo(version))
                                      for p in repo_zip.List(path))
    return stat_info

  def GetIdentity(self):
    return '%s' % StringIdentity(self.__class__.__name__ + self._repo_key)

  def __repr__(self):
    return '%s(key=%s, url=%s)' % (type(self).__name__,
                                   self._repo_key,
                                   self._repo_url)
