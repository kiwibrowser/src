# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import re
import time

from google.appengine.api import urlfetch
import webapp2

from base import bigquery
from base import constants
from common import buildbot


class Builds(webapp2.RequestHandler):

  def get(self):
    urlfetch.set_default_fetch_deadline(300)

    bq = bigquery.BigQuery()

    current_events = []
    events = []
    for master_name in constants.MASTER_NAMES:
      builders = buildbot.Builders(master_name)
      available_builds = _AvailableBuilds(builders)
      recorded_builds = _RecordedBuilds(bq, builders, available_builds)
      for builder in builders:
        # Filter out recorded builds from available builds.
        build_numbers = (available_builds[builder.name] -
                         recorded_builds[builder.name])
        builder_current_events, builder_events = _TraceEventsForBuilder(
            builder, build_numbers)
        current_events += builder_current_events
        events += builder_events

    jobs = []
    if current_events:
      jobs += bq.InsertRowsAsync(
          constants.DATASET, constants.CURRENT_BUILDS_TABLE,
          current_events, truncate=True)
    if events:
      jobs += bq.InsertRowsAsync(constants.DATASET, constants.BUILDS_TABLE,
                                 events)

    for job in jobs:
      bq.PollJob(job, 60 * 20)  # 20 minutes.


def _AvailableBuilds(builders):
  available_builds = {}
  for builder in builders:
    if not builder.cached_builds:
      available_builds[builder.name] = frozenset()
      continue

    max_build = max(builder.cached_builds)
    # Buildbot on tryserver.chromium.perf is occasionally including build 0 in
    # its list of cached builds. That results in more builds than we want.
    # Limit the list to the last 100 builds, because the urlfetch URL limit is
    # 2048 bytes, and "&select=100000" * 100 is 1400 bytes.
    builds = frozenset(build for build in builder.cached_builds
                       if build >= max_build - 100)
    available_builds[builder.name] = builds
  return available_builds


def _RecordedBuilds(bq, builders, available_builds):
  # 105 days / 15 weeks. Must be some number greater than 100 days, because
  # we request up to 100 builds (see above comment), and the slowest cron bots
  # run one job every day.
  start_time_ms = -1000 * 60 * 60 * 24 * 105
  table = '%s.%s@%d-' % (constants.DATASET, constants.BUILDS_TABLE,
                         start_time_ms)

  conditions = []
  for builder in builders:
    if not available_builds[builder.name]:
      continue
    max_build = max(available_builds[builder.name])
    min_build = min(available_builds[builder.name])
    conditions.append('WHEN builder = "%s" THEN build >= %d AND build <= %d' %
                      (builder.name, min_build, max_build))

  query = (
      'SELECT builder, build '
      'FROM [%s] ' % table +
      'WHERE CASE %s END ' % ' '.join(conditions) +
      'GROUP BY builder, build'
  )
  query_result = bq.QuerySync(query, 600)

  builds = collections.defaultdict(set)
  for row in query_result:
    builds[row['f'][0]['v']].add(int(row['f'][1]['v']))
  return builds


def _TraceEventsForBuilder(builder, build_numbers):
  if not build_numbers:
    return (), ()

  build_numbers_string = ', '.join(map(str, sorted(build_numbers)))
  logging.info('Getting %s: %s', builder.name, build_numbers_string)

  # Fetch build information and generate trace events.
  current_events = []
  events = []

  builder_builds = builder.builds.Fetch(build_numbers)
  query_time = time.time()
  for build in builder_builds:
    if build.complete:
      events += _TraceEventsFromBuild(builder, build, query_time)
    else:
      current_events += _TraceEventsFromBuild(builder, build, query_time)

  return current_events, events


def _TraceEventsFromBuild(builder, build, query_time):
  match = re.match(r'(.+) \(([0-9]+)\)', builder.name)
  if match:
    configuration, host_shard = match.groups()
    host_shard = int(host_shard)
  else:
    configuration = builder.name
    host_shard = 0

  # Build trace event.
  if build.end_time:
    build_end_time = build.end_time
  else:
    build_end_time = query_time
  os, os_version, role = _ParseBuilderName(builder.name)
  yield {
      'name': 'Build %d' % build.number,
      'start_time': build.start_time,
      'end_time': build_end_time,

      'build': build.number,
      'builder': builder.name,
      'configuration': configuration,
      'host_shard': host_shard,
      'hostname': build.slave_name,
      'master': builder.master_name,
      'os': os,
      'os_version': os_version,
      'role': role,
      'status': build.status,
      'url': build.url,
  }

  # Step trace events.
  for step in build.steps:
    if not step.start_time:
      continue

    if step.name == 'steps':
      continue

    if step.end_time:
      step_end_time = step.end_time
    else:
      step_end_time = query_time
    yield {
        'name': step.name,
        'start_time': step.start_time,
        'end_time': step_end_time,

        'benchmark': step.name,  # TODO(dtu): This isn't always right.
        'build': build.number,
        'builder': builder.name,
        'configuration': configuration,
        'host_shard': host_shard,
        'hostname': build.slave_name,
        'master': builder.master_name,
        'os': os,
        'os_version': os_version,
        'role': role,
        'status': step.status,
        'url': step.url,
    }


def _ParseBuilderName(builder_name):
  builder_name = builder_name.lower()

  for os in ('android', 'linux', 'mac', 'win'):
    if os in builder_name:
      break
  else:
    os = None

  if 'build' in builder_name or 'compile' in builder_name:
    role = 'builder'
  else:
    role = 'tester'

  return (os, None, role)
