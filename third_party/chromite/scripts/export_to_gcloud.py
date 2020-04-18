# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Export entities to gcloud datastore."""

from __future__ import print_function

from gcloud import datastore
import ast
import json

from chromite.lib import commandline
from chromite.lib import dslib


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('service_acct_json', type=str, action='store',
                      help='Path to service account credentials JSON file.')
  parser.add_argument('entities', type=str, action='store',
                      help=('Path to file with entities to export. '
                            'File should be newline-separated JSON entries.'))
  parser.add_argument('--project_id', '-i', type=str, action='store',
                      default=None,
                      help=('Optional project_id of datastore to write to. If '
                            'not supplied, will be taken from credentials '
                            'file.'))
  parser.add_argument('--namespace', '-n', type=str, action='store',
                      default=None,
                      help='Optional namespace in which to store entities.')
  parser.add_argument('--parent_key', '-p', type=str, action='store',
                      default=None,
                      help='Key of parent entity to insert into. This should '
                      'be in python tuple-literal form, e.g. ("Foo", 1)')
  return parser


class DuplicateKeyError(ValueError):
  """Raised when two Entities have the same key."""


def GetEntities(project_id, json_lines, outer_parent_key=None, namespace=None):
  """Create gcloud entities from json string entries.

  project_id: String gcloud project id that entities are for.
  json_lines: File or other line-by-line iterator of json strings to turn into
              entities.
  outer_parent_key: Optional datastore.Key instance to act as the parent_key
                    of all top level entities.
  namespace: Optional string namespace for entities.
  """
  entity_keys = {}

  for line in json_lines:
    item = json.loads(line)
    kind, idx = item.pop('id')
    parent = item.pop('parent', None)

    if (kind, idx) in entity_keys:
      raise DuplicateKeyError(
          'Duplicate entities with id (%s, %s)' % (kind, idx))

    if parent:
      parent_key = entity_keys[tuple(parent)]
    else:
      parent_key = outer_parent_key

    key = datastore.Key(
        kind, idx, project=project_id, parent=parent_key, namespace=namespace)
    e = datastore.Entity(key=key)
    e.update(item)
    entity_keys[(kind, idx)] = key
    entity_keys[idx] = key

    yield e


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  entities_path = options.entities
  creds_file = options.service_acct_json
  project_id = options.project_id
  namespace = options.namespace

  entities = []
  c, project_id = dslib.GetClient(creds_file, project_id, namespace)

  if options.parent_key:
    upper_parent_key = c.key(*ast.literal_eval(options.parent_key))
  else:
    upper_parent_key = None


  with open(entities_path, 'r') as f:
    entities = GetEntities(project_id, f, upper_parent_key, namespace)
    dslib.ChunkedBatchWrite(entities, c)
