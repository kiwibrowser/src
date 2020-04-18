# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Send system monitoring data to the timeseries monitoring API."""

from __future__ import absolute_import
from __future__ import print_function

import time

from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import metrics
from chromite.lib import ts_mon_config
from infra_libs.ts_mon.common import interface

from chromite.scripts.sysmon import git_metrics
from chromite.scripts.sysmon import loop
from chromite.scripts.sysmon import net_metrics
from chromite.scripts.sysmon import osinfo_metrics
from chromite.scripts.sysmon import proc_metrics
from chromite.scripts.sysmon import prod_metrics
from chromite.scripts.sysmon import puppet_metrics
from chromite.scripts.sysmon import system_metrics

logger = logging.getLogger(__name__)


class _MetricCollector(object):
  """Metric collector class."""

  def __init__(self, collect_prod_hosts=False):
    self._collect_osinfo = _TimedCallback(
        callback=osinfo_metrics.collect_os_info,
        interval=60 * 60)
    if collect_prod_hosts:
      logger.info(u'Enabling prod host metric collection.')
      self._collect_prod_hosts = _TimedCallback(
          callback=prod_metrics.collect_prod_hosts,
          interval=10 * 60)
    else:
      self._collect_prod_hosts = lambda: None

  def __call__(self):
    """Collect metrics."""
    system_metrics.collect_uptime()
    system_metrics.collect_cpu_info()
    system_metrics.collect_disk_info()
    system_metrics.collect_mem_info()
    net_metrics.collect_net_info()
    proc_metrics.collect_proc_info()
    system_metrics.collect_load_avg()
    puppet_metrics.collect_puppet_summary()
    git_metrics.collect_git_metrics()
    self._collect_prod_hosts()
    self._collect_osinfo()
    system_metrics.collect_unix_time()  # must be just before flush
    metrics.Flush()


class _TimedCallback(object):
  """Limits callback to one call in a given interval."""

  def __init__(self, callback, interval):
    """Initialize instance.

    Args:
      callback: function to call
      interval: Number of seconds between allowed calls
    """
    self._callback = callback
    self._interval = interval
    self._last_called = float('-inf')

  def __call__(self):
    if time.time() >= self._next_call:
      self._callback()
      self._last_called = time.time()

  @property
  def _next_call(self):
    return self._last_called + self._interval


def main():
  parser = commandline.ArgumentParser(
      description=__doc__,
      default_log_level='DEBUG')
  parser.add_argument(
      '--interval',
      default=60,
      type=int,
      help='time (in seconds) between sampling system metrics')
  parser.add_argument(
      '--collect-prod-hosts',
      action='store_true',
      help='[DEPRECATED. Use --collect-host-manifest instead.] '
           'Enable collection of prod host metrics, like roles')
  parser.add_argument(
      '--collect-host-manifest',
      default=None,
      choices=['prod', 'staging'],
      help='Enable collection of server metrics (e.g. roles) for servers in '
           'the given lab environment.')
  opts = parser.parse_args()
  opts.Freeze()

  # This call returns a context manager that doesn't do anything, so we
  # ignore the return value.
  ts_mon_config.SetupTsMonGlobalState('sysmon', auto_flush=False)
  # The default prefix is '/chrome/infra/'.
  interface.state.metric_name_prefix = (interface.state.metric_name_prefix
                                        + 'chromeos/sysmon/')

  # Transitional, while we migrate users off of |collect_prod_hosts|
  if opts.collect_host_manifest is not None:
    opts.collect_prod_hosts = True
  collector = _MetricCollector(collect_prod_hosts=opts.collect_prod_hosts)
  loop.SleepLoop(callback=collector,
                 interval=opts.interval).loop_forever()
