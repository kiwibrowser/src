# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Send swarming-proxy latency monitoring data to ts_mon."""

from __future__ import absolute_import
from __future__ import print_function


import sys
import time

from chromite.cbuildbot import commands
from chromite.cbuildbot import swarming_lib
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import metrics
from chromite.lib import timeout_util
from chromite.lib import ts_mon_config

def main(argv):
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('swarming_server', action='store',
                      help='Swarming server to send no-op requests to.')
  options = parser.parse_args(argv)

  m_timer = 'chromeos/autotest/swarming_proxy/no_op_durations'
  m_count = 'chromeos/autotest/swarming_proxy/no_op_attempts'
  command = commands.RUN_SUITE_PATH
  fields = {'success': False, 'swarming_server': options.swarming_server}
  with ts_mon_config.SetupTsMonGlobalState('swarm_mon', indirect=True):
    while True:
      with metrics.SecondsTimer(m_timer, fields=fields) as f:
        try:
          with metrics.SuccessCounter(m_count):
            swarming_lib.RunSwarmingCommand([command, '--do_nothing'],
                                            options.swarming_server,
                                            dimensions=[('pool', 'default')],
                                            timeout_secs=120)
          f['success'] = True
        except (cros_build_lib.RunCommandError, timeout_util.TimeoutError):
          pass
      time.sleep(60)


if __name__ == '__main__':
  main(sys.argv)
