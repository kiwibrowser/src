# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from benchmarks import loading

from contrib.cluster_telemetry import ct_benchmarks_util
from contrib.cluster_telemetry import page_set

from telemetry.page import traffic_setting


# pylint: disable=protected-access
class LoadingClusterTelemetry(loading._LoadingBase):

  options = {'upload_results': True}

  _ALL_NET_CONFIGS = traffic_setting.NETWORK_CONFIGS.keys()

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    super(LoadingClusterTelemetry, cls).AddBenchmarkCommandLineArgs(parser)
    ct_benchmarks_util.AddBenchmarkCommandLineArgs(parser)
    parser.add_option(
        '--wait-time',  action='store', type='int',
        default=60, help='Number of seconds to wait for after navigation.')
    parser.add_option(
        '--traffic-setting',  choices=cls._ALL_NET_CONFIGS,
        default=traffic_setting.REGULAR_4G,
        help='Traffic condition (string). Default to "%%default". Can be: %s' %
         ', '.join(cls._ALL_NET_CONFIGS))

  def CreateStorySet(self, options):
    def Wait(action_runner):
      action_runner.Wait(options.wait_time)
    return page_set.CTPageSet(
      options.urls_list, options.user_agent, options.archive_data_file,
      traffic_setting=options.traffic_setting,
      run_page_interaction_callback=Wait)

  @classmethod
  def Name(cls):
    return 'loading.cluster_telemetry'
