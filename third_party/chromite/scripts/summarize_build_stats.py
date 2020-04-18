# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to summarize stats for different builds in prod."""

from __future__ import print_function

import datetime
import itertools
import numpy
import re
import sys

from chromite.lib import constants
from chromite.lib import cidb
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.cli.cros import cros_cidbcreds  # TODO: Move into lib???
from chromite.lib import cros_logging as logging


# These are the preferred base URLs we use to canonicalize bugs/CLs.
BUGANIZER_BASE_URL = 'b/'
GUTS_BASE_URL = 't/'
CROS_BUG_BASE_URL = 'crbug.com/'
INTERNAL_CL_BASE_URL = 'crosreview.com/i/'
EXTERNAL_CL_BASE_URL = 'crosreview.com/'
CHROMIUM_CL_BASE_URL = 'codereview.chromium.org/'

class CLStatsEngine(object):
  """Engine to generate stats about CL actions taken by the Commit Queue."""

  def __init__(self, db):
    self.db = db
    self.builds = []
    self.claction_history = None
    self.reasons = {}
    self.blames = {}
    self.summary = {}
    self.builds_by_build_id = {}
    self.slave_builds_by_master_id = {}
    self.slave_builds_by_config = {}

  def GatherBuildAnnotations(self):
    """Gather the failure annotations for builds from cidb."""
    annotations_by_builds = self.db.GetAnnotationsForBuilds(
        [b['id'] for b in self.builds])
    for b in self.builds:
      build_id = b['id']
      build_number = b['build_number']
      annotations = annotations_by_builds.get(build_id, [])
      if not annotations:
        self.reasons[build_number] = ['None']
        self.blames[build_number] = []
      else:
        # TODO(pprabhu) crbug.com/458275
        # We currently squash together multiple annotations into one to ease
        # co-existence with the spreadsheet based logic. Once we've moved off of
        # using the spreadsheet, we should update all uses of the annotations to
        # expect one or more annotations.
        self.reasons[build_number] = [
            a['failure_category'] for a in annotations]
        self.blames[build_number] = []
        for annotation in annotations:
          self.blames[build_number] += self.ProcessBlameString(
              annotation['blame_url'])

  @staticmethod
  def ProcessBlameString(blame_string):
    """Parse a human-created |blame_string| from the spreadsheet.

    Returns:
      A list of canonicalized URLs for bugs or CLs that appear in the blame
      string. Canonicalized form will be 'crbug.com/1234', 'crrev.com/c/1234',
      'crosreview.com/1234', 'b/1234', 't/1234', 'crrev.com/i/1234', or
      'crosreview.com/i/1234' as applicable.
    """
    urls = []
    tokens = blame_string.split()

    # Format to generate the regex patterns. Matches one of provided domain
    # names, followed by lazy wildcard, followed by greedy digit wildcard,
    # followed by optional slash and optional comma and optional (# +
    # alphanum wildcard).
    general_regex = r'^.*(%s).*?([0-9]+)/?,?(#\S*)?$'

    crbug = general_regex % r'crbug.com|bugs.chromium.org'
    internal_review = general_regex % (
        r'crosreview.com/i|chrome-internal-review.googlesource.com|crrev.com/i')
    external_review = general_regex % (
        r'crosreview.com|chromium-review.googlesource.com|crrev.com/c')
    guts = general_regex % r't/|gutsv\d.corp.google.com/#ticket/'
    chromium_review = general_regex % r'codereview.chromium.org'

    # Buganizer regex is different, as buganizer urls do not end with the bug
    # number.
    buganizer = r'^.*(b/|b.corp.google.com/issue\?id=)([0-9]+).*$'

    # Patterns need to be tried in a specific order -- internal review needs
    # to be tried before external review, otherwise urls like crosreview.com/i
    # will be incorrectly parsed as external.
    patterns = [crbug,
                internal_review,
                external_review,
                buganizer,
                guts,
                chromium_review]
    url_patterns = [CROS_BUG_BASE_URL,
                    INTERNAL_CL_BASE_URL,
                    EXTERNAL_CL_BASE_URL,
                    BUGANIZER_BASE_URL,
                    GUTS_BASE_URL,
                    CHROMIUM_CL_BASE_URL]

    for t in tokens:
      for p, u in zip(patterns, url_patterns):
        m = re.match(p, t)
        if m:
          urls.append(u + m.group(2))
          break

    return urls

  def Gather(self, start_date, end_date,
             master_config=constants.CQ_MASTER,
             sort_by_build_number=True):
    """Fetches build data and failure reasons.

    Args:
      start_date: A datetime.date instance for the earliest build to
          examine.
      end_date: A datetime.date instance for the latest build to
          examine.
      master_config: Config name of master to gather data for.
                     Default to CQ_MASTER.
      sort_by_build_number: Optional boolean. If True, builds will be
          sorted by build number.
    """
    logging.info('Gathering data for %s from %s until %s', master_config,
                 start_date, end_date)
    self.builds = self.db.GetBuildHistory(
        master_config,
        start_date=start_date,
        end_date=end_date,
        num_results=self.db.NUM_RESULTS_NO_LIMIT)
    if self.builds:
      logging.info('Fetched %d builds (build_id: %d to %d)', len(self.builds),
                   self.builds[0]['id'], self.builds[-1]['id'])
    else:
      logging.info('Fetched no builds.')
    if sort_by_build_number:
      logging.info('Sorting by build number.')
      self.builds.sort(key=lambda x: x['build_number'])

    logging.info('Gathering cl actions history')
    self.claction_history = self.db.GetActionHistory(start_date, end_date)
    logging.info('Gathering build annotations')
    self.GatherBuildAnnotations()

    self.builds_by_build_id.update(
        {b['id'] : b for b in self.builds})

    # Gather slave statuses for each of the master builds. For now this is a
    # separate query per CQ run, but this could be consolidated to a single
    # query if necessary (requires adding a cidb.py API method).
    for bid in self.builds_by_build_id:
      self.slave_builds_by_master_id[bid] = self.db.GetSlaveStatuses(bid)

    self.slave_builds_by_config = cros_build_lib.GroupByKey(
        itertools.chain(*self.slave_builds_by_master_id.values()),
        'build_config')

  def _PrintCounts(self, reasons, fmt):
    """Print a sorted list of reasons in descending order of frequency.

    Args:
      reasons: A key/value mapping mapping the reason to the count.
      fmt: A format string for our log message, containing %(cnt)d
        and %(reason)s.
    """
    d = reasons
    for cnt, reason in sorted(((v, k) for (k, v) in d.items()), reverse=True):
      logging.info(fmt, dict(cnt=cnt, reason=reason))
    if not d:
      logging.info('  None')

  def FalseRejectionRate(self, good_patch_count, false_rejection_count):
    """Calculate the false rejection ratio.

    This is the chance that a good patch will be rejected by the Pre-CQ or CQ
    in a given run.

    Args:
      good_patch_count: The number of good patches in the run.
      false_rejection_count: A dict containing the number of false rejections
          for the CQ and PRE_CQ.

    Returns:
      A dict containing the false rejection ratios for CQ, PRE_CQ, and combined.
    """
    false_rejection_rate = dict()
    for bot, rejection_count in false_rejection_count.iteritems():
      false_rejection_rate[bot] = (
          rejection_count * 100. / (rejection_count + good_patch_count)
      )
    false_rejection_rate['combined'] = 0
    if good_patch_count:
      rejection_count = sum(false_rejection_count.values())
      false_rejection_rate['combined'] = (
          rejection_count * 100. / (good_patch_count + rejection_count)
      )
    return false_rejection_rate

  def GetBuildRunTimes(self, builds):
    """Gets the elapsed run times of the completed builds within |builds|.

    Args:
      builds: Iterable of build statuses as returned by cidb.

    Returns:
      A list of the elapsed times (in seconds) of the builds that completed.
    """
    times = []
    for b in builds:
      if b['finish_time']:
        td = (b['finish_time'] - b['start_time']).total_seconds()
        times.append(td)
    return times

  def Summarize(self, build_type, bad_patch_candidates=False):
    """Process, print, and return a summary of statistics.

    As a side effect, save summary to self.summary.

    Returns:
      A dictionary summarizing the statistics.
    """
    if build_type == 'cq':
      return self.SummarizeCQ(bad_patch_candidates=bad_patch_candidates)
    else:
      return self.SummarizePFQ()

  def SummarizeCQ(self, bad_patch_candidates=False):
    """Process, print, and return a summary of cl action statistics.

    As a side effect, save summary to self.summary.

    Returns:
      A dictionary summarizing the statistics.
    """
    if self.builds:
      logging.info('%d total runs included, from build %d to %d.',
                   len(self.builds), self.builds[0]['build_number'],
                   self.builds[-1]['build_number'])
      total_passed = len([b for b in self.builds
                          if b['status'] == constants.BUILDER_STATUS_PASSED])
      logging.info('%d of %d runs passed.', total_passed, len(self.builds))
    else:
      logging.info('No runs included.')

    build_times_sec = sorted(self.GetBuildRunTimes(self.builds))

    build_reason_counts = {}
    for reasons in self.reasons.values():
      for reason in reasons:
        if reason != 'None':
          build_reason_counts[reason] = build_reason_counts.get(reason, 0) + 1

    unique_blames = set()
    build_blame_counts = {}
    for blames in self.blames.itervalues():
      unique_blames.update(blames)
      for blame in blames:
        build_blame_counts[blame] = build_blame_counts.get(blame, 0) + 1
    unique_cl_blames = {blame for blame in unique_blames if
                        EXTERNAL_CL_BASE_URL in blame}

    # Shortcuts to some time aggregates about action history.
    patch_handle_times = self.claction_history.GetPatchHandlingTimes().values()
    pre_cq_handle_times = self.claction_history.GetPreCQHandlingTimes().values()
    cq_wait_times = self.claction_history.GetCQWaitingTimes().values()
    cq_handle_times = self.claction_history.GetCQHandlingTimes().values()

    # Calculate how many good patches were falsely rejected and why.
    good_patch_rejections = self.claction_history.GetFalseRejections()
    patch_reason_counts = {}
    patch_blame_counts = {}
    for k, v in good_patch_rejections.iteritems():
      for a in v:
        build = self.builds_by_build_id.get(a.build_id)
        if a.bot_type == constants.CQ and build is not None:
          build_number = build['build_number']
          reasons = self.reasons.get(build_number, ['None'])
          blames = self.blames.get(build_number, ['None'])
          for x in reasons:
            patch_reason_counts[x] = patch_reason_counts.get(x, 0) + 1
          for x in blames:
            patch_blame_counts[x] = patch_blame_counts.get(x, 0) + 1

    good_patch_count = len(self.claction_history.GetSubmittedPatches(False))
    false_rejection_count = {}
    bad_cl_candidates = {}
    for bot_type in [constants.CQ, constants.PRE_CQ]:
      rejections = self.claction_history.GetFalseRejections(bot_type)
      false_rejection_count[bot_type] = sum(map(len,
                                                rejections.values()))

      rejections = self.claction_history.GetTrueRejections(bot_type)
      rejected_cls = set([x.GetChangeTuple() for x in rejections.keys()])
      bad_cl_candidates[bot_type] = self.claction_history.SortBySubmitTimes(
          rejected_cls)

    false_rejection_rate = self.FalseRejectionRate(good_patch_count,
                                                   false_rejection_count)

    # This list counts how many times each good patch was rejected.
    rejection_counts = [0] * (good_patch_count - len(good_patch_rejections))
    rejection_counts += [len(x) for x in good_patch_rejections.values()]

    # Count how many times good patches were exonerated
    total_exonerations = 0
    unique_exonerations = 0
    for good_rejected_patch in good_patch_rejections.keys():
      print('Examining %s' % str(good_rejected_patch))
      actions = self.claction_history.GetCLOrPatchActions(good_rejected_patch)
      actions = [a for a in actions
                 if a.action == constants.CL_ACTION_EXONERATED]
      if actions:
        unique_exonerations += 1
        total_exonerations += len(actions)

    # Break down the frequency of how many times each patch is rejected.
    good_patch_rejection_breakdown = []
    if rejection_counts:
      for x in range(max(rejection_counts) + 1):
        good_patch_rejection_breakdown.append((x, rejection_counts.count(x)))

    # For CQ runs that passed, track which slave was the long pole, i.e. the
    # last to finish.
    long_pole_slave_counts = {}
    for bid, master_build in self.builds_by_build_id.items():
      if master_build['status'] == constants.BUILDER_STATUS_PASSED:
        if not self.slave_builds_by_master_id[bid]:
          continue
        _, long_config = max((slave['finish_time'], slave['build_config'])
                             for slave in self.slave_builds_by_master_id[bid]
                             if slave['finish_time'] and slave['important'])
        long_pole_slave_counts[long_config] = (
            long_pole_slave_counts.get(long_config, 0) + 1)

    # Calc list of slowest slaves and their corresponding build stats
    total_counts = sum(long_pole_slave_counts.values())
    slowest_cq_slaves = []
    for (count, config) in sorted(
        {v: k for k, v in long_pole_slave_counts.items()}.items(),
        reverse=True):
      if count < (total_counts / 20.0):
        continue
      build_times = self.GetBuildRunTimes(self.slave_builds_by_config[config])
      slave = str(config)
      percentile_50 = numpy.percentile(build_times, 50) / 3600.0
      percentile_90 = numpy.percentile(build_times, 90) / 3600.0
      slowest_cq_slaves.append((slave, count, percentile_50, percentile_90))

    summary = {
        'total_cl_actions': len(self.claction_history),
        'total_builds': len(self.builds),
        'first_build_num': self.builds[0]['build_number'],
        'last_build_num': self.builds[-1]['build_number'],
        'last_build_id': self.builds[-1]['id'],
        'unique_cls': len(self.claction_history.affected_cls),
        'unique_patches': len(self.claction_history.affected_patches),
        'submitted_patches': len(self.claction_history.submit_actions),
        'rejections': len(self.claction_history.reject_actions),
        'submit_fails': len(self.claction_history.submit_fail_actions),
        'good_patch_rejections': sum(rejection_counts),
        'mean_good_patch_rejections': numpy.mean(rejection_counts),
        'good_patch_rejection_breakdown': good_patch_rejection_breakdown,
        'good_patch_rejection_count': false_rejection_count,
        'good_patch_rejections_50': numpy.percentile(rejection_counts, 50),
        'good_patch_rejections_90': numpy.percentile(rejection_counts, 90),
        'good_patch_rejections_95': numpy.percentile(rejection_counts, 95),
        'good_patch_rejections_99': numpy.percentile(rejection_counts, 99),
        'total_exonerations': total_exonerations,
        'unique_exonerations': unique_exonerations,
        'false_rejection_rate': false_rejection_rate,
        'median_handling_time': numpy.median(patch_handle_times),
        'patch_handling_time': patch_handle_times,
        'cl_handling_time_50': numpy.percentile(patch_handle_times, 50)/3600.0,
        'cl_handling_time_90': numpy.percentile(patch_handle_times, 90)/3600.0,
        'cq_time_50': numpy.percentile(cq_handle_times, 50) / 3600.0,
        'cq_time_90': numpy.percentile(cq_handle_times, 90) / 3600.0,
        'wait_time_50': numpy.percentile(cq_wait_times, 50) / 3600.0,
        'wait_time_90': numpy.percentile(cq_wait_times, 90) / 3600.0,
        'cq_run_time_50': numpy.percentile(build_times_sec, 50) / 3600.0,
        'cq_run_time_90': numpy.percentile(build_times_sec, 90) / 3600.0,
        'bad_cl_candidates': bad_cl_candidates,
        'unique_blames_change_count': len(unique_cl_blames),
        'long_pole_slave_counts': long_pole_slave_counts,
        'slowest_cq_slaves': slowest_cq_slaves,
        'patch_flake_rejections': len(good_patch_rejections),
        'bad_cl_precq_rejected': len(bad_cl_candidates['pre-cq']),
        'false_rejection_total': sum(false_rejection_count.values()),
        'false_rejection_pre_cq': false_rejection_count[constants.PRE_CQ],
        'false_rejection_cq': false_rejection_count[constants.CQ],
        'build_blame_counts': build_blame_counts,
        'patch_blame_counts': patch_blame_counts,
    }

    s = summary

    logging.info('CQ committed %s changes', s['submitted_patches'])
    logging.info('CQ correctly rejected %s unique changes',
                 summary['unique_blames_change_count'])
    logging.info('pre-CQ and CQ incorrectly rejected %s changes a total of '
                 '%s times (pre-CQ: %s; CQ: %s)',
                 s['patch_flake_rejections'],
                 s['false_rejection_total'],
                 s['false_rejection_pre_cq'],
                 s['false_rejection_cq'])

    logging.info('      Total CL actions: %d.', s['total_cl_actions'])
    logging.info('    Unique CLs touched: %d.', s['unique_cls'])
    logging.info('Unique patches touched: %d.', s['unique_patches'])
    logging.info('   Total CLs submitted: %d.', s['submitted_patches'])
    logging.info('      Total rejections: %d.', s['rejections'])
    logging.info(' Total submit failures: %d.', s['submit_fails'])
    logging.info(' Good patches rejected: %d.',
                 s['patch_flake_rejections'])
    logging.info('   Mean rejections per')
    logging.info('            good patch: %.2f',
                 s['mean_good_patch_rejections'])
    logging.info('Good patch rejections')
    logging.info('                 50ile: %d', s['good_patch_rejections_50'])
    logging.info('                 90ile: %d', s['good_patch_rejections_90'])
    logging.info('                 95ile: %d', s['good_patch_rejections_95'])
    logging.info('                 99ile: %d', s['good_patch_rejections_99'])
    logging.info(' False rejection rate for CQ: %.1f%%',
                 s['false_rejection_rate'].get(constants.CQ, 0))
    logging.info(' False rejection rate for Pre-CQ: %.1f%%',
                 s['false_rejection_rate'].get(constants.PRE_CQ, 0))
    logging.info(' Combined false rejection rate: %.1f%%',
                 s['false_rejection_rate']['combined'])

    for x, p in s['good_patch_rejection_breakdown']:
      logging.info('%d good patches were rejected %d times.', p, x)
    logging.info('')
    logging.info('Good patch handling time:')
    logging.info('  10th percentile: %.2f hours',
                 numpy.percentile(patch_handle_times, 10) / 3600.0)
    logging.info('  25th percentile: %.2f hours',
                 numpy.percentile(patch_handle_times, 25) / 3600.0)
    logging.info('  50th percentile: %.2f hours',
                 s['median_handling_time'] / 3600.0)
    logging.info('  75th percentile: %.2f hours',
                 numpy.percentile(patch_handle_times, 75) / 3600.0)
    logging.info('  90th percentile: %.2f hours',
                 numpy.percentile(patch_handle_times, 90) / 3600.0)
    logging.info('')
    logging.info('Time spent in Pre-CQ:')
    logging.info('  10th percentile: %.2f hours',
                 numpy.percentile(pre_cq_handle_times, 10) / 3600.0)
    logging.info('  25th percentile: %.2f hours',
                 numpy.percentile(pre_cq_handle_times, 25) / 3600.0)
    logging.info('  50th percentile: %.2f hours',
                 numpy.percentile(pre_cq_handle_times, 50) / 3600.0)
    logging.info('  75th percentile: %.2f hours',
                 numpy.percentile(pre_cq_handle_times, 75) / 3600.0)
    logging.info('  90th percentile: %.2f hours',
                 numpy.percentile(pre_cq_handle_times, 90) / 3600.0)
    logging.info('')
    logging.info('Time spent waiting for CQ:')
    logging.info('  10th percentile: %.2f hours',
                 numpy.percentile(cq_wait_times, 10) / 3600.0)
    logging.info('  25th percentile: %.2f hours',
                 numpy.percentile(cq_wait_times, 25) / 3600.0)
    logging.info('  50th percentile: %.2f hours',
                 numpy.percentile(cq_wait_times, 50) / 3600.0)
    logging.info('  75th percentile: %.2f hours',
                 numpy.percentile(cq_wait_times, 75) / 3600.0)
    logging.info('  90th percentile: %.2f hours',
                 numpy.percentile(cq_wait_times, 90) / 3600.0)
    logging.info('')
    logging.info('Time spent in CQ:')
    logging.info('  10th percentile: %.2f hours',
                 numpy.percentile(cq_handle_times, 10) / 3600.0)
    logging.info('  25th percentile: %.2f hours',
                 numpy.percentile(cq_handle_times, 25) / 3600.0)
    logging.info('  50th percentile: %.2f hours',
                 numpy.percentile(cq_handle_times, 50) / 3600.0)
    logging.info('  75th percentile: %.2f hours',
                 numpy.percentile(cq_handle_times, 75) / 3600.0)
    logging.info('  90th percentile: %.2f hours',
                 numpy.percentile(cq_handle_times, 90) / 3600.0)
    logging.info('')

    # Log some statistics about cq-master run-time.
    logging.info('CQ-master run time:')
    logging.info('  50th percentile: %.2f hours',
                 numpy.percentile(build_times_sec, 50) / 3600.0)
    logging.info('  90th percenfile: %.2f hours',
                 numpy.percentile(build_times_sec, 90) / 3600.0)

    for bot_type, patches in summary['bad_cl_candidates'].items():
      logging.info('%d bad patch candidates were rejected by the %s',
                   len(patches), bot_type)
      if bad_patch_candidates:
        for k in patches:
          logging.info('Bad patch candidate in: %s', k)

    fmt_fai = '  %(cnt)d failures in %(reason)s'
    fmt_rej = '  %(cnt)d rejections due to %(reason)s'

    logging.info('Reasons why good patches were rejected:')
    self._PrintCounts(patch_reason_counts, fmt_rej)

    logging.info('Bugs or CLs responsible for good patches rejections:')
    self._PrintCounts(patch_blame_counts, fmt_rej)

    logging.info('Reasons why builds failed:')
    self._PrintCounts(build_reason_counts, fmt_fai)

    logging.info('Bugs or CLs responsible for build failures:')
    self._PrintCounts(build_blame_counts, fmt_fai)

    logging.info('Slowest CQ slaves out of %s passing runs:', total_counts)

    for slave, count, per_50, per_90 in summary['slowest_cq_slaves']:
      logging.info('%s times the slowest slave was %s', count, slave)
      logging.info('  50th percentile: %.2f hours, 90th percentile: %.2f hours',
                   per_50,
                   per_90)
    return summary

  # TODO(akeshet): some of this logic is copied directly from SummarizeCQ.
  # Refactor to reuse that code instead.
  def SummarizePFQ(self):
    """Process, print, and return a summary of pfq bug and failure statistics.

    As a side effect, save summary to self.summary.

    Returns:
      A dictionary summarizing the statistics.
    """
    if self.builds:
      logging.info('%d total runs included, from build %d to %d.',
                   len(self.builds), self.builds[0]['build_number'],
                   self.builds[-1]['build_number'])
      total_passed = len([b for b in self.builds
                          if b['status'] == constants.BUILDER_STATUS_PASSED])
      logging.info('%d of %d runs passed.', total_passed, len(self.builds))
    else:
      logging.info('No runs included.')

    # TODO(akeshet): This is the end of the verbatim copied code.

    # Count the number of times each particular (canonicalized) blame url was
    # given.
    unique_blame_counts = {}
    for blames in self.blames.itervalues():
      for b in blames:
        unique_blame_counts[b] = unique_blame_counts.get(b, 0) + 1

    top_blames = sorted([(count, blame) for
                         blame, count in unique_blame_counts.iteritems()],
                        reverse=True)
    logging.info('Top blamed issues:')
    if top_blames:
      for tb in top_blames:
        logging.info('   %s x %s', tb[0], tb[1])
    else:
      logging.info('None!')

    return {}

ReportHTMLTemplate = """
<head>
<style>
  body {{font-family: "sans serif"}}
  table {{border-collapse: collapse; width: 50%}}
  table, td, th {{border: 1px solid black}}
  th, td {{padding: 5px}}
  td {{text-align: left}}
  replace {{background-color: red; font-style: bold}}
  badcl {{background-color: darkgreen; color: white}}
  bugtot {{background-color: darkslateblue; color:white}}
  infra {{background-color: darkred; color:white}}
  #note {{background-color: green; font-style: italic}}
</style>
<title>Summary of CQ Performance for the past week</title>
</head>

<body>
<p>&nbsp;</p>
<p id="note">
Instruction text in italic green. Places to replace are in <replace>bold red</replace>.<br>

<ol id="note">
  <li>Copy-paste this page into an email</li>
  <li>Set subject line to "Summary of CQ performance for the past week"</li>
  <li>Follow instructions in italic green</li>
  <li>Delete any green text</li>
  <li>Insure that all <replace>REPLACE</replace> text is replaced</li>
  <li>Email to <a href="mailto:chromeos-infra-discuss@google.com">chromeos-infra-discuss@google.com</a> and <a href="mailto:infra-product-taskforce@google.com">infra-product-taskforce@google.com</a>.
</ol>
</p>

<p>
<b>{total_builds}</b> CQ runs included, from build <b>{first_build_num}</b> to <b>{last_build_num}</b>.
</p>

<p>
The CQ <b>committed {submitted_patches} changes</b> this week.<br>
The CQ <b>correctly rejected {unique_blames_change_count} unique changes</b> this week, which would otherwise have broken the tree and required investigation and revert.<br>
</p>

<p>
The pre-CQ <b>rejected {bad_cl_precq_rejected} changes this week</b>, which would otherwise have broken a CQ run and affected other developers.<br>
</p>

<h2>CL handling times</h2>
<p>
See also <a href="http://shortn/_cN4FAtq4sP">CQ Weekly Summary Dashboard</a>.<br>
The CL handling time was <b>{cl_handling_time_50:.2f} hours</b> 50%ile <b>{cl_handling_time_90:.2f} hours</b> 90%ile <a href="http://shortn/_DfXfGnzqow">history</a>.<br>
Time spent in the CQ was <b>{cq_time_50:.2f} hours</b> 50%ile <b>{cq_time_90:.2f} hours</b> 90%ile <a href="http://shortn/_2yYTGswEKh">history</a>.<br>
Time spent waiting was <b>{wait_time_50:.2f} hours</b> 50%ile <b>{wait_time_90:.2f} hours</b> 90%ile <a href="http://shortn/_e9vKXdSOjH">history</a>.<br>
CQ run time was <b>{cq_run_time_50:.2f} hours</b> 50%ile <b>{cq_run_time_90:.2f} hours</b> 90%ile <a href="http://shortn/_STokpvKTaS">history</a>.<br>
Pre-cq times available on <a href="http://shortn/_ZP8ivoYPUZ">monarch history</a>.<br>
Wall-clock times times available on <a href="http://shortn/_qXixBiSDUA">monarch history</a>.<br>
</p>

<h2>Slowest Passing Slaves</h2>
The slowest passing slaves (accounting for CQ self-destruct, relevance detection, and history-aware-submit logic) are available as a <A href="http://shortn/_RBQNer8DDk">monarch history</A>.


<h2>False rejections and Exonerations thereof</h2>
<p>
The pre-CQ + CQ <b>incorrectly rejected [{patch_flake_rejections}] unique changes a total of [{false_rejection_total}] times</b> this week. (Pre-CQ:[{false_rejection_pre_cq}]; CQ: [{false_rejection_cq}])<br>
The probability of a good patch being incorrectly rejected by the CQ or Pre-CQ is [{false_rejection_rate[combined]:.2f}]%. (Pre-CQ:[{false_rejection_rate[pre-cq]:.2f}]%; CQ: [{false_rejection_rate[cq]:.2f}])<br>
The CL exonerator retried <b>[{unique_exonerations}] unique good changes a total of [{total_exonerations}] times this week</b> saving the need for the developer to CQ+1.


</p>

<h2>Which issues or CLs caused the most false rejections this week?</h2>
<i>Note: test flake may be caused by flake in the test itself, or flake in the product.</i><br>
<p id="note">At your discretion, pick the top few which caused the most failures</p>
<ul>
{false_rejections_html}
</ul>

<h2>Which issues or CLs caused the most CQ run failures this week?</h2>
<p id="note">At your discretion, pull the top blamed CLs from the summarize_build_stats output. When appropriate, summarize what action we plan to take to block these changes in the pre-CQ.</p>
<ul>
{build_failures_html}
</ul>

<i>Generated on {datetime}</i>
</body>
"""

def GenerateReport(file_out, summary):
  """Write formated summary to file_out"""

  report = summary.copy()
  report['datetime'] = str(datetime.datetime.now())

  sorted_blame_counts = sorted([(v, k) for (k, v) in
                                summary['patch_blame_counts'].iteritems()],
                               reverse=True)

  category_pick = ('_<replace>Pick a category</replace>_ <badcl>Bad CL</badcl>'
                   ' <bugtot>Product</bugtot> <infra>Infra</infra>')

  false_rejections = [{'id': blame, 'rejections': rejs}
                      for rejs, blame in sorted_blame_counts]
  flake_fmt = ('  <li><a href="http://{id}">{id}</a> (<b>{rejections} </b> '
               'false rejections): %s _<replace>Brief explanation of bug. If '
               'fixed, or describe workarounds</replace>_</li>' % category_pick)
  report['false_rejections_html'] = '\n'.join([flake_fmt.format(**x)
                                               for x in false_rejections])

  sorted_fails = sorted([(v, k) for (k, v) in
                         summary['build_blame_counts'].iteritems()],
                        reverse=True)
  build_fails = [{'id': blame, 'fails': fails}
                 for fails, blame in sorted_fails]
  cl_flake_fmt = ('  <li><a href="http://{id}">{id}</a> '
                  '(<b>{fails}</b> build failures): %s '
                  '_<replace>explanation</replace>_</li>' % category_pick)
  report['build_failures_html'] = '\n'.join([cl_flake_fmt.format(**x)
                                             for x in build_fails])

  file_out.write(ReportHTMLTemplate.format(**report))


def _CheckOptions(options):
  # Ensure that specified start date is in the past.
  now = datetime.datetime.now()
  if options.start_date and now.date() < options.start_date:
    logging.error('Specified start date is in the future: %s',
                  options.start_date)
    return False

  return True


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)

  ex_group = parser.add_mutually_exclusive_group(required=True)
  ex_group.add_argument('--start-date', action='store', type='date',
                        default=None,
                        help='Limit scope to a start date in the past.')
  ex_group.add_argument('--past-month', action='store_true', default=False,
                        help='Limit scope to the past 30 days up to now.')
  ex_group.add_argument('--past-week', action='store_true', default=False,
                        help='Limit scope to the past week up to now.')
  ex_group.add_argument('--past-day', action='store_true', default=False,
                        help='Limit scope to the past day up to now.')

  parser.add_argument('--cred-dir', action='store',
                      metavar='CIDB_CREDENTIALS_DIR',
                      help='Database credentials directory with certificates '
                           'and other connection information. Obtain your '
                           'credentials at go/cros-cidb-admin .')
  parser.add_argument('--starting-build', action='store', type=int,
                      default=None, help='Filter to builds after given number'
                                         '(inclusive).')
  parser.add_argument('--ending-build', action='store', type=int,
                      default=None, help='Builder to builds before a given '
                                         'number (inclusive).')
  parser.add_argument('--end-date', action='store', type='date', default=None,
                      help='Limit scope to an end date in the past.')

  parser.add_argument('--build-type', choices=['cq', 'chrome-pfq'],
                      default='cq',
                      help='Build type to summarize. Default: cq.')
  parser.add_argument('--bad-patch-candidates', action='store_true',
                      default=False,
                      help='In CQ mode, whether to print bad patch '
                           'candidates.')
  parser.add_argument('--report-file', action='store',
                      help='Write HTML formatted report to given file')
  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  if not _CheckOptions(options):
    sys.exit(1)

  credentials = options.cred_dir
  if not credentials:
    credentials = cros_cidbcreds.CheckAndGetCIDBCreds()

  db = cidb.CIDBConnection(credentials)

  if options.end_date:
    end_date = options.end_date
  else:
    end_date = datetime.datetime.now().date()

  # Determine the start date to use, which is required.
  if options.start_date:
    start_date = options.start_date
  else:
    assert options.past_month or options.past_week or options.past_day
    # Database search results will include both the starting and ending
    # days.  So, the number of days to subtract is one less than the
    # length of the search period.
    #
    # For instance, the starting day for the week ending 2014-04-21
    # should be 2017-04-15 (date - 6 days).
    if options.past_month:
      start_date = end_date - datetime.timedelta(days=29)
    elif options.past_week:
      start_date = end_date - datetime.timedelta(days=6)
    else:
      start_date = end_date

  if options.build_type == 'cq':
    master_config = constants.CQ_MASTER
  else:
    master_config = constants.PFQ_MASTER

  cl_stats_engine = CLStatsEngine(db)
  cl_stats_engine.Gather(start_date, end_date, master_config)
  summary = cl_stats_engine.Summarize(options.build_type,
                                      options.bad_patch_candidates)

  if options.report_file:
    with open(options.report_file, "w") as f:
      logging.info("Writing report to %s", options.report_file)
      GenerateReport(f, summary)
