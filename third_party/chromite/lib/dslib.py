# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for interacting with gcloud datastore."""

from __future__ import print_function

from gcloud import datastore
import json

from chromite.lib import cros_logging as logging
from chromite.lib import iter_utils


_BATCH_CHUNK_SIZE = 500


def GetClient(creds_file, project_id=None, namespace=None):
  """Get a datastore client instance.

  Args:
    creds_file: Path to JSON creds file.
    project_id: Optional project_id of datastore. If not supplied,
                will use one from creds file.
    namespace: Optional namespace to insert into.

  Returns:
    A (datastore.Client instance, project_id) tuple.
  """
  if project_id is None:
    with open(creds_file, 'r') as f:
      project_id = json.load(f)['project_id']

  return datastore.Client.from_service_account_json(
      creds_file, project_id, namespace=namespace), project_id


def ChunkedBatchWrite(entities, client, batch_size=_BATCH_CHUNK_SIZE):
  """Write |entities| to datastore |client| in batches of size |batch_size|.

  Datastore has a entities-per-batch limit of 500. This utility function breaks
  helps write a large number of entities to datastore by splitting it into
  limited size batch writes.

  Args:
    entities: iterator of datastore entities to write.
    client: datastore.Client instance.
    batch_size: (default: 500) Maximum number of entities per batch.
  """
  for chunk in iter_utils.SplitToChunks(entities, batch_size):
    entities = list(chunk)

    batch = client.batch()
    for entity in entities:
      batch.put(entity)
    try:
      batch.commit()
    except gcloud.exceptions.BadRequest:
      logging.warn('Unexportable entities:\n%s', entities)
      raise
