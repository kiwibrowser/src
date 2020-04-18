#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Custom swarming triggering script.

This script does custom swarming triggering logic, to allow one bot to
conceptually span multiple Swarming configurations, while lumping all trigger
calls under one logical step.

The reason this script is needed is to allow seamless upgrades of the GPU, OS
version, or graphics driver. Most Chromium tests, GPU tests in particular, are
triggered with precise values for all of these Swarming dimensions. This ensures
that if a machine is added to the Swarming pool with a slightly different
configuration, tests don't fail for unexpected reasons.

During an upgrade of the fleet, it's not feasible to take half of the machines
offline. Some experience was gained with this during a recent upgrade of the
GPUs in Chromium's main Windows and Linux NVIDIA bots. In the middle of the
upgrade, only 50% of the capacity was available, and CQ jobs started to time
out. Once the hurdle had been passed in the middle of the upgrade, capacity was
sufficient, but it's crucial that this process remain seamless.

This script receives multiple machine configurations on the command line in the
form of quoted strings. These strings are JSON dictionaries that represent
entries in the "dimensions" array of the "swarming" dictionary in the
src/testing/buildbot JSON files. The script queries the Swarming pool for the
number of machines of each configuration, and distributes work (shards) among
them using the following algorithm:

1. If either configuration has machines available (online, not busy at the time
of the query) then distribute shards to them first.

2. Compute the relative fractions of all of the live (online, not quarantined,
not dead) machines of all configurations.

3. Distribute the remaining shards probabilistically among these configurations.

The use of random numbers attempts to avoid the pathology where one
configuration only has a couple of machines, and work is never distributed to it
once all machines are busy.

This script must have roughly the same command line interface as swarming.py
trigger. It modifies it in the following ways:
 * Intercepts the dump-json argument, and creates its own by combining the
   results from each trigger call.
 * Scans through the multiple-trigger-configs dictionaries. For any key found,
   deletes that dimension from the originally triggered task's dimensions. This
   is what allows the Swarming dimensions to be replaced.
 * On a per-shard basis, adds the Swarming dimensions chosen from the
   multiple-trigger-configs list to the dimensions for the shard.

This script is normally called from the swarming recipe module in tools/build.

"""

import argparse
import copy
import json
import os
import random
import subprocess
import sys
import tempfile
import urllib

import base_test_triggerer


class MultiDimensionTestTriggerer(base_test_triggerer.BaseTestTriggerer):
  def __init__(self):
    super(MultiDimensionTestTriggerer, self).__init__()

  def choose_random_int(self, max_num):
    return random.randint(1, max_num)

  def pick_bot_configuration(self, verbose):
    # These are the rules used:
    # 1. If any configuration has bots available, pick the configuration with
    #    the most bots available.
    # 2. If no configuration has bots available, pick a random configuration
    #    based on the total number of bots in each configuration.
    #
    # This method updates bot_statuses_ in case (1), and in both cases, returns
    # the index into bot_configs_ that should be used.
    if any(status['available'] > 0 for status in self._bot_statuses):
      # Case 1.
      max_index = 0
      max_val = self._bot_statuses[0]['available']
      for i in xrange(1, len(self._bot_statuses)):
        avail = self._bot_statuses[i]['available']
        if avail > max_val:
          max_index = i
          max_val = avail
      self._bot_statuses[max_index]['available'] -= 1
      assert self._bot_statuses[max_index]['available'] >= 0
      if verbose:
        print 'Chose bot config %d because bots were available' % (max_index)
      return max_index
    # Case 2.
    # We want to choose a bot uniformly at random from all of the bots specified
    # in the bot configs. To do this, we conceptually group the bots into
    # buckets, pick a random number between 1 and the total number of bots, and
    # figure out which bucket of bots it landed in.
    r = self.choose_random_int(self._total_bots)
    for i, status in enumerate(self._bot_statuses):
      if r <= status['total']:
        if verbose:
          print 'Chose bot config %d stochastically' % (i)
        return i
      r -= status['total']
    raise Exception('Should not reach here')

  def select_config_indices(self, args, verbose):
    selected_indices = []
    for _ in xrange(args.shards):
      selected_indices.append(self.pick_bot_configuration(verbose))
    return selected_indices

  def prune_test_specific_configs(self, args, verbose):
    self.query_swarming_for_bot_configs(verbose)

def main():
  # setup args for common contract of base class
  parser = base_test_triggerer.BaseTestTriggerer.setup_parser_contract(
      argparse.ArgumentParser(description=__doc__))
  args, remaining = parser.parse_known_args()

  triggerer =  MultiDimensionTestTriggerer()
  return triggerer.trigger_tasks(args, remaining)


if __name__ == '__main__':
  sys.exit(main())
