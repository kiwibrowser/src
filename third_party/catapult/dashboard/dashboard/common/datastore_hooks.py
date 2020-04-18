# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Adds pre hook to data store queries to hide internal-only data.

Checks if the user has a google.com address, and hides data with the
internal_only property set if not.
"""

import webapp2

from google.appengine.api import apiproxy_stub_map
from google.appengine.api import users
from google.appengine.datastore import datastore_pb

from dashboard.common import utils

# The list below contains all kinds that have an internal_only property.
# IMPORTANT: any new data types with internal_only properties must be added
# here in order to be restricted to internal users.
_INTERNAL_ONLY_KINDS = [
    'Bot',
    'TestMetadata',
    'Sheriff',
    'Anomaly',
    'TryJob',
    'TableConfig',
    'Histogram',
    'SparseDiagnostic',
]

# Permissions namespaces.
EXTERNAL = 'externally_visible'
INTERNAL = 'internal_only'


def InstallHooks():
  """Installs datastore pre hook to add access checks to queries.

  This only needs to be called once, when doing config (currently in
  appengine_config.py).
  """
  apiproxy_stub_map.apiproxy.GetPreCallHooks().Push(
      '_DatastorePreHook', _DatastorePreHook, 'datastore_v3')


def SetPrivilegedRequest():
  """Allows the current request to act as a privileged user.

  This should ONLY be called for handlers that are restricted from end users
  by some other mechanism (IP whitelisting, admin-only pages).

  This should be set once per request, before accessing the data store.
  """
  request = webapp2.get_request()
  request.registry['privileged'] = True


def SetSinglePrivilegedRequest():
  """Allows the current request to act as a privileged user only ONCE.

  This should be called ONLY by handlers that have checked privilege immediately
  before making a query. It will be automatically unset when the next query is
  made.
  """
  request = webapp2.get_request()
  request.registry['single_privileged'] = True


def CancelSinglePrivilegedRequest():
  """Disallows the current request to act as a privileged user only."""
  request = webapp2.get_request()
  request.registry['single_privileged'] = False


def _IsServicingPrivilegedRequest():
  """Checks whether the request is considered privileged."""
  try:
    request = webapp2.get_request()
  except AssertionError:
    # This happens in unit tests, when code gets called outside of a request.
    return False
  path = getattr(request, 'path', '')
  if path.startswith('/mapreduce'):
    return True
  if path.startswith('/_ah/queue/deferred'):
    return True
  if path.startswith('/_ah/pipeline/'):
    return True
  if request.registry.get('privileged', False):
    return True
  if request.registry.get('single_privileged', False):
    request.registry['single_privileged'] = False
    return True
  whitelist = utils.GetIpWhitelist()
  if whitelist and hasattr(request, 'remote_addr'):
    return request.remote_addr in whitelist
  return False


def IsUnalteredQueryPermitted():
  """Checks if the current user is internal, or the request is privileged.

  "Internal users" are users whose email address belongs to a certain
  privileged domain; but some privileged requests, such as task queue tasks,
  are also considered privileged.

  Returns:
    True for users with google.com emails and privileged requests.
  """
  if utils.IsInternalUser():
    return True
  if users.is_current_user_admin():
    # It's possible to be an admin with a non-internal account; For example,
    # the default login for dev appserver instances is test@example.com.
    return True
  return _IsServicingPrivilegedRequest()


def GetNamespace():
  """Returns a namespace prefix string indicating internal or external user."""
  return INTERNAL if IsUnalteredQueryPermitted() else EXTERNAL


def _DatastorePreHook(service, call, request, _):
  """Adds a filter which checks whether to return internal data for queries.

  If the user is not privileged, we don't want to return any entities that
  have internal_only set to True. That is done here in a datastore hook.
  See: https://developers.google.com/appengine/articles/hooks

  Args:
    service: Service name, must be 'datastore_v3'.
    call: String representing function to call. One of 'Put', Get', 'Delete',
        or 'RunQuery'.
    request: Request protobuf.
    _: Response protobuf (not used).
  """
  assert service == 'datastore_v3'
  if call != 'RunQuery':
    return
  if request.kind() not in _INTERNAL_ONLY_KINDS:
    return
  if IsUnalteredQueryPermitted():
    return

  # Add a filter for internal_only == False, because the user is external.
  try:
    external_filter = request.filter_list().add()
  except AttributeError:
    # This is required to support proto1, which may be used by the unit tests.
    # Later, if we don't need to support proto1, then this can be removed.
    external_filter = request.add_filter()
  external_filter.set_op(datastore_pb.Query_Filter.EQUAL)
  new_property = external_filter.add_property()
  new_property.set_name('internal_only')
  new_property.mutable_value().set_booleanvalue(False)
  new_property.set_multiple(False)
