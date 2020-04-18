#!/usr/bin/env vpython
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for creating a phased orderfile.

This kind of orderfile is based on cygprofile lightweight instrumentation. The
profile dump format is described in process_profiles.py. These tools assume
profiling has been done with two phases.

The first phase, labeled 0 in the filename, is called "startup" and the second,
labeled 1, is called "interaction". These two phases are used to create an
orderfile with three parts: the code touched only in startup, the code
touched only during interaction, and code common to the two phases. We refer to
these parts as the orderfile phases.
"""

import argparse
import collections
import glob
import itertools
import logging
import os.path

import process_profiles


# Files matched when using this script to analyze directly (see main()).
PROFILE_GLOB = 'cygprofile-*.txt_*'


OrderfilePhaseOffsets = collections.namedtuple(
    'OrderfilePhaseOffsets', ('startup', 'common', 'interaction'))


class PhasedAnalyzer(object):
  """A class which collects analysis around phased orderfiles.

  It maintains common data such as symbol table information to make analysis
  more convenient.
  """
  # These figures are taken from running memory and speedometer telemetry
  # benchmarks, and are still subject to change as of 2018-01-24.
  STARTUP_STABILITY_THRESHOLD = 1.5
  COMMON_STABILITY_THRESHOLD = 1.75
  INTERACTION_STABILITY_THRESHOLD = 2.5

  def __init__(self, profiles, processor):
    """Intialize.

    Args:
      profiles (ProfileManager) Manager of the profile dump files.
      processor (SymbolOffsetProcessor) Symbol table processor for the dumps.
    """
    self._profiles = profiles
    self._processor = processor
    self._phase_offsets = None

  def IsStableProfile(self):
    """Verify that the profiling has been stable.

    See ComputeStability for details.

    Returns:
      True if the profile was stable as described above.
    """
    (startup_stability, common_stability,
     interaction_stability) = self.ComputeStability()

    stable = True
    if startup_stability > self.STARTUP_STABILITY_THRESHOLD:
      logging.error('Startup unstable: %.3f', startup_stability)
      stable = False
    if common_stability > self.COMMON_STABILITY_THRESHOLD:
      logging.error('Common unstable: %.3f', common_stability)
      stable = False
    if interaction_stability > self.INTERACTION_STABILITY_THRESHOLD:
      logging.error('Interaction unstable: %.3f', interaction_stability)
      stable = False

    return stable

  def ComputeStability(self):
    """Compute heuristic phase stability metrics.

    This computes the ratio in size of symbols between the union and
    intersection of all orderfile phases. Intuitively if this ratio is not too
    large it means that the profiling phases are stable with respect to the code
    they cover.

    Returns:
      (float, float, float) A heuristic stability metric for startup, common and
          interaction orderfile phases, respectively.
    """
    phase_offsets = self._GetOrderfilePhaseOffsets()
    assert len(phase_offsets) > 1  # Otherwise the analysis is silly.

    startup_union = set(phase_offsets[0].startup)
    startup_intersection = set(phase_offsets[0].startup)
    common_union = set(phase_offsets[0].common)
    common_intersection = set(phase_offsets[0].common)
    interaction_union = set(phase_offsets[0].interaction)
    interaction_intersection = set(phase_offsets[0].interaction)
    for offsets in phase_offsets[1:]:
      startup_union |= set(offsets.startup)
      startup_intersection &= set(offsets.startup)
      common_union |= set(offsets.common)
      common_intersection &= set(offsets.common)
      interaction_union |= set(offsets.interaction)
      interaction_intersection &= set(offsets.interaction)
    startup_stability = self._SafeDiv(
        self._processor.OffsetsPrimarySize(startup_union),
        self._processor.OffsetsPrimarySize(startup_intersection))
    common_stability = self._SafeDiv(
        self._processor.OffsetsPrimarySize(common_union),
        self._processor.OffsetsPrimarySize(common_intersection))
    interaction_stability = self._SafeDiv(
        self._processor.OffsetsPrimarySize(interaction_union),
        self._processor.OffsetsPrimarySize(interaction_intersection))
    return (startup_stability, common_stability, interaction_stability)

  def _GetOrderfilePhaseOffsets(self):
    """Compute the phase offsets for each run.

    Returns:
      [OrderfilePhaseOffsets] Each run corresponds to an OrderfilePhaseOffsets,
          which groups the symbol offsets discovered in the runs.
    """
    if self._phase_offsets is not None:
      return self._phase_offsets

    assert self._profiles.GetPhases() == set([0, 1]), 'Unexpected phases'
    self._phase_offsets = []
    for first, second in zip(self._profiles.GetRunGroupOffsets(phase=0),
                             self._profiles.GetRunGroupOffsets(phase=1)):
      all_first_offsets = self._processor.GetReachedOffsetsFromDump(first)
      all_second_offsets = self._processor.GetReachedOffsetsFromDump(second)
      first_offsets_set = set(all_first_offsets)
      second_offsets_set = set(all_second_offsets)
      common_offsets_set = first_offsets_set & second_offsets_set
      first_offsets_set -= common_offsets_set
      second_offsets_set -= common_offsets_set

      startup = [x for x in all_first_offsets
                 if x in first_offsets_set]

      interaction = [x for x in all_second_offsets
                     if x in second_offsets_set]

      common_seen = set()
      common = []
      for x in itertools.chain(all_first_offsets, all_second_offsets):
        if x in common_offsets_set and x not in common_seen:
          common_seen.add(x)
          common.append(x)

      self._phase_offsets.append(OrderfilePhaseOffsets(
          startup=startup,
          interaction=interaction,
          common=common))

    return self._phase_offsets

  @classmethod
  def _SafeDiv(cls, a, b):
    if not b:
      return None
    return float(a) / b


def _CreateArgumentParser():
  parser = argparse.ArgumentParser(
      description='Compute statistics on phased orderfiles')
  parser.add_argument('--profile-directory', type=str, required=True,
                      help=('Directory containing profile runs. Files '
                            'matching {} are used.'.format(PROFILE_GLOB)))
  parser.add_argument('--instrumented-build-dir', type=str,
                      help='Path to the instrumented build', required=True)
  parser.add_argument('--library-name', default='libchrome.so',
                      help=('Chrome shared library name (usually libchrome.so '
                            'or libmonochrome.so'))
  return parser


def main():
  logging.basicConfig(level=logging.INFO)
  parser = _CreateArgumentParser()
  args = parser.parse_args()
  profiles = process_profiles.ProfileManager(
      glob.glob(os.path.join(args.profile_directory, PROFILE_GLOB)))
  processor = process_profiles.SymbolOffsetProcessor(os.path.join(
      args.instrumented_build_dir, 'lib.unstripped', args.library_name))
  phaser = PhasedAnalyzer(profiles, processor)
  print 'Stability: {:.2f} {:.2f} {:.2f}'.format(*phaser.ComputeStability())


if __name__ == '__main__':
  main()
