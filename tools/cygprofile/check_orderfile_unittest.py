#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import check_orderfile
import symbol_extractor


class TestCheckOrderFile(unittest.TestCase):
  _SYMBOL_INFOS = [symbol_extractor.SymbolInfo('first', 0x1, 0, ''),
                   symbol_extractor.SymbolInfo('second', 0x2, 0, ''),
                   symbol_extractor.SymbolInfo('notProfiled', 0x4, 0, ''),
                   symbol_extractor.SymbolInfo('third', 0x3, 0, ''),]

  def testMatchesSymbols(self):
    symbols = ['first', 'second', 'third']
    (misordered_pairs_count, matched_count, missing_count) = (
        check_orderfile._CountMisorderedSymbols(symbols, self._SYMBOL_INFOS))
    self.assertEquals(
        (misordered_pairs_count, matched_count, missing_count), (0, 3, 0))

  def testMissingMatches(self):
    symbols = ['second', 'third', 'other', 'first']
    (_, matched_count, unmatched_count) = (
        check_orderfile._CountMisorderedSymbols(symbols, self._SYMBOL_INFOS))
    self.assertEquals(matched_count, 3)
    self.assertEquals(unmatched_count, 1)

  def testNoUnorderedSymbols(self):
    symbols = ['first', 'other', 'second', 'third', 'noMatchEither']
    (misordered_pairs_count, _, _) = (
        check_orderfile._CountMisorderedSymbols(symbols, self._SYMBOL_INFOS))
    self.assertEquals(misordered_pairs_count, 0)

  def testUnorderedSymbols(self):
    symbols = ['first', 'other', 'third', 'second', 'noMatchEither']
    (misordered_pairs_count, _, _) = (
        check_orderfile._CountMisorderedSymbols(symbols, self._SYMBOL_INFOS))
    self.assertEquals(misordered_pairs_count, 1)


if __name__ == '__main__':
  unittest.main()
