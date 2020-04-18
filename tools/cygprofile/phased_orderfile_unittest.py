#!/usr/bin/env vpython
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for phased_orderfile.py."""

import collections
import unittest

import phased_orderfile
import process_profiles

from test_utils import (SimpleTestSymbol,
                        TestSymbolOffsetProcessor,
                        TestProfileManager)


class Mod10Processor(object):
  """A restricted mock for a SymbolOffsetProcessor.

  This only implements GetReachedOffsetsFromDump, and works by mapping a dump
  offset to offset - (offset % 10). If the dump offset is negative, it is marked
  as not found.
  """
  def GetReachedOffsetsFromDump(self, dump):
    return [x - (x % 10) for x in dump if x >= 0]


class PhasedOrderfileTestCase(unittest.TestCase):

  def setUp(self):
    self._file_counter = 0

  def File(self, timestamp_sec, phase):
    self._file_counter += 1
    return 'file-{}-{}.txt_{}'.format(
        self._file_counter, timestamp_sec * 1000 * 1000 * 1000, phase)

  def testProfileStability(self):
    symbols = [SimpleTestSymbol(str(i), i, 10)
               for i in xrange(20)]
    phaser = phased_orderfile.PhasedAnalyzer(
        None, TestSymbolOffsetProcessor(symbols))
    opo = lambda s, c, i: phased_orderfile.OrderfilePhaseOffsets(
        startup=s, common=c, interaction=i)
    phaser._phase_offsets = [opo(range(5), range(6, 10), range(11,15)),
                             opo(range(4), range(6, 10), range(18, 20))]
    self.assertEquals((1.25, 1, None), phaser.ComputeStability())

  def testIsStable(self):
    symbols = [SimpleTestSymbol(str(i), i, 10)
               for i in xrange(20)]
    phaser = phased_orderfile.PhasedAnalyzer(
        None, TestSymbolOffsetProcessor(symbols))
    opo = lambda s, c, i: phased_orderfile.OrderfilePhaseOffsets(
        startup=s, common=c, interaction=i)
    phaser._phase_offsets = [opo(range(5), range(6, 10), range(11,15)),
                             opo(range(4), range(6, 10), range(18, 20))]
    phaser.STARTUP_STABILITY_THRESHOLD = 1.1
    self.assertFalse(phaser.IsStableProfile())
    phaser.STARTUP_STABILITY_THRESHOLD = 1.5
    self.assertTrue(phaser.IsStableProfile())

  def testGetOrderfilePhaseOffsets(self):
    mgr = TestProfileManager({
        self.File(0, 0): [12, 21, -1, 33],
        self.File(0, 1): [31, 49, 52],
        self.File(100, 0): [113, 128],
        self.File(200, 1): [132, 146],
        self.File(300, 0): [19, 20, 32],
        self.File(300, 1): [24, 39]})
    phaser = phased_orderfile.PhasedAnalyzer(mgr, Mod10Processor())
    opo = lambda s, c, i: phased_orderfile.OrderfilePhaseOffsets(
        startup=s, common=c, interaction=i)
    self.assertListEqual([opo([10, 20], [30], [40, 50]),
                          opo([110, 120], [], []),
                          opo([], [], [130, 140]),
                          opo([10], [20, 30], [])],
                         phaser._GetOrderfilePhaseOffsets())


if __name__ == "__main__":
  unittest.main()
