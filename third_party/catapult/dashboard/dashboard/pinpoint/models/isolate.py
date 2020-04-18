# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Model for storing information to look up isolates.

An isolate is a way to describe the dependencies of a specific build.

More about isolates:
https://github.com/luci/luci-py/blob/master/appengine/isolate/doc/client/Design.md
"""

import hashlib

from google.appengine.ext import ndb


def Get(builder_name, change, target):
  """Retrieve an isolate hash from the Datastore.

  Args:
    builder_name: The name of the builder that produced the isolate.
    change: The Change the isolate was built at.
    target: The compile target the isolate is for.

  Returns:
    A tuple containing the isolate server and isolate hash as strings.
  """
  entity = ndb.Key(Isolate, _Key(builder_name, change, target)).get()
  if not entity:
    entity = ndb.Key(Isolate, _OldKey(builder_name, change, target)).get()
    if not entity:
      raise KeyError('No isolate with builder %s, change %s, and target %s.' %
                     (builder_name, change, target))
  return entity.isolate_server, entity.isolate_hash


def Put(isolate_server, isolate_infos):
  """Add isolate hashes to the Datastore.

  This function takes multiple entries to do a batched Datstore put.

  Args:
    isolate_server: The hostname of the server where the isolates are stored.
    isolate_infos: An iterable of tuples. Each tuple is of the form
        (builder_name, change, target, isolate_hash).
  """
  entities = []
  for isolate_info in isolate_infos:
    builder_name, change, target, isolate_hash = isolate_info
    entity = Isolate(
        isolate_server=isolate_server,
        isolate_hash=isolate_hash,
        id=_Key(builder_name, change, target))
    entities.append(entity)
  ndb.put_multi(entities)


class Isolate(ndb.Model):
  # TODO: Make isolate_server `required=True` in November 2018.
  isolate_server = ndb.StringProperty(indexed=False)
  isolate_hash = ndb.StringProperty(indexed=False, required=True)
  created = ndb.DateTimeProperty(auto_now_add=True)


def _Key(builder_name, change, target):
  # The key must be stable across machines, platforms,
  # Python versions, and Python invocations.
  return '\n'.join((builder_name, change.id_string, target))


# TODO: In October 2018, remove this and delete all Isolates without
# a creation date. Isolates expire after about 6 months. crbug.com/828778
def _OldKey(builder_name, change, target):
  # The key must be stable across machines, platforms,
  # Python versions, and Python invocations.
  string = '\n'.join((builder_name[:-5], change.id_string, target))
  return hashlib.sha256(string).hexdigest()
