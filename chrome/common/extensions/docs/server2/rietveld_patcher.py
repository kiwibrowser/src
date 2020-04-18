# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import tarfile
from StringIO import StringIO

from file_system import FileNotFoundError
from future import Future
from patcher import Patcher


_CHROMIUM_REPO_BASEURLS = [
  'https://src.chromium.org/svn/trunk/src/',
  'http://src.chromium.org/svn/trunk/src/',
  'svn://svn.chromium.org/chrome/trunk/src',
  'https://chromium.googlesource.com/chromium/src.git@master',
  'http://git.chromium.org/chromium/src.git@master',
]


class RietveldPatcherError(Exception):
  def __init__(self, message):
    self.message = message


class RietveldPatcher(Patcher):
  ''' Class to fetch resources from a patchset in Rietveld.
  '''
  def __init__(self,
               issue,
               fetcher):
    self._issue = issue
    self._fetcher = fetcher
    self._cache = None

  # In RietveldPatcher, the version is the latest patchset number.
  def GetVersion(self):
    try:
      issue_json = json.loads(self._fetcher.Fetch(
          'api/%s' % self._issue).content)
    except Exception as e:
      raise RietveldPatcherError(
          'Failed to fetch information for issue %s.' % self._issue)

    if issue_json.get('closed'):
      raise RietveldPatcherError('Issue %s has been closed.' % self._issue)

    patchsets = issue_json.get('patchsets')
    if not isinstance(patchsets, list) or len(patchsets) == 0:
      raise RietveldPatcherError('Cannot parse issue %s.' % self._issue)

    if not issue_json.get('base_url') in _CHROMIUM_REPO_BASEURLS:
      raise RietveldPatcherError('Issue %s\'s base url (%s) is unknown.' %
          (self._issue, issue_json.get('base_url')))

    return str(patchsets[-1])

  def GetPatchedFiles(self, version=None):
    if version is None:
      patchset = self.GetVersion()
    else:
      patchset = version
    try:
      patchset_json = json.loads(self._fetcher.Fetch(
          'api/%s/%s' % (self._issue, patchset)).content)
    except Exception as e:
      raise RietveldPatcherError(
          'Failed to fetch details for issue %s patchset %s.' % (self._issue,
                                                                 patchset))

    files = patchset_json.get('files')
    if files is None or not isinstance(files, dict):
      raise RietveldPatcherError('Failed to parse issue %s patchset %s.' %
          (self._issue, patchset))

    added = []
    deleted = []
    modified = []
    for f in files:
      status = (files[f].get('status') or 'M')
      # status can be 'A   ' or 'A + '
      if 'A' in status:
        added.append(f)
      elif 'D' in status:
        deleted.append(f)
      elif 'M' in status:
        modified.append(f)
      else:
        raise RietveldPatcherError('Unknown file status for file %s: "%s."' %
                                                                (key, status))

    return (added, deleted, modified)

  def Apply(self, paths, file_system, version=None):
    if version is None:
      version = self.GetVersion()

    def apply_(tarball_result):
      if tarball_result.status_code != 200:
        raise RietveldPatcherError(
            'Failed to download tarball for issue %s patchset %s. Status: %s' %
            (self._issue, version, tarball_result.status_code))

      try:
        tar = tarfile.open(fileobj=StringIO(tarball_result.content))
      except tarfile.TarError as e:
        raise RietveldPatcherError(
            'Error loading tarball for issue %s patchset %s.' % (self._issue,
                                                                 version))

      value = {}
      for path in paths:
        tar_path = 'b/%s' % path

        patched_file = None
        try:
          patched_file = tar.extractfile(tar_path)
          data = patched_file.read()
        except tarfile.TarError as e:
          # Show appropriate error message in the unlikely case that the tarball
          # is corrupted.
          raise RietveldPatcherError(
              'Error extracting tarball for issue %s patchset %s file %s.' %
              (self._issue, version, tar_path))
        except KeyError as e:
          raise FileNotFoundError(
              'File %s not found in the tarball for issue %s patchset %s' %
              (tar_path, self._issue, version))
        finally:
          if patched_file:
            patched_file.close()

        value[path] = data

      return value
    return self._fetcher.FetchAsync('tarball/%s/%s' % (self._issue,
                                                       version)).Then(apply_)

  def GetIdentity(self):
    return self._issue
