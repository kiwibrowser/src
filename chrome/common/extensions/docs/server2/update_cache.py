# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cPickle
import copy
import getopt
import json
import logging
import os.path
import sys
import traceback

from branch_utility import BranchUtility
from commit_tracker import CommitTracker
from compiled_file_system import CompiledFileSystem
from data_source_registry import CreateDataSources
from environment import GetAppVersion
from environment_wrappers import CreateUrlFetcher, GetAccessToken
from future import All
from gcs_file_system_provider import CloudStorageFileSystemProvider
from host_file_system_provider import HostFileSystemProvider
from local_git_util import ParseRevision
from object_store_creator import ObjectStoreCreator
from persistent_object_store_fake import PersistentObjectStoreFake
from render_refresher import RenderRefresher
from server_instance import ServerInstance
from timer import Timer


# The path template to use for flushing the memcached. Should be formatted with
# with the app version.
_FLUSH_MEMCACHE_PATH = ('https://%s-dot-chrome-apps-doc.appspot.com/'
                        '_flush_memcache')


def _UpdateCommitId(commit_name, commit_id):
  '''Sets the commit ID for a named commit. This is the final step performed
  during update. Once all the appropriate datastore entries have been populated
  for a new commit ID, the 'master' commit entry is updated to that ID and the
  frontend will begin serving the new data.

  Note that this requires an access token identifying the main service account
  for the chrome-apps-doc project. VM instances will get this automatically
  from their environment, but if you want to do a local push to prod you will
  need to set the DOCSERVER_ACCESS_TOKEN environment variable appropriately.
  '''
  commit_tracker = CommitTracker(
      ObjectStoreCreator(store_type=PersistentObjectStoreFake,
                         start_empty=False))
  commit_tracker.Set(commit_name, commit_id).Get()
  logging.info('Commit "%s" updated to %s.' % (commit_name, commit_id))


def _GetCachedCommitId(commit_name):
  '''Determines which commit ID was last cached.
  '''
  commit_tracker = CommitTracker(
      ObjectStoreCreator(store_type=PersistentObjectStoreFake,
                         start_empty=False))
  return commit_tracker.Get(commit_name).Get()


def _FlushMemcache():
  '''Requests that the frontend flush its memcached to avoid serving stale data.
  '''
  flush_url = _FLUSH_MEMCACHE_PATH % GetAppVersion()
  headers = { 'Authorization': 'Bearer %s' % GetAccessToken() }
  response = CreateUrlFetcher().Fetch(flush_url, headers)
  if response.status_code != 200:
    logging.error('Unable to flush memcache: HTTP %s (%s)' %
                  (response.status_code, response.content))
  else:
    logging.info('Memcache flushed.')


def _CreateServerInstance(commit):
  '''Creates a ServerInstance based on origin/master.
  '''
  object_store_creator = ObjectStoreCreator(
      start_empty=False, store_type=PersistentObjectStoreFake)
  branch_utility = BranchUtility.Create(object_store_creator)
  host_file_system_provider = HostFileSystemProvider(object_store_creator,
                                                     pinned_commit=commit)
  gcs_file_system_provider = CloudStorageFileSystemProvider(
      object_store_creator)
  return ServerInstance(object_store_creator,
                        CompiledFileSystem.Factory(object_store_creator),
                        branch_utility,
                        host_file_system_provider,
                        gcs_file_system_provider)


def _UpdateDataSource(name, data_source):
  try:
    class_name = data_source.__class__.__name__
    timer = Timer()
    logging.info('Updating %s...' % name)
    data_source.Refresh().Get()
  except Exception as e:
    logging.error('%s: error %s' % (class_name, traceback.format_exc()))
    raise e
  finally:
    logging.info('Updating %s took %s' % (name, timer.Stop().FormatElapsed()))


def UpdateCache(single_data_source=None, commit=None):
  '''Attempts to populate the datastore with a bunch of information derived from
  a given commit.
  '''
  server_instance = _CreateServerInstance(commit)

  # This is the thing that would be responsible for refreshing the cache of
  # examples. Here for posterity, hopefully it will be added to the targets
  # below someday.
  # render_refresher = RenderRefresher(server_instance, self._request)

  data_sources = CreateDataSources(server_instance)
  data_sources['content_providers'] = server_instance.content_providers
  data_sources['platform_bundle'] = server_instance.platform_bundle
  if single_data_source:
    _UpdateDataSource(single_data_source, data_sources[single_data_source])
  else:
    for name, source in data_sources.iteritems():
      _UpdateDataSource(name, source)


def _Main(argv):
  try:
    opts = dict((name[2:], value) for name, value in
                getopt.getopt(argv, '',
                              ['load-file=', 'data-source=', 'commit=',
                               'no-refresh', 'no-push', 'save-file=',
                               'no-master-update', 'push-all', 'force'])[0])
  except getopt.GetoptError as e:
    print '%s\n' % e
    print (
    'Usage: update_cache.py [options]\n\n'
    'Options:\n'
    '  --data-source=NAME        Limit update to a single data source.\n'
    '  --load-file=FILE          Load object store data from FILE before\n'
    '                            starting the update.\n'
    '  --save-file=FILE          Save object store data to FILE after running\n'
    '                            the update.\n'
    '  --no-refresh              Do not attempt to update any data sources.\n'
    '  --no-push                 Do not push to Datastore.\n'
    '  --commit=REV              Commit ID to use for master update.\n'
    '  --no-master-update        Do not update the master commit ID.\n'
    '  --push-all                Push all entities to the Datastore even if\n'
    '                            they do not differ from the loaded cache.\n\n'
    '  --force                   Force an update even if the latest commit is'
    '                            already cached.\n')
    exit(1)

  logging.getLogger().setLevel(logging.INFO)

  data_source = opts.get('data-source', None)
  load_file = opts.get('load-file', None)
  save_file = opts.get('save-file', None)
  do_refresh = 'no-refresh' not in opts
  do_push = 'no-push' not in opts
  do_master_update = 'no-master-update' not in opts
  push_all = do_push and ('push-all' in opts)
  commit = ParseRevision(opts.get('commit', 'origin/HEAD'))
  force_update = 'force' in opts

  original_data = {}
  if load_file:
    logging.info('Loading cache...')
    PersistentObjectStoreFake.LoadFromFile(load_file)
    if not push_all:
      original_data = copy.deepcopy(PersistentObjectStoreFake.DATA)

  last_commit = _GetCachedCommitId('master')
  if ParseRevision(commit) == last_commit and not force_update:
    logging.info('Latest cache (revision %s) is up to date. Bye.' % commit)
    exit(0)

  timer = Timer()
  if do_refresh:
    logging.info('Starting refresh from commit %s...' % ParseRevision(commit))
    if data_source:
      UpdateCache(single_data_source=data_source,
                   commit=commit)
    else:
      UpdateCache(commit=commit)

  if do_push:
    from datastore_util import PushData
    if do_master_update:
      _UpdateCommitId('master', commit)
    push_timer = Timer()
    logging.info('Pushing data into datastore...')
    PushData(PersistentObjectStoreFake.DATA, original_data=original_data)
    logging.info('Done. Datastore push took %s' %
                 push_timer.Stop().FormatElapsed())
    _FlushMemcache()
  if save_file:
    PersistentObjectStoreFake.SaveToFile(save_file)

  logging.info('Update completed in %s' % timer.Stop().FormatElapsed())


if __name__ == '__main__':
  _Main(sys.argv[1:])

