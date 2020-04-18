# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for recording CL's action metrics."""

from __future__ import print_function

from infra_libs import ts_mon

from chromite.lib import clactions
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import metrics


def RecordSubmissionMetrics(action_history, submitted_change_strategies):
  """Record submission metrics in monarch.

  Args:
    action_history: A CLActionHistory instance for all cl actions for all
        changes in submitted_change_strategies.
    submitted_change_strategies: A dictionary from GerritPatchTuples to
        submission strategy strings. These changes will have their handling
        times recorded in monarch.
  """
  # TODO(phobbs) move to top level after crbug.com/755415
  handling_time_metric = metrics.CumulativeSecondsDistribution(
      constants.MON_CL_HANDLE_TIME)
  wall_clock_time_metric = metrics.CumulativeSecondsDistribution(
      constants.MON_CL_WALL_CLOCK_TIME)
  precq_time_metric = metrics.CumulativeSecondsDistribution(
      constants.MON_CL_PRECQ_TIME)
  wait_time_metric = metrics.CumulativeSecondsDistribution(
      constants.MON_CL_WAIT_TIME)
  cq_run_time_metric = metrics.CumulativeSecondsDistribution(
      constants.MON_CL_CQRUN_TIME)
  cq_tries_metric = metrics.CumulativeSmallIntegerDistribution(
      constants.MON_CL_CQ_TRIES)

  # These 3 false rejection metrics are different in subtle but important ways.

  # false_rejections: distribution of the number of times a CL was rejected,
  # broken down by what it was rejected by (cq vs. pre-cq). Every CL will emit
  # two data points to this distribution.
  false_rejection_metric = metrics.CumulativeSmallIntegerDistribution(
      constants.MON_CL_FALSE_REJ)

  # false_rejections_total: distribution of the total number of times a CL
  # was rejected (not broken down by phase). Note that there is no way to
  # independently calculate this from |false_rejections| distribution above,
  # since one cannot reconstruct after the fact which pre-cq and cq data points
  # (for the same underlying CL) belong together.
  false_rejection_total_metric = metrics.CumulativeSmallIntegerDistribution(
      constants.MON_CL_FALSE_REJ_TOTAL)

  # false_rejection_count: counter of the total number of false rejections that
  # have occurred (broken down by phase)
  false_rejection_count_metric = metrics.Counter(
      constants.MON_CL_FALSE_REJ_COUNT)

  # This metric excludes rejections which were forgiven by the CL-exonerator
  # service.
  false_rejections_minus_exonerations_metric = \
      metrics.CumulativeSmallIntegerDistribution(
          constants.MON_CQ_FALSE_REJ_MINUS_EXONERATIONS,
          description='The number of rejections - exonerations per CL.',
          field_spec=[ts_mon.StringField('submission_strategy')],
      )

  precq_false_rejections = action_history.GetFalseRejections(
      bot_type=constants.PRE_CQ)
  cq_false_rejections = action_history.GetFalseRejections(
      bot_type=constants.CQ)
  exonerations = action_history.GetExonerations()

  for change, strategy in submitted_change_strategies.iteritems():
    strategy = strategy or ''
    handling_time = clactions.GetCLHandlingTime(change, action_history)
    wall_clock_time = clactions.GetCLWallClockTime(change, action_history)
    precq_time = clactions.GetPreCQTime(change, action_history)
    wait_time = clactions.GetCQWaitTime(change, action_history)
    run_time = clactions.GetCQRunTime(change, action_history)
    pickups = clactions.GetCQAttemptsCount(change, action_history)

    fields = {'submission_strategy': strategy}

    handling_time_metric.add(handling_time, fields=fields)
    wall_clock_time_metric.add(wall_clock_time, fields=fields)
    precq_time_metric.add(precq_time, fields=fields)
    wait_time_metric.add(wait_time, fields=fields)
    cq_run_time_metric.add(run_time, fields=fields)
    cq_tries_metric.add(pickups, fields=fields)

    rejection_types = (
        (constants.PRE_CQ, precq_false_rejections),
        (constants.CQ, cq_false_rejections),
    )

    total_rejections = 0
    for by, rej in rejection_types:
      c = len(rej.get(change, []))
      f = dict(fields, rejected_by=by)
      false_rejection_metric.add(c, fields=f)
      false_rejection_count_metric.increment_by(c, fields=f)
      total_rejections += c

    false_rejection_total_metric.add(total_rejections, fields=fields)
    n_exonerations = len(exonerations.get(change, []))
    # TODO(crbug.com/804900) max(0, ...) is required because of an accounting
    # bug where sometimes this quantity is negative.
    net_rejections = total_rejections - n_exonerations
    if net_rejections < 0:
      logging.error(
          'Exonerations is larger than total rejections for CL %s.'
          ' See crbug.com/804900', change)
    false_rejections_minus_exonerations_metric.add(
        max(0, net_rejections),
        fields=fields)
