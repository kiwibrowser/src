#!/usr/bin/env vpython
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for process_profiles.py."""

import collections
import unittest

import process_profiles

from test_utils import (SimpleTestSymbol,
                        TestSymbolOffsetProcessor,
                        TestProfileManager)

class ProcessProfilesTestCase(unittest.TestCase):

  def setUp(self):
    self.symbol_0 = SimpleTestSymbol('0', 0, 0)
    self.symbol_1 = SimpleTestSymbol('1', 8, 16)
    self.symbol_2 = SimpleTestSymbol('2', 32, 8)
    self.symbol_3 = SimpleTestSymbol('3', 40, 12)
    self.offset_to_symbol_info = (
        [None, None] + [self.symbol_1] * 4 + [None] * 2 + [self.symbol_2] * 2
        + [self.symbol_3] * 3)
    self.symbol_infos = [self.symbol_0, self.symbol_1,
                         self.symbol_2, self.symbol_3]
    self._file_counter = 0

  def File(self, timestamp_sec, phase):
    self._file_counter += 1
    return 'file-{}-{}.txt_{}'.format(
        self._file_counter, timestamp_sec * 1000 * 1000 * 1000, phase)

  def testGetOffsetToSymbolInfo(self):
    processor = TestSymbolOffsetProcessor(self.symbol_infos)
    offset_to_symbol_info = processor._GetDumpOffsetToSymbolInfo()
    self.assertListEqual(self.offset_to_symbol_info, offset_to_symbol_info)

  def testGetReachedOffsetsFromDump(self):
    processor = TestSymbolOffsetProcessor(self.symbol_infos)
    # 2 hits for symbol_1, 0 for symbol_2, 1 for symbol_3
    dump = [8, 12, 48]
    reached = processor.GetReachedOffsetsFromDump(dump)
    self.assertListEqual([self.symbol_1.offset, self.symbol_3.offset], reached)
    # Ordering matters, no repetitions
    dump = [48, 12, 8, 12, 8, 16]
    reached = processor.GetReachedOffsetsFromDump(dump)
    self.assertListEqual([self.symbol_3.offset, self.symbol_1.offset], reached)

  def testSymbolNameToPrimary(self):
    symbol_infos = [SimpleTestSymbol('1', 8, 16),
                    SimpleTestSymbol('AnAlias', 8, 16),
                    SimpleTestSymbol('Another', 40, 16)]
    processor = TestSymbolOffsetProcessor(symbol_infos)
    self.assertDictEqual({8: symbol_infos[0],
                          40: symbol_infos[2]}, processor.OffsetToPrimaryMap())

  def testOffsetToSymbolsMap(self):
    symbol_infos = [SimpleTestSymbol('1', 8, 16),
                    SimpleTestSymbol('AnAlias', 8, 16),
                    SimpleTestSymbol('Another', 40, 16)]
    processor = TestSymbolOffsetProcessor(symbol_infos)
    self.assertDictEqual({8: [symbol_infos[0], symbol_infos[1]],
                          40: [symbol_infos[2]]},
                         processor.OffsetToSymbolsMap())

  def testPrimarySizeMismatch(self):
    symbol_infos = [SimpleTestSymbol('1', 8, 16),
                    SimpleTestSymbol('AnAlias', 8, 32)]
    processor = TestSymbolOffsetProcessor(symbol_infos)
    self.assertRaises(AssertionError, processor.OffsetToPrimaryMap)
    symbol_infos = [SimpleTestSymbol('1', 8, 0),
                    SimpleTestSymbol('2', 8, 32),
                    SimpleTestSymbol('3', 8, 32),
                    SimpleTestSymbol('4', 8, 0),]
    processor = TestSymbolOffsetProcessor(symbol_infos)
    self.assertDictEqual({8: symbol_infos[1]}, processor.OffsetToPrimaryMap())

  def testMatchSymbols(self):
    symbols = [SimpleTestSymbol('W', 30, 10),
               SimpleTestSymbol('Y', 60, 5),
               SimpleTestSymbol('X', 100, 10)]
    processor = TestSymbolOffsetProcessor(symbols)
    self.assertListEqual(symbols[1:3],
                         processor.MatchSymbolNames(['Y', 'X']))

  def testOffsetsPrimarySize(self):
    symbols = [SimpleTestSymbol('W', 10, 1),
               SimpleTestSymbol('X', 20, 2),
               SimpleTestSymbol('Y', 30, 4),
               SimpleTestSymbol('Z', 40, 8)]
    processor = TestSymbolOffsetProcessor(symbols)
    self.assertEqual(13, processor.OffsetsPrimarySize([10, 30, 40]))

  def testMedian(self):
    self.assertEquals(None, process_profiles._Median([]))
    self.assertEquals(5, process_profiles._Median([5]))
    self.assertEquals(5, process_profiles._Median([1, 5, 20]))
    self.assertEquals(5, process_profiles._Median([4, 6]))
    self.assertEquals(5, process_profiles._Median([1, 4, 6, 100]))
    self.assertEquals(5, process_profiles._Median([1, 4, 5, 6, 100]))

  def testRunGroups(self):
    files = [self.File(40, 0), self.File(100, 0), self.File(200, 1),
             self.File(35, 1), self.File(42, 0), self.File(95, 0)]
    mgr = process_profiles.ProfileManager(files)
    mgr._ComputeRunGroups()
    self.assertEquals(3, len(mgr._run_groups))
    self.assertEquals(3, len(mgr._run_groups[0].Filenames()))
    self.assertEquals(2, len(mgr._run_groups[1].Filenames()))
    self.assertEquals(1, len(mgr._run_groups[2].Filenames()))
    self.assertTrue(files[0] in mgr._run_groups[0].Filenames())
    self.assertTrue(files[3] in mgr._run_groups[0].Filenames())
    self.assertTrue(files[4] in mgr._run_groups[0].Filenames())
    self.assertTrue(files[1] in mgr._run_groups[1].Filenames())
    self.assertTrue(files[5] in mgr._run_groups[1].Filenames())
    self.assertTrue(files[2] in mgr._run_groups[2].Filenames())

  def testReadOffsets(self):
    mgr = TestProfileManager({
        self.File(30, 0): [1, 3, 5, 7],
        self.File(40, 1): [8, 10],
        self.File(50, 0): [13, 15]})
    self.assertListEqual([1, 3, 5, 7, 8, 10, 13, 15],
                         mgr.GetMergedOffsets())
    self.assertListEqual([8, 10], mgr.GetMergedOffsets(1))
    self.assertListEqual([], mgr.GetMergedOffsets(2))

  def testRunGroupOffsets(self):
    mgr = TestProfileManager({
        self.File(30, 0): [1, 2, 3, 4],
        self.File(150, 0): [9, 11, 13],
        self.File(40, 1): [5, 6, 7]})
    offsets_list = mgr.GetRunGroupOffsets()
    self.assertEquals(2, len(offsets_list))
    self.assertListEqual([1, 2, 3, 4, 5, 6, 7], offsets_list[0])
    self.assertListEqual([9, 11, 13], offsets_list[1])
    offsets_list = mgr.GetRunGroupOffsets(0)
    self.assertEquals(2, len(offsets_list))
    self.assertListEqual([1, 2, 3, 4], offsets_list[0])
    self.assertListEqual([9, 11, 13], offsets_list[1])
    offsets_list = mgr.GetRunGroupOffsets(1)
    self.assertEquals(2, len(offsets_list))
    self.assertListEqual([5, 6, 7], offsets_list[0])
    self.assertListEqual([], offsets_list[1])

  def testSorted(self):
    # The fact that the ProfileManager sorts by filename is implicit in the
    # other tests. It is tested explicitly here.
    mgr = TestProfileManager({
        self.File(40, 0): [1, 2, 3, 4],
        self.File(150, 0): [9, 11, 13],
        self.File(30, 1): [5, 6, 7]})
    offsets_list = mgr.GetRunGroupOffsets()
    self.assertEquals(2, len(offsets_list))
    self.assertListEqual([5, 6, 7, 1, 2, 3, 4], offsets_list[0])

  def testPhases(self):
    mgr = TestProfileManager({
        self.File(40, 0): [],
        self.File(150, 0): [],
        self.File(30, 1): [],
        self.File(30, 2): [],
        self.File(30, 0): []})
    self.assertEquals(set([0,1,2]), mgr.GetPhases())


if __name__ == '__main__':
  unittest.main()
