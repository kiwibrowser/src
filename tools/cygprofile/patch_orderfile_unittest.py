#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import patch_orderfile
import symbol_extractor


class TestPatchOrderFile(unittest.TestCase):
  def testRemoveSuffixes(self):
    no_clone = 'this.does.not.contain.clone'
    self.assertEquals(no_clone, patch_orderfile.RemoveSuffixes(no_clone))
    with_clone = 'this.does.contain.clone.'
    self.assertEquals(
        'this.does.contain', patch_orderfile.RemoveSuffixes(with_clone))
    with_part = 'this.is.a.part.42'
    self.assertEquals(
        'this.is.a', patch_orderfile.RemoveSuffixes(with_part))

  def testSymbolsWithSameOffset(self):
    symbol_name = "dummySymbol"
    symbol_name2 = "other"
    name_to_symbol_infos = {symbol_name: [
        symbol_extractor.SymbolInfo(symbol_name, 0x42, 0x12,
                                    section='.text')]}
    offset_to_symbol_infos = {
        0x42: [symbol_extractor.SymbolInfo(symbol_name, 0x42, 0x12,
                                           section='.text'),
               symbol_extractor.SymbolInfo(symbol_name2, 0x42, 0x12,
                                           section='.text')]}
    symbol_names = patch_orderfile._SymbolsWithSameOffset(
        symbol_name, name_to_symbol_infos, offset_to_symbol_infos)
    self.assertEquals(len(symbol_names), 2)
    self.assertEquals(symbol_names[0], symbol_name)
    self.assertEquals(symbol_names[1], symbol_name2)
    self.assertEquals([], patch_orderfile._SymbolsWithSameOffset(
        "symbolThatShouldntMatch",
        name_to_symbol_infos, offset_to_symbol_infos))

  def testSectionNameToSymbols(self):
    mapping = {'.text.foo': ['foo'],
               '.text.hot.bar': ['bar', 'bar1']}
    self.assertEquals(list(patch_orderfile._SectionNameToSymbols(
                      '.text.foo', mapping)),
                      ['foo'])
    self.assertEquals(list(patch_orderfile._SectionNameToSymbols(
                      '.text.hot.bar', mapping)),
                      ['bar', 'bar1'])
    self.assertEquals(list(patch_orderfile._SectionNameToSymbols(
                      '.text.hot.bar', mapping)),
                      ['bar', 'bar1'])
    self.assertEquals(list(patch_orderfile._SectionNameToSymbols(
                      '.text.hot.foobar', mapping)),
                      ['foobar'])
    self.assertEquals(list(patch_orderfile._SectionNameToSymbols(
                      '.text.unlikely.*', mapping)),
                      [])

  def testSectionMatchingRules(self):
    symbol_name1 = 'symbol1'
    symbol_name2 = 'symbol2'
    symbol_name3 = 'symbol3'
    section_name1 = '.text.' + symbol_name1
    section_name3 = '.text.foo'
    suffixed = set([section_name3])
    name_to_symbol_infos = {symbol_name1: [
        symbol_extractor.SymbolInfo(symbol_name1, 0x42, 0x12,
                                    section='.text')]}
    offset_to_symbol_infos = {
        0x42: [symbol_extractor.SymbolInfo(symbol_name1, 0x42, 0x12,
                                           section='.text'),
               symbol_extractor.SymbolInfo(symbol_name2, 0x42, 0x12,
                                           section='.text')]}
    section_to_symbols_map = {section_name1: [symbol_name1],
                              section_name3: [symbol_name1, symbol_name3]}
    symbol_to_sections_map = {symbol_name1:
                                  [section_name1, section_name3],
                              symbol_name3: [section_name3]}
    expected = [
        section_name1,
        section_name3,
        section_name3 + '.*',
        '.text.hot.' + symbol_name1,
        '.text.unlikely.' + symbol_name1,
        symbol_name1,
        '.text.hot.symbol2',
        '.text.unlikely.symbol2',
        '.text.symbol2',
        'symbol2']
    self.assertEqual(expected, list(patch_orderfile._SectionMatchingRules(
        section_name1, name_to_symbol_infos, offset_to_symbol_infos,
        section_to_symbols_map, symbol_to_sections_map, suffixed)))

  def testUniqueGenerator(self):
    @patch_orderfile._UniqueGenerator
    def TestIterator():
      yield 1
      yield 2
      yield 1
      yield 3

    self.assertEqual(list(TestIterator()), [1,2,3])

  def testCombineSectionListsByPrimaryName(self):
    self.assertEqual(patch_orderfile._CombineSectionListsByPrimaryName(
        {'foo': ['.text.foo', '.text.bar.constprop.1'],
         'foo.part.1': ['.text.baz'],
         'foobar': ['.text.foobar']}),
        {'foo': ['.text.foo', '.text.bar', '.text.baz'],
         'foobar': ['.text.foobar']})

  def testSectionsWithSuffixes(self):
     self.assertEqual(patch_orderfile._SectionsWithSuffixes(
        {'foo': ['.text.foo', '.text.bar.constprop.1'],
         'foo.part.1': ['.text.baz'],
         'foobar': ['.text.foobar']}),
        set(['.text.bar']))


if __name__ == "__main__":
  unittest.main()
