# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from caching_file_system import CachingFileSystem
from local_file_system import LocalFileSystem
from local_git_file_system import LocalGitFileSystem
from offline_file_system import OfflineFileSystem
from third_party.json_schema_compiler.memoize import memoize


class HostFileSystemProvider(object):
  '''Provides host file systems ("host" meaning the file system that hosts the
  server's source code and templates) tracking master, or any branch.

  File system instances are memoized to maintain the in-memory caches across
  multiple callers.
  '''
  def __init__(self,
               object_store_creator,
               pinned_commit=None,
               default_master_instance=None,
               offline=False,
               constructor_for_test=None,
               cache_only=False):
    '''
    |object_store_creator|
      Provides caches for file systems that need one.
    |pinned_commit|
      If not None, the commit at which a 'master' file system will be created.
      If None, 'master' file systems will use HEAD.
    |default_master_instance|
      If not None, 'master' file systems provided by this class without a
      specific commit will return |default_master_instance| instead.
    |offline|
      If True all provided file systems will be wrapped in an OfflineFileSystem.
    |constructor_for_test|
      Provides a custom constructor rather than creating LocalGitFileSystems.
    |cache_only|
      If True, all provided file systems will be cache-only, meaning that cache
      misses will result in errors rather than cache updates.
    '''
    self._object_store_creator = object_store_creator
    self._pinned_commit = pinned_commit
    self._default_master_instance = default_master_instance
    self._offline = offline
    self._constructor_for_test = constructor_for_test
    self._cache_only = cache_only

  @memoize
  def GetMaster(self, commit=None):
    '''Gets a file system tracking 'master'. Use this method rather than
    GetBranch('master') because the behaviour is subtly different; 'master' can
    be pinned to a specific commit (|pinned_commit| in constructor) and can have
    have its default instance overridden (|default_master_instance| in the
    constructor).

    |commit| if non-None determines a specific commit to pin the host file
    system at, though it will be ignored if it's newer than |pinned_commit|.
    If None then |commit| will track |pinned_commit| if is has been
    set, or just HEAD (which might change during server runtime!).
    '''
    if commit is None:
      if self._default_master_instance is not None:
        return self._default_master_instance
      return self._Create('master', commit=self._pinned_commit)
    return self._Create('master', commit=commit)

  @memoize
  def GetBranch(self, branch):
    '''Gets a file system tracking |branch|, for example '1150' - anything other
    than 'master', which must be constructed via the GetMaster() method.

    Note: Unlike GetMaster this function doesn't take a |commit| argument
    since we assume that branches hardly ever change, while master frequently
    changes.
    '''
    assert isinstance(branch, basestring), 'Branch %s must be a string' % branch
    assert branch != 'master', (
        'Cannot specify branch=\'master\', use GetMaster()')
    return self._Create(branch)

  def _Create(self, branch, commit=None):
    '''Creates local git file systems (or if in a test, potentially whatever
    |self._constructor_for_test specifies). Wraps the resulting file system in
    an Offline file system if the offline flag is set, and finally wraps it in
    a Caching file system.
    '''
    if self._constructor_for_test is not None:
      file_system = self._constructor_for_test(branch=branch, commit=commit)
    else:
      file_system = LocalGitFileSystem.Create(branch=branch, commit=commit)
    if self._offline:
      file_system = OfflineFileSystem(file_system)
    return CachingFileSystem(file_system, self._object_store_creator,
        fail_on_miss=self._cache_only)

  @staticmethod
  def ForLocal(object_store_creator, **optargs):
    '''Used in creating a server instance on localhost.
    '''
    return HostFileSystemProvider(
        object_store_creator,
        constructor_for_test=lambda **_: LocalFileSystem.Create(),
        **optargs)

  @staticmethod
  def ForTest(file_system, object_store_creator, **optargs):
    '''Used in creating a test server instance. The HostFileSystemProvider
    returned here will always return |file_system| when its Create() method is
    called.
    '''
    return HostFileSystemProvider(
        object_store_creator,
        constructor_for_test=lambda **_: file_system,
        **optargs)
