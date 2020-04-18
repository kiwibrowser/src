# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script that shows build timing for a build, and it's stages.

This script shows how long a build took, how long each stage took, and when each
stage started relative to the start of the build.
"""

from __future__ import print_function
from sets import Set

import collections
import datetime

from chromite.lib import constants
from chromite.lib import cros_build_lib


BUILD_TYPE_MAP = {
    'cq': constants.CQ_MASTER,
    'canary': constants.CANARY_MASTER,
    'chrome-pfq': constants.PFQ_MASTER,
}


BuildTiming = collections.namedtuple(
    'BuildTiming', ['id', 'build_config', 'success',
                    'start', 'finish', 'duration',
                    'stages'])


# Sometimes used with TimeDeltas, and sometimes with TimeDeltaStats.
StageTiming = collections.namedtuple(
    'StageTiming', ['name', 'start', 'finish', 'duration'])


def GetPerBuildStageStats(build_statuses):
  """Creates a map of builders to their individual stages.

  Args:
    build_statuses: List of build_status dictionaries as returned by various
                    CIDB methods.

  Returns:
    final_map: A map of builders to a dict of the individual stages for that
      builder.
  """
  all_builds = {}
  final_map = {}

  # Group all builds by build config.
  for status in build_statuses:
    all_builds.setdefault(status['build_config'], []).extend(status['stages'])

  # Group the individual stages now.
  for b in all_builds:
    stage_map = {}
    stages = all_builds[b]

    for s in stages:
      stage_map.setdefault(s['name'], []).append(s)

    final_map[b] = stage_map

  return final_map


class TimeDeltaStats(collections.namedtuple(
    'TimeDeltaStats', ['median', 'mean', 'min', 'max'])):
  """Collection a stats about a set of time.timedelta values."""
  __slotes__ = ()
  def __str__(self):
    return 'median %s mean %s min %s max %s' % self


def FillInBuildStatusesWithStages(db, build_statuses):
  """Fill in a 'stages' value for a list of build_statuses.

  Modifies the build_status objects in-place.

  Args:
    db: cidb.CIDBConnection object.
    build_statuses: List of build_status dictionaries as returned by various
                    CIDB methods.
  """
  ids = [status['id'] for status in build_statuses]
  all_stages = db.GetBuildsStages(ids)

  stages_by_build_id = cros_build_lib.GroupByKey(all_stages, 'build_id')

  for status in build_statuses:
    status['stages'] = stages_by_build_id.get(status['id'], [])


def BuildIdToBuildStatus(db, build_id):
  """Fetch a BuildStatus (with stages) from cidb from a build_id.

  Args:
    db: cidb.CIDBConnection object.
    build_id: build id as an integer.

  Returns:
    build status dictionary from CIDB with 'stages' field populated.
  """
  build_status = db.GetBuildStatus(build_id)
  build_status['stages'] = db.GetBuildStages(build_id)
  return build_status


def FilterBuildStatuses(build_statuses):
  """We only want to process passing 'normal' builds for stats.

  Args:
    build_statuses: List of Cidb result dictionary. 'stages' are not needed.

  Returns:
    List of all build statuses that weren't removed.
  """
  # Ignore release branches, branch builders, chrome waterfall, etc.
  WATERFALLS = ('chromeos', 'chromiumos', 'chromiumos.tryserver')

  return [status for status in build_statuses
          if status['waterfall'] in WATERFALLS]


def BuildConfigToStatuses(db, build_config, start_date, end_date):
  """Find a list of BuildStatus dictionaries with stages populated.

  Args:
    db: cidb.CIDBConnection object.
    build_config: Name of build config to find builds for.
    start_date: datetime.datetime object for start of search range.
    end_date: datetime.datetime object for end of search range.

  Returns:
    A list of cidb style BuildStatus dictionaries with 'stages' populated.
  """
  # Find builds.
  build_statuses = db.GetBuildHistory(
      build_config, db.NUM_RESULTS_NO_LIMIT,
      start_date=start_date, end_date=end_date)

  build_statuses = FilterBuildStatuses(build_statuses)

  # Fill in stage information.
  FillInBuildStatusesWithStages(db, build_statuses)
  return build_statuses


def MasterConfigToStatuses(db, build_config, start_date, end_date):
  """Find a list of BuildStatuses for all master/slave builds.

  Args:
    db: cidb.CIDBConnection object.
    build_config: Name of build config of master builder.
    start_date: datetime.datetime object for start of search range.
    end_date: datetime.datetime object for end of search range.

  Returns:
    A list of cidb style BuildStatus dictionaries with 'stages' populated.
  """
  # Find masters.
  master_statuses = db.GetBuildHistory(
      build_config, db.NUM_RESULTS_NO_LIMIT,
      start_date=start_date, end_date=end_date)

  # Find slaves.
  slave_statuses = []
  for status in master_statuses:
    slave_statuses += db.GetSlaveStatuses(status['id'])

  build_statuses = FilterBuildStatuses(master_statuses + slave_statuses)

  # Fill in stage information.
  FillInBuildStatusesWithStages(db, build_statuses)
  return build_statuses


def GetBuildTimings(build_status):
  """Convert a build_status with stages into BuildTimings.

  After filling in a build_status dictionary with stage information
  (FillInBuildStatusesWithStages), convert to a BuildTimings tuple with only the
  data we care about.

  Args:
    build_status: Cidb result dictionary with 'stages' added.

  Returns:
    BuildTimings tuple with all time values populated as timedeltas or None.
  """
  start = build_status['start_time']

  def safeDuration(start, finish):
    # Do time math, but don't raise on a missing value.
    if start is None or finish is None:
      return None
    return finish - start

  stage_times = []
  for stage in build_status['stages']:
    stage_times.append(
        StageTiming(stage['name'],
                    safeDuration(start, stage['start_time']),
                    safeDuration(start, stage['finish_time']),
                    safeDuration(stage['start_time'], stage['finish_time'])))

  return BuildTiming(build_status['id'],
                     build_status['build_config'],
                     build_status['status'] == 'pass',
                     build_status['start_time'],
                     build_status['finish_time'],
                     safeDuration(start, build_status['finish_time']),
                     stage_times)


def CalculateTimeStats(durations):
  """Use a set of durations to populate a TimeDelaStats.

  Args:
    durations: A list of timedate.timedelta objects. May contain None values.

  Returns:
    A TimeDeltaStats object or None (if no valid deltas).
  """
  durations = [d for d in durations if d is not None]

  if not durations:
    return None

  durations.sort()
  median = durations[len(durations) / 2]

  # Convert to seconds so we can round to nearest second.
  summation = sum(d.total_seconds() for d in durations)
  average = datetime.timedelta(seconds=int(summation / len(durations)))

  minimum = durations[0]
  maximum = durations[-1]

  return TimeDeltaStats(median, average, minimum, maximum)


def CalculateBuildStats(builds_timings):
  """Find total build time stats for a set of BuildTiming objects.

  Args:
    builds_timings: List of BuildTiming objects.

  Returns:
    Successful build count as int.
    Timeout build count as int.
    Total build count as int.
    TimeDeltaStats object,or None if no valid builds.
  """
  timeout_count = sum(1 for b in builds_timings if b.finish is None)
  successful = [b.duration for b in builds_timings if b.success]
  time_stats = CalculateTimeStats(successful)

  return len(successful), timeout_count, len(builds_timings), time_stats


def CalculateStageStats(builds_timings):
  """Find time stats for all stages in a set of BuildTiming objects.

  Given a set of builds, find all unique stage names, and calculate average
  stats across all instances of that stage name.

  Given a list of 20 builds, if a stage is only in 2 of them, it's stats are
  only computed across the two instances.

  Args:
    builds_timings: List of BuildTiming objects.

  Returns:
    List of StageTiming objects with all time values populated with
    TimeDeltaStats values.
  """
  stage_map = {}
  for b in builds_timings:
    if b.success:
      for s in b.stages:
        stage_map.setdefault(s.name, []).append(s)

  stage_stats = []
  for name in stage_map.iterkeys():
    named_stages = stage_map[name]
    stage_stats.append(
        StageTiming(
            name=name,
            start=CalculateTimeStats([s.start for s in named_stages]),
            finish=CalculateTimeStats([s.finish for s in named_stages]),
            duration=CalculateTimeStats([s.duration for s in named_stages])))

  return stage_stats


def GetNumDistinctChromeVersions(db, build_config, start_date, end_date):
  """Get the number of distinct chrome versions.

  This represents the number of successful chrome uprevs.

  Args:
    db: cidb.CIDBConnection object.
    build_config: Name of build config of master builder.
    start_date: datetime.datetime object for start of search range.
    end_date: datetime.datetime object for end of search range.

  Returns:
    A list of cidb style BuildStatus dictionaries with 'stages' populated.
  """
  # Find masters.
  master_statuses = db.GetBuildHistory(
      build_config, db.NUM_RESULTS_NO_LIMIT,
      start_date=start_date, end_date=end_date)

  # Collect chrome versions from successful runs in a Set.
  chrome_versions = Set()
  for status in master_statuses:
    if status['status'] == 'pass':
      metadata = db.GetMetadata(status['id'])
      if metadata is not None:
        chrome_version = metadata['chrome_version']
        if chrome_version is not None:
          chrome_versions.add(chrome_version)

  return len(chrome_versions)


def GetBuildSuccessRates(build_statuses):
  """Get the success rate for each builder.

  Args:
    build_statuses: Array of cidb result dictionaries.

  Returns:
    A dictionary of success rates by build.
  """
  config_successes = {}
  for build_status in build_statuses:
    config = build_status['build_config']
    success = build_status['status'] == 'pass'
    config_successes.setdefault(config, []).append(success)
    config_successes.setdefault('total', []).append(success)

  success_rates = {}
  for key, successes in config_successes.iteritems():
    success_rates[key] = Percent(sum(successes), len(successes))
  return success_rates


def GetStageSuccessRates(build_statuses):
  """Get the success rate for each stage.

  Args:
    build_statuses: Array of cidb result dictionaries.

  Returns:
    An dictionary of success rates by stage.
  """
  config_successes = {}
  for build_status in build_statuses:
    for stage in build_status['stages']:
      status = stage['status']
      # TODO(stevenjb): Handle 'planned', 'forgiven', 'inflight'?
      if (status == 'skipped' or
          status == 'planned' or status == 'forgiven' or status == 'inflight'):
        continue
      success = stage['status'] != 'fail'
      config_successes.setdefault(stage['name'], []).append(success)

  success_rates = {}
  for key, successes in config_successes.iteritems():
    success_rates[key] = Percent(sum(successes), len(successes))

  return success_rates


def FindAndSortStageStats(focus_build, builds_timings):
  """Return a list of stage names, sorted by median start time.

  The stage names returned will exist in the focus_build, or stage stages, but
  might not be in both.

  Args:
    focus_build: BuildTiming object for a single build, or None.
    builds_timings: List of BuildTiming objects.

  Returns:
    tuple (
      [stage names as strings, sorted by start time.],
      {name: StageTiming from focus_build},
      {name: StageTiming with TimeDeltaStats},
    )
  """
  stage_stats = CalculateStageStats(builds_timings)

  # Map name to StageTiming.
  focus_stages = {}
  if focus_build:
    focus_stages = {s.name: s for s in focus_build.stages}
  stats_stages = {s.name: s for s in stage_stats}

  # Order the stage names to display, sorted by median start time.
  stage_names = list(set(focus_stages.keys() + stats_stages.keys()))
  def name_key(name):
    f, s = focus_stages.get(name), stats_stages.get(name)
    return s.start.median if s and s.start else f.start or datetime.timedelta()
  stage_names.sort(key=name_key)

  return stage_names, focus_stages, stats_stages


def GroupBuildsByMonths(timings):
  """Break a list into distinct months.

  Args:
    timings: A list of BuildTiming objects sorted by start time.

  Returns:
    A list of lists of BuildTiming objects. Each sublist represents one month.
  """
  # Break builds into months.
  all_months = []
  month = 0
  month_timings = []
  for timing in timings:
    if timing.start.month == month:
      month_timings.append(timing)
    else:
      month = timing.start.month
      month_timings = [timing]
      all_months.append(month_timings)

  return all_months


def GroupBuildsByConfig(builds_timings):
  """Turn a list of BuildTiming objects into a map based on config name.

  This is useful when looking at waterfall groups (cq, canary, etc), if you
  want stats per builder in that group.

  Args:
    builds_timings: List of BuildTiming objects to display stats for.

  Returns:
    A dictionary of the form {config_name: [BuildTiming, ...]}
  """
  config_map = {}
  for b in builds_timings:
    config_map.setdefault(b.build_config, []).append(b)

  return config_map


def Percent(numerator, denominator):
  """Convert two integers into a display friendly percentage string.

  Percent(5, 10) -> ' 50%'
  Percent(5, 5) -> '100%'
  Percent(1, 100) -> '  1%'
  Percent(1, 1000) -> '  0%'

  Args:
    numerator: Integer.
    denominator: Integer.

  Returns:
    string formatted result.
  """
  return '%3d%%' % int(100 * (numerator / float(denominator)))


def Report(output, description, focus_build, builds_timings,
           stages=True, trending=False, csv=False):
  """Generate a report describing our stats.

  Args:
    output: A file object to write the report to.
    description: A user friendly string description what the report covers.
    focus_build: A BuildTiming object for a build to compare against stats.
    builds_timings: List of BuildTiming objects to display stats for.
    stages: Include a per-stage break down in the report.
    trending: Display the listed builds as broken down by month.
    csv: Output data in CSV format for spreadsheet import.
  """
  builds_timings.sort(key=lambda b: b.id)

  if csv:
    return ReportCsv(output, description, focus_build, builds_timings,
                     stages=stages)

  output.write('%s\n' % description)

  if builds_timings:
    output.write('Averages for %s Builds: %s - %s\n' %
                 (len(builds_timings),
                  builds_timings[0].id,
                  builds_timings[-1].id))

    success, timeouts, total, build_stats = CalculateBuildStats(builds_timings)
    output.write(' %s success %s timeouts %s %s\n' % (
        focus_build.duration if focus_build else '',
        Percent(success, total),
        Percent(timeouts, total),
        build_stats,
    ))

  if stages:
    output.write('\n')
    stage_names, focus_stages, stats_stages = FindAndSortStageStats(
        focus_build, builds_timings)

    # Display info about each stage.
    for name in stage_names:
      output.write('%s:\n' % name)
      f, s = focus_stages.get(name), stats_stages.get(name)
      output.write('  start:    %s %s\n' % (f.start if f else '',
                                            s.start if s else ''))
      output.write('  duration: %s %s\n' % (f.duration if f else '',
                                            s.duration if s else ''))
      output.write('  finish:   %s %s\n' % (f.finish if f else '',
                                            s.finish if s else ''))

  if trending:
    all_months = GroupBuildsByMonths(builds_timings)
    output.write('\n')

    # Report stats per month.
    for month_timings in all_months:
      success, timeouts, total, build_stats = CalculateBuildStats(month_timings)

      prefix = '%s-%s:' % (month_timings[0].start.year,
                           month_timings[0].start.month)
      output.write('%s success %s timeouts %s %s\n' % (
          prefix.ljust(9),
          Percent(success, total),
          Percent(timeouts, total),
          build_stats,
      ))

      if stages:
        _, _, month_stage_stags = FindAndSortStageStats(None,
                                                        month_timings)

        for name in stage_names:
          s = month_stage_stags.get(name)
          if s:
            output.write('  %s:\n' % name)
            output.write('    start:    %s\n' % (s.start,))
            output.write('    duration: %s\n' % (s.duration,))
            output.write('    finish:   %s\n' % (s.finish,))


def ReportCsv(output, description, focus_build, builds_timings, stages=True):
  """Generate a report describing our stats.

  Args:
    output: A file object to write the report to.
    description: A user friendly string description what the report covers.
    focus_build: A BuildTiming object for a build to compare against stats.
    builds_timings: List of BuildTiming objects to display stats for.
    stages: Include a per-stage break down in the report.
  """
  builds_timings.sort(key=lambda b: b.id)

  stats_header = ['success', 'median', 'mean', 'min', 'max']

  desc_row = [
      description,
      'Averages for %s Builds: %s - %s' % (
          len(builds_timings),
          builds_timings[0].id,
          builds_timings[-1].id,
      )
  ]
  headers = ['']
  subheaders = ['']
  rows = []

  def AddStatsHeaders(name):
    headers.extend([name, '', '', '', ''])
    subheaders.extend(stats_header)

  AddStatsHeaders('Build')

  # Discover full list of stage names.
  stage_names, focus_stats_stages, stats_stages = FindAndSortStageStats(
      focus_build, builds_timings)

  # Focus build row.
  if focus_build:
    row = ['Focus', focus_build.success, focus_build.duration, '', '', '']
    rows.append(row)

    if stages:
      for stage_name in stage_names:
        s = focus_stats_stages.get(stage_name)
        row.extend(['', s.duration, '', '', ''])

  # All builds row.
  successes, _, total, build_stats = CalculateBuildStats(builds_timings)
  row = ['ALL',
         Percent(successes, total),
         build_stats.median, build_stats.mean,
         build_stats.min, build_stats.max]

  if stages:
    for stage_name in stage_names:
      AddStatsHeaders(stage_name)
      s = stats_stages.get(stage_name)
      row.append('')
      row.extend(s.duration)

  rows.append(row)

  # Report stats per month.
  for month_timings in GroupBuildsByMonths(builds_timings):
    successes, _, total, month_stats = CalculateBuildStats(month_timings)
    prefix = '%s-%s' % (month_timings[0].start.year,
                        month_timings[0].start.month)
    row = [prefix,
           Percent(successes, total),
           month_stats.median, month_stats.mean,
           month_stats.min, month_stats.max]

    if stages:
      _, _, month_stage_stags = FindAndSortStageStats(None, month_timings)

      for stage_name in stage_names:
        s = month_stage_stags.get(stage_name)
        if s:
          row.append('')
          row.extend(s.duration)
        else:
          row.extend(('', '', '', '', ''))

    rows.append(row)

  # Sanity check.
  assert len(headers) == len(subheaders)
  for row in rows:
    assert len(row) == len(headers), '%s\n%s' % (row, headers)

  def ColumnsToCsvText(columns):
    return ', '.join(['"%s"' % c for c in columns]) + '\n'

  # Produce final results.
  output.write(ColumnsToCsvText(desc_row))
  output.write(ColumnsToCsvText(headers))
  output.write(ColumnsToCsvText(subheaders))
  for row in rows:
    output.write(ColumnsToCsvText(row))


def StabilityReport(output, description, builds_timings):
  """Generate a report describing general health of our builds.

  Args:
    output: A file object to write the report to.
    description: A user friendly string description what the report covers.
    builds_timings: List of BuildTiming objects to display stats for.
  """
  assert builds_timings

  builds_timings.sort(key=lambda b: b.id, reverse=True)

  output.write('%s\n' % description)

  timeout_map = GroupBuildsByConfig(builds_timings)

  # Get a list of all build_configs present.
  build_configs = timeout_map.keys()

  stats_map = {name: CalculateBuildStats(timeout_map[name])
               for name in build_configs}

  # Sort lowest success rate to the highest.
  # TODO: Make this sorting configurable.
  build_configs.sort(key=lambda n: stats_map[n][0] / float(stats_map[n][2]))

  max_name_len = max(len(name) for name in build_configs)

  for name in build_configs:
    successes, timeouts, total, _ = stats_map[name]

    justified_name = ('%s:' % name).ljust(max_name_len + 1)

    output.write('%s %s successes %s timeouts %d builds.\n' %
                 (justified_name,
                  Percent(successes, total),
                  Percent(timeouts, total),
                  total))

def PerBuildStageStabilityReport(output, description, per_build_stage_stats):
  """Generate a report showing stage failures by builder.

  Args:
    output: A file object to write the report to.
    description: A user friendly string description what the report covers.
    per_build_stage_stats: A map of each builder to a dict of the individual
                           stages.
  """
  assert per_build_stage_stats

  output.write('\n%s\n\n' % description)

  sorted_builders = sorted(per_build_stage_stats)
  for builder in sorted_builders:
    output.write('%s\n' % builder)
    # Calculate the max length stage name for aligning the percentages.
    max_stage_name_len = max(len(name) for name in
                             per_build_stage_stats[builder])

    sorted_stages = sorted(per_build_stage_stats[builder])
    for stage in sorted_stages:
      successes = 0
      valid_stages = 0

      for status in per_build_stage_stats[builder][stage]:
        # We only care about 'pass, fail, or aborted'.  Assuming that 'missing'
        # isn't a failure.
        if status['status'] not in ('pass', 'fail', 'aborted'):
          continue

        valid_stages += 1
        if status['status'] == 'pass':
          successes += 1

      stage_name = ('%s' % stage).ljust(max_stage_name_len + 1)
      # We'll ignore 100% success rates.
      if successes == valid_stages:
        continue
      if valid_stages:
        output.write('  %s %s (%d/%d)\n' % (stage_name,
                                            Percent(successes, valid_stages),
                                            successes, valid_stages))

def SuccessReport(output, description, build_success_rates, stage_success_rates,
                  chrome_uprevs):
  """Generate a report describing the success rate of the builders.

  Report includes success rate by builder and by stage.

  Args:
    output: A file object to write the report to.
    description: A user friendly string description what the report covers.
    build_success_rates: Dictionary of success rates by builder.
    stage_success_rates: Dictionary of success rates by stage.
    chrome_uprevs: Number of chrome uprevs.
  """
  assert build_success_rates

  output.write('\n%s\n\n' % description)

  output.write('\nChrome uprevs: %d\n' % chrome_uprevs)

  stage_keys = stage_success_rates.keys()
  if len(stage_keys):
    output.write('Stages with failures:\n\n')
    stage_keys.sort()
    for key in stage_keys:
      success_rate = stage_success_rates[key]
      if success_rate == '100%':
        continue
      output.write('%s Success rate: %s.\n' % (key, success_rate))
    output.write('\n')

  output.write('Builders:\n\n')
  builder_keys = build_success_rates.keys()
  builder_keys.sort()
  for key in builder_keys:
    if key == 'total':
      continue
    output.write('%s Success rate: %s.\n' % (key, build_success_rates[key]))
  output.write('\nTotal success rate for %s: %s.\n' %
               (description, build_success_rates['total']))


def SuccessReportCsv(output, build_success_rates, stage_success_rates,
                     chrome_uprevs):
  """Generate a CSV report with the success rate of the builders.

  Report includes success rate by builder and by stage.

  Args:
    output: A file object to write the report to.
    build_success_rates: Dictionary of success rates by builder.
    stage_success_rates: Dictionary of success rates by stage.
    chrome_uprevs: Number of chrome uprevs.
  """
  assert build_success_rates

  # Header
  output.write('description,success\n')

  # Chrome uprevs
  output.write('chrome_uprevs,%d\n' % chrome_uprevs)

  # Builder success rates
  output.write('builder:%s,%s\n' % ('total', build_success_rates['total']))
  builder_keys = build_success_rates.keys()
  builder_keys.sort()
  for key in builder_keys:
    if key == 'total':
      continue
    output.write('builder:%s,%s\n' % (key, build_success_rates[key]))

  # Stage success rates
  stage_keys = stage_success_rates.keys()
  if len(stage_keys):
    stage_keys.sort()
    for key in stage_keys:
      output.write('stage:%s,%s\n' % (key, stage_success_rates[key]))
