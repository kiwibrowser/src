# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import datetime

from object_store_creator import ObjectStoreCreator
from future import Future


# The maximum number of commit IDs to retain in a named commit's history deque.
_MAX_COMMIT_HISTORY_LENGTH = 50


class CachedCommit(object):
  '''Object type which is stored for each entry in a named commit's history.
  |datetime| is used as a timestamp for when the commit cache was completed,
  and is only meant to provide a loose ordering of commits for administrative
  servlets to display.'''
  def __init__(self, commit_id, datetime):
    self.commit_id = commit_id
    self.datetime = datetime


class CommitTracker(object):
  '''Utility class for managing and querying the storage of various named commit
  IDs.'''
  def __init__(self, object_store_creator):
    # The object stores should never be created empty since the sole purpose of
    # this tracker is to persist named commit data across requests.
    self._store = object_store_creator.Create(CommitTracker, start_empty=False)
    self._history_store = object_store_creator.Create(CommitTracker,
        category='history', start_empty=False)

  def Get(self, key):
    return self._store.Get(key)

  def Set(self, key, commit):
    return (self._store.Set(key, commit)
      .Then(lambda _: self._UpdateHistory(key, commit)))

  def GetHistory(self, key):
    '''Fetches the commit ID history for a named commit. If the commit has no
    history, this will return an empty collection.'''
    return (self._history_store.Get(key)
        .Then(lambda history: () if history is None else history))

  def _UpdateHistory(self, key, commit):
    '''Appends a commit ID to a named commit's tracked history.'''
    def create_or_amend_history(history):
      if history is None:
        history = collections.deque([], maxlen=50)
      history.append(CachedCommit(commit, datetime.datetime.now()))
      return self._history_store.Set(key, history)
    return self._history_store.Get(key).Then(create_or_amend_history)
