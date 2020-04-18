# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from caching_file_system import CachingFileSystem
from empty_dir_file_system import EmptyDirFileSystem
from github_file_system import GithubFileSystem as OldGithubFileSystem
from new_github_file_system import GithubFileSystem as NewGithubFileSystem


class GithubFileSystemProvider(object):
  '''Provides GithubFileSystems bound to an owner/repo pair.
  '''

  def __init__(self, object_store_creator):
    self._object_store_creator = object_store_creator

  def Create(self, owner, repo):
    '''Creates a GithubFileSystem. For legacy reasons this is hacked
    such that the apps samples returns the old GithubFileSystem.

    |owner| is the owner of the GitHub account, e.g. 'GoogleChrome'.
    |repo| is the repository name, e.g. 'devtools-docs'.
    '''
    if owner == 'GoogleChrome' and repo == 'chrome-app-samples':
      # NOTE: The old GitHub file system implementation doesn't support being
      # wrapped by a CachingFileSystem. It's also too slow to run on the dev
      # server, since every app API page would need to read from it.
      return OldGithubFileSystem.CreateChromeAppsSamples(
          self._object_store_creator)
    return CachingFileSystem(
        NewGithubFileSystem.Create(owner, repo, self._object_store_creator),
        self._object_store_creator)

  @staticmethod
  def ForEmpty():
    class EmptyImpl(object):
      def Create(self, owner, repo):
        return EmptyDirFileSystem()
    return EmptyImpl()
