# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from branch_utility import BranchUtility
from commit_tracker import CommitTracker
from compiled_file_system import CompiledFileSystem
from environment import IsDevServer, IsReleaseServer
from host_file_system_provider import HostFileSystemProvider
from third_party.json_schema_compiler.memoize import memoize
from render_servlet import RenderServlet
from object_store_creator import ObjectStoreCreator
from server_instance import ServerInstance
from gcs_file_system_provider import CloudStorageFileSystemProvider


class InstanceServletRenderServletDelegate(RenderServlet.Delegate):
  '''AppEngine instances should never need to call out to Gitiles. That should
  only ever be done by the cronjobs, which then write the result into
  DataStore, which is as far as instances look. To enable this, crons can pass
  a custom (presumably online) ServerInstance into Get().

  Why? Gitiles is slow and a bit flaky. Refresh jobs failing is annoying but
  temporary. Instances failing affects users, and is really bad.

  Anyway - to enforce this, we actually don't give instances access to
  Gitiles.  If anything is missing from datastore, it'll be a 404. If the
  cronjobs don't manage to catch everything - uhoh. On the other hand, we'll
  figure it out pretty soon, and it also means that legitimate 404s are caught
  before a round trip to Gitiles.
  '''
  def __init__(self, delegate):
    self._delegate = delegate

  @memoize
  def CreateServerInstance(self):
    object_store_creator = ObjectStoreCreator(start_empty=False)
    branch_utility = self._delegate.CreateBranchUtility(object_store_creator)
    commit_tracker = CommitTracker(object_store_creator)
    # In production have offline=True so that we can catch cron errors. In
    # development it's annoying to have to run the cron job, so offline=False.
    # Note that offline=True if running on any appengine server due to
    # http://crbug.com/345361.
    host_file_system_provider = self._delegate.CreateHostFileSystemProvider(
        object_store_creator,
        offline=not (IsDevServer() or IsReleaseServer()),
        pinned_commit=commit_tracker.Get('master').Get(),
        cache_only=True)
    return ServerInstance(object_store_creator,
                          CompiledFileSystem.Factory(object_store_creator),
                          branch_utility,
                          host_file_system_provider,
                          CloudStorageFileSystemProvider(object_store_creator))

class InstanceServlet(object):
  '''Servlet for running on normal AppEngine instances.
  Create this via GetConstructor() so that cache state can be shared amongst
  them via the memoizing Delegate.
  '''
  class Delegate(object):
    '''Allow runtime dependencies to be overriden for testing.
    '''
    def CreateBranchUtility(self, object_store_creator):
      return BranchUtility.Create(object_store_creator)

    def CreateHostFileSystemProvider(self, object_store_creator, **optargs):
      return HostFileSystemProvider(object_store_creator, **optargs)

    def CreateGithubFileSystemProvider(self, object_store_creator):
      return GithubFileSystemProvider(object_store_creator)

  @staticmethod
  def GetConstructor(delegate_for_test=None):
    render_servlet_delegate = InstanceServletRenderServletDelegate(
        delegate_for_test or InstanceServlet.Delegate())
    return lambda request: RenderServlet(request, render_servlet_delegate)

  # NOTE: if this were a real Servlet it would implement a Get() method, but
  # GetConstructor returns an appropriate lambda function (Request -> Servlet).
