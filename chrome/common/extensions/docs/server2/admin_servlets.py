# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging

from appengine_wrappers import memcache
from commit_tracker import CommitTracker
from environment import IsDevServer
from environment_wrappers import CreateUrlFetcher
from future import All
from object_store_creator import ObjectStoreCreator
from servlet import Servlet, Response


# This is the service account ID associated with the chrome-apps-doc project in
# the Google Developers Console.
_SERVICE_ACCOUNT_NAME = '636061184119-compute@developer.gserviceaccount.com'

# This is Google's OAuth2 service for retrieving user information from an access
# token. It is used to authenticate an incoming access token in the metadata
# flush servlet, ensuring that only requests from the above service account are
# fulfilled.
_ACCOUNT_INFO_URL = ('https://www.googleapis.com/oauth2/v1/userinfo?'
                     'access_token=%s')


class QueryCommitServlet(Servlet):
  '''Provides read access to the commit ID cache within the server. For example:

  /_query_commit/master

  will return the commit ID stored under the commit key "master" within the
  commit cache. Currently "master" is the only named commit we cache, and it
  corresponds to the commit ID whose data currently populates the data cache
  used by live instances.
  '''
  def __init__(self, request):
    Servlet.__init__(self, request)

  def Get(self):
    object_store_creator = ObjectStoreCreator(start_empty=False)
    commit_tracker = CommitTracker(object_store_creator)

    def generate_response(result):
      commit_id, history = result
      history_log = ''.join('%s: %s<br>' % (entry.datetime, entry.commit_id)
                            for entry in reversed(history))
      response = 'Current commit: %s<br><br>Most recent commits:<br>%s' % (
          commit_id, history_log)
      return response

    commit_name = self._request.path
    id_future = commit_tracker.Get(commit_name)
    history_future = commit_tracker.GetHistory(commit_name)
    return Response.Ok(
        All((id_future, history_future)).Then(generate_response).Get())


class FlushMemcacheServlet(Servlet):
  '''Flushes the entire memcache.

  This requires an access token for the project's main service account. Without
  said token, the request is considered invalid.
  '''

  class Delegate(object):
    def IsAuthorized(self, access_token):
      '''Verifies that a given access_token represents the main service account.
      '''
      fetcher = CreateUrlFetcher()
      response = fetcher.Fetch(_ACCOUNT_INFO_URL % access_token)
      if response.status_code != 200:
        return False
      try:
        info = json.loads(response.content)
      except:
        return False
      return info['email'] == _SERVICE_ACCOUNT_NAME

  def __init__(self, request, delegate=Delegate()):
    Servlet.__init__(self, request)
    self._delegate = delegate

  def GetAccessToken(self):
    auth_header = self._request.headers.get('Authorization')
    if not auth_header:
      return None
    try:
      method, token = auth_header.split(' ', 1)
    except:
      return None
    if method != 'Bearer':
      return None
    return token

  def Get(self):
    access_token = self.GetAccessToken()
    if not access_token:
      return Response.Unauthorized('Unauthorized', 'Bearer', 'update')
    if not self._delegate.IsAuthorized(access_token):
      return Response.Forbidden('Forbidden')
    result = memcache.flush_all()
    return Response.Ok('Flushed: %s' % result)


class UpdateCacheServlet(Servlet):
  '''Devserver-only servlet for pushing local file data into the datastore.
  This is useful if you've used update_cache.py to build a local datastore
  for testing. Query:

      /_update_cache/FOO_DATA

  to make the devserver read FOO_DATA from its pwd and push all the data into
  datastore.
  '''
  def __init__(self, request):
    Servlet.__init__(self, request)

  def Get(self):
    if not IsDevServer():
      return Response.BadRequest('')
    import cPickle
    from persistent_object_store_appengine import PersistentObjectStoreAppengine
    with open(self._request.path, 'r') as f:
      data = cPickle.load(f)
    for namespace, contents in data.iteritems():
      store = PersistentObjectStoreAppengine(namespace)
      for k, v in cPickle.loads(contents).iteritems():
        try:
          store.Set(k, v).Get()
        except:
          logging.warn('Skipping entry %s because of errors.' % k)
    return Response.Ok('Data pushed!')
