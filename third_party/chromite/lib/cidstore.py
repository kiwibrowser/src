# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Client library for Continuous Integration Datastore."""

from __future__ import print_function

# Safe import of gcloud.datastore module. If the module is not imported, most
# operations in the module are safe no-ops.
_datastore_imported = False
_datastore_import_exc = None
try:
  from gcloud import datastore
  _datastore_imported = True
except Exception as e:
  datastore = None
  _datastore_import_exc = e


class CIDStoreClient(object):
  """Client object for interacting with CIDStore Cloud Datastore.

  This wrapper class provides import safety. If the gcloud datastore client
  library is not importable, the constructor to this class and all its methods
  will silently do nothin and return None.

  However, if the client library is correctly imported, but there are errors
  in the account credentials or aother errors, those exceptions will be raised.
  """

  _client = None

  def __init__(self, service_account_json_path, project_id):
    """Construct a CIDStoreClient.

    Args:
      service_account_json_path: Absolute path to a gcloud service account
          json credential file.
      project_id: String identifier of gcloud project to connect to.
    """
    if not _datastore_imported:
      return

    self._client = datastore.Client.from_service_account_json(
        service_account_json_path, project_id)

  def IsConnected(self):
    """Returns True if this wrapper is connected to gcloud datastore."""
    return self._client is not None
