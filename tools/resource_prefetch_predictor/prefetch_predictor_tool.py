#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Inspection of the prefetch predictor database.

On Android, the database can be extracted using:
adb pull \
  '/data/user/0/$package_name/app_chrome/Default/Network Action Predictor'
  predictor_db
"""

import argparse
import datetime
import sqlite3
import os
import sys


class Entry(object):
  """Represents an entry in the predictor database."""
  def __init__(
      self, primary_key, proto_buffer):
    from resource_prefetch_predictor_pb2 import (PrefetchData, ResourceData)
    self.primary_key = primary_key
    self.prefetch_data = PrefetchData()
    self.prefetch_data.ParseFromString(proto_buffer)

  @classmethod
  def _ComputeResourceScore(cls, resource):
    """Mirrors ResourcePrefetchPredictorTables::ComputeResourceScore.

    Args:
      resource: ResourceData.

    Return:
      The resource score (int).
    """
    from resource_prefetch_predictor_pb2 import (PrefetchData, ResourceData)
    priority_multiplier = 1
    type_multiplier = 1

    if resource.priority == ResourceData.REQUEST_PRIORITY_HIGHEST:
      priority_multiplier = 3
    elif resource.priority == ResourceData.REQUEST_PRIORITY_MEDIUM:
      priority_multiplier = 2

    if resource.resource_type in (ResourceData.RESOURCE_TYPE_STYLESHEET,
                                  ResourceData.RESOURCE_TYPE_SCRIPT):
      type_multiplier = 3
    elif resource.resource_type == ResourceData.RESOURCE_TYPE_FONT_RESOURCE:
      type_multiplier = 2

    return (100 * (priority_multiplier * 100 + type_multiplier * 10)
            - resource.average_position)

  @classmethod
  def FromRow(cls, row):
    """Builds an entry from a database row."""
    return Entry(*row)

  @classmethod
  def _PrettyPrintResource(cls, resource):
    """Pretty-prints a resource to stdout.

    Args:
      resource: ResourceData.
    """
    print 'score: %d' % cls._ComputeResourceScore(resource)
    print resource

  def PrettyPrintCandidates(self):
    """Prints the candidates for prefetch."""
    print 'primary_key: %s' % self.prefetch_data.primary_key
    for resource in self.prefetch_data.resources:
      confidence = float(resource.number_of_hits) / (
          resource.number_of_hits + resource.number_of_misses)
      if resource.number_of_hits < 2 or confidence < .7:
        continue
      self._PrettyPrintResource(resource)

def DumpOriginDatabaseRow(domain, primary_key, proto):
  from resource_prefetch_predictor_pb2 import OriginData
  entry = OriginData()
  entry.ParseFromString(proto)
  # For the offset, see kTimeTToMicrosecondsOffset in base/time/time.h.
  last_visit_timestamp = int(entry.last_visit_time / 1e6 - 11644473600)
  formatted_last_visit_time = datetime.datetime.utcfromtimestamp(
      last_visit_timestamp).strftime('%Y-%m-%d %H:%M:%S')
  print '''host: %s
last_visit_time: %s
origins:''' % (entry.host, formatted_last_visit_time)
  for origin_stat in entry.origins:
    print '''  origin: %s
  number_of_hits: %d
  number_of_misses: %d
  consecutive_misses: %d
  average_position: %f
  always_access_network: %s
  accessed_network: %s
''' % (origin_stat.origin, origin_stat.number_of_hits,
       origin_stat.number_of_misses, origin_stat.consecutive_misses,
       origin_stat.average_position, origin_stat.always_access_network,
       origin_stat.accessed_network)


# The version of python sqlite3 library we have in Ubuntu 14.04 LTS doesn't
# support views but command line util does.
# TODO(alexilin): get rid of this when python sqlite3 adds view support.
def CreateCompatibleDatabaseCopy(filename):
  import tempfile, shutil, subprocess
  _, tmpfile = tempfile.mkstemp()
  shutil.copy2(filename, tmpfile)
  subprocess.call(['sqlite3', tmpfile, 'DROP VIEW MmapStatus'])
  return tmpfile

def DatabaseStats(filename, host):
  query_template = 'SELECT key, proto from %s'
  connection = sqlite3.connect(filename)
  c = connection.cursor()
  print 'HOST DATABASE'
  query = query_template % 'resource_prefetch_predictor_host'
  entries = [Entry.FromRow(row) for row in c.execute(query)]
  for x in entries:
    if host is None or x.primary_key == host:
      x.PrettyPrintCandidates()

  print '\n\nORIGIN DATABASE'
  query = query_template % 'resource_prefetch_predictor_origin'
  rows = [row for row in c.execute(query)
          if host is None or row.primary_key == host]
  for row in rows:
    DumpOriginDatabaseRow(host, *row)


def _AddProtocolBuffersPath(build_dir):
  assert os.path.isdir(build_dir)
  proto_dir = os.path.join(
      build_dir, os.path.join('pyproto', 'chrome', 'browser', 'predictors'))
  sys.path.append(proto_dir)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-f', dest='database_filename', required=True,
                      help='Path to the database')
  parser.add_argument('-d', dest='domain', default=None, help='Domain')
  parser.add_argument('--build-dir', dest='build_dir', required=True,
                      help='Path to the build directory.')
  args = parser.parse_args()
  _AddProtocolBuffersPath(args.build_dir)

  try:
    database_copy = CreateCompatibleDatabaseCopy(args.database_filename)
    DatabaseStats(database_copy, args.domain)
  finally:
    if os.path.exists(database_copy):
      os.remove(database_copy)


if __name__ == '__main__':
  main()
