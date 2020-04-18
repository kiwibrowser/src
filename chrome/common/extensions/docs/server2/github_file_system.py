# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
from StringIO import StringIO
import posixpath

from appengine_blobstore import AppEngineBlobstore, BLOBSTORE_GITHUB
from appengine_url_fetcher import AppEngineUrlFetcher
from appengine_wrappers import urlfetch, blobstore
from docs_server_utils import StringIdentity
from file_system import FileSystem, StatInfo
from future import Future
from path_util import IsDirectory
import url_constants
from zipfile import ZipFile, BadZipfile

ZIP_KEY = 'zipball'
USERNAME = None
PASSWORD = None


def _MakeBlobstoreKey(version):
  return ZIP_KEY + '.' + str(version)


def _GetAsyncFetchCallback(fetcher,
                           username,
                           password,
                           blobstore,
                           key_to_set,
                           key_to_delete=None):
  fetch = fetcher.FetchAsync(ZIP_KEY, username=username, password=password)

  def resolve():
    try:
      result = fetch.Get()
      # Check if Github authentication failed.
      if result.status_code == 401:
        logging.error('Github authentication failed for %s, falling back to '
                      'unauthenticated.' % USERNAME)
        blob = fetcher.Fetch(ZIP_KEY).content
      else:
        blob = result.content
    except urlfetch.DownloadError as e:
      logging.error('Bad github zip file: %s' % e)
      return None
    if key_to_delete is not None:
      blobstore.Delete(_MakeBlobstoreKey(key_to_delete, BLOBSTORE_GITHUB))
    try:
      return_zip = ZipFile(StringIO(blob))
    except BadZipfile as e:
      logging.error('Bad github zip file: %s' % e)
      return None

    blobstore.Set(_MakeBlobstoreKey(key_to_set), blob, BLOBSTORE_GITHUB)
    return return_zip

  return resolve


class GithubFileSystem(FileSystem):
  @staticmethod
  def CreateChromeAppsSamples(object_store_creator):
    return GithubFileSystem(
        '%s/GoogleChrome/chrome-app-samples' % url_constants.GITHUB_REPOS,
        AppEngineBlobstore(),
        object_store_creator)

  def __init__(self, url, blobstore, object_store_creator):
    # If we key the password store on the app version then the whole advantage
    # of having it in the first place is greatly lessened (likewise it should
    # always start populated).
    password_store = object_store_creator.Create(
        GithubFileSystem,
        app_version=None,
        category='password',
        start_empty=False)
    if USERNAME is None:
      password_data = password_store.GetMulti(('username', 'password')).Get()
      self._username, self._password = (password_data.get('username'),
                                        password_data.get('password'))
    else:
      password_store.SetMulti({'username': USERNAME, 'password': PASSWORD})
      self._username, self._password = (USERNAME, PASSWORD)

    self._url = url
    self._fetcher = AppEngineUrlFetcher(url)
    self._blobstore = blobstore
    self._stat_object_store = object_store_creator.Create(GithubFileSystem)
    self._version = None
    self._GetZip(self.Stat(ZIP_KEY).version)

  def _GetZip(self, version):
    try:
      blob = self._blobstore.Get(_MakeBlobstoreKey(version), BLOBSTORE_GITHUB)
    except blobstore.BlobNotFoundError:
      self._zip_file = Future(value=None)
      return
    if blob is not None:
      try:
        self._zip_file = Future(value=ZipFile(StringIO(blob)))
      except BadZipfile as e:
        self._blobstore.Delete(_MakeBlobstoreKey(version), BLOBSTORE_GITHUB)
        logging.error('Bad github zip file: %s' % e)
        self._zip_file = Future(value=None)
    else:
      self._zip_file = Future(
          callback=_GetAsyncFetchCallback(self._fetcher,
                                          self._username,
                                          self._password,
                                          self._blobstore,
                                          version,
                                          key_to_delete=self._version))
    self._version = version

  def _ReadFile(self, path):
    try:
      zip_file = self._zip_file.Get()
    except Exception as e:
      logging.error('Github ReadFile error: %s' % e)
      return ''
    if zip_file is None:
      logging.error('Bad github zip file.')
      return ''
    prefix = zip_file.namelist()[0]
    return zip_file.read(prefix + path)

  def _ListDir(self, path):
    try:
      zip_file = self._zip_file.Get()
    except Exception as e:
      logging.error('Github ListDir error: %s' % e)
      return []
    if zip_file is None:
      logging.error('Bad github zip file.')
      return []
    filenames = zip_file.namelist()
    # Take out parent directory name (GoogleChrome-chrome-app-samples-c78a30f)
    filenames = [f[len(filenames[0]):] for f in filenames]
    # Remove the path of the directory we're listing from the filenames.
    filenames = [f[len(path):] for f in filenames
                 if f != path and f.startswith(path)]
    # Remove all files not directly in this directory.
    return [f for f in filenames if f[:-1].count('/') == 0]

  def Read(self, paths, skip_not_found=False):
    version = self.Stat(ZIP_KEY).version
    if version != self._version:
      self._GetZip(version)
    result = {}
    for path in paths:
      if IsDirectory(path):
        result[path] = self._ListDir(path)
      else:
        result[path] = self._ReadFile(path)
    return Future(value=result)

  def _DefaultStat(self, path):
    version = 0
    # TODO(kalman): we should replace all of this by wrapping the
    # GithubFileSystem in a CachingFileSystem. A lot of work has been put into
    # CFS to be robust, and GFS is missing out.
    # For example: the following line is wrong, but it could be moot.
    self._stat_object_store.Set(path, version)
    return StatInfo(version)

  def Stat(self, path):
    version = self._stat_object_store.Get(path).Get()
    if version is not None:
      return StatInfo(version)
    try:
      result = self._fetcher.Fetch('commits/HEAD',
                                   username=USERNAME,
                                   password=PASSWORD)
    except urlfetch.DownloadError as e:
      logging.warning('GithubFileSystem Stat: %s' % e)
      return self._DefaultStat(path)

    # Check if Github authentication failed.
    if result.status_code == 401:
      logging.warning('Github authentication failed for %s, falling back to '
                      'unauthenticated.' % USERNAME)
      try:
        result = self._fetcher.Fetch('commits/HEAD')
      except urlfetch.DownloadError as e:
        logging.warning('GithubFileSystem Stat: %s' % e)
        return self._DefaultStat(path)

    # Parse response JSON - but sometimes github gives us invalid JSON.
    try:
      version = json.loads(result.content)['sha']
      self._stat_object_store.Set(path, version)
      return StatInfo(version)
    except StandardError as e:
      logging.warning(
          ('%s: got invalid or unexpected JSON from github. Response status ' +
           'was %s, content %s') % (e, result.status_code, result.content))
      return self._DefaultStat(path)

  def GetIdentity(self):
    return '%s@%s' % (self.__class__.__name__, StringIdentity(self._url))
