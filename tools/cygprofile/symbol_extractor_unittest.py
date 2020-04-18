#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import symbol_extractor
import unittest

class TestSymbolInfo(unittest.TestCase):
  def testIgnoresBlankLine(self):
    symbol_info = symbol_extractor._FromObjdumpLine('')
    self.assertIsNone(symbol_info)

  def testIgnoresMalformedLine(self):
    # This line is too short.
    line = ('00c1b228      F .text  00000060 _ZN20trace_event')
    symbol_info = symbol_extractor._FromObjdumpLine(line)
    self.assertIsNone(symbol_info)
    # This line has the wrong marker.
    line = '00c1b228 l     f .text  00000060 _ZN20trace_event'
    symbol_info = symbol_extractor._FromObjdumpLine(line)
    self.assertIsNone(symbol_info)

  def testAssertionErrorOnInvalidLines(self):
    # This line has an invalid scope.
    line = ('00c1b228 z     F .text  00000060 _ZN20trace_event')
    self.assertRaises(AssertionError, symbol_extractor._FromObjdumpLine, line)
    # This line has too many fields.
    line = ('00c1b228 l     F .text  00000060 _ZN20trace_event too many')
    self.assertRaises(AssertionError, symbol_extractor._FromObjdumpLine, line)
    # This line has invalid characters in the symbol.
    line = ('00c1b228 l     F .text  00000060 _ZN20trace_?bad')
    self.assertRaises(AssertionError, symbol_extractor._FromObjdumpLine, line)
    # This line has an invalid character at the start of the symbol name.
    line = ('00c1b228 l     F .text  00000060 $_ZN20trace_bad')
    self.assertRaises(AssertionError, symbol_extractor._FromObjdumpLine, line)

  def testSymbolInfo(self):
    line = ('00c1c05c l     F .text  0000002c '
            '_GLOBAL__sub_I_chrome_main_delegate.cc')
    test_name = '_GLOBAL__sub_I_chrome_main_delegate.cc'
    test_offset = 0x00c1c05c
    test_size = 0x2c
    test_section = '.text'
    symbol_info = symbol_extractor._FromObjdumpLine(line)
    self.assertIsNotNone(symbol_info)
    self.assertEquals(test_offset, symbol_info.offset)
    self.assertEquals(test_size, symbol_info.size)
    self.assertEquals(test_name, symbol_info.name)
    self.assertEquals(test_section, symbol_info.section)

  def testHiddenSymbol(self):
    line = ('00c1c05c l     F .text  0000002c '
            '.hidden _GLOBAL__sub_I_chrome_main_delegate.cc')
    test_name = '_GLOBAL__sub_I_chrome_main_delegate.cc'
    test_offset = 0x00c1c05c
    test_size = 0x2c
    test_section = '.text'
    symbol_info = symbol_extractor._FromObjdumpLine(line)
    self.assertIsNotNone(symbol_info)
    self.assertEquals(test_offset, symbol_info.offset)
    self.assertEquals(test_size, symbol_info.size)
    self.assertEquals(test_name, symbol_info.name)
    self.assertEquals(test_section, symbol_info.section)

  def testDollarInSymbolName(self):
    # A $ character elsewhere in the symbol name is fine.
    # This is an example of a lambda function name from Clang.
    line = ('00c1b228 l     F .text  00000060 _ZZL11get_globalsvENK3$_1clEv')
    symbol_info = symbol_extractor._FromObjdumpLine(line)
    self.assertIsNotNone(symbol_info)
    self.assertEquals(0xc1b228, symbol_info.offset)
    self.assertEquals(0x60, symbol_info.size)
    self.assertEquals('_ZZL11get_globalsvENK3$_1clEv', symbol_info.name)
    self.assertEquals('.text', symbol_info.section)


class TestSymbolInfosFromStream(unittest.TestCase):
  def testSymbolInfosFromStream(self):
    lines = ['Garbage',
             '',
             '00c1c05c l     F .text  0000002c first',
             ''
             'more garbage',
             '00155 g     F .text  00000012 second']
    symbol_infos = symbol_extractor._SymbolInfosFromStream(lines)
    self.assertEquals(len(symbol_infos), 2)
    first = symbol_extractor.SymbolInfo('first', 0x00c1c05c, 0x2c, '.text')
    self.assertEquals(first, symbol_infos[0])
    second = symbol_extractor.SymbolInfo('second', 0x00155, 0x12, '.text')
    self.assertEquals(second, symbol_infos[1])


class TestSymbolInfoMappings(unittest.TestCase):
  def setUp(self):
    self.symbol_infos = [
        symbol_extractor.SymbolInfo('firstNameAtOffset', 0x42, 42, '.text'),
        symbol_extractor.SymbolInfo('secondNameAtOffset', 0x42, 42, '.text'),
        symbol_extractor.SymbolInfo('thirdSymbol', 0x64, 20, '.text')]

  def testGroupSymbolInfosByOffset(self):
    offset_to_symbol_info = symbol_extractor.GroupSymbolInfosByOffset(
        self.symbol_infos)
    self.assertEquals(len(offset_to_symbol_info), 2)
    self.assertIn(0x42, offset_to_symbol_info)
    self.assertEquals(offset_to_symbol_info[0x42][0], self.symbol_infos[0])
    self.assertEquals(offset_to_symbol_info[0x42][1], self.symbol_infos[1])
    self.assertIn(0x64, offset_to_symbol_info)
    self.assertEquals(offset_to_symbol_info[0x64][0], self.symbol_infos[2])

  def testCreateNameToSymbolInfo(self):
    name_to_symbol_info = symbol_extractor.CreateNameToSymbolInfo(
        self.symbol_infos)
    self.assertEquals(len(name_to_symbol_info), 3)
    for i in range(3):
      name = self.symbol_infos[i].name
      self.assertIn(name, name_to_symbol_info)
      self.assertEquals(self.symbol_infos[i], name_to_symbol_info[name])

  def testSymbolCollisions(self):
    symbol_infos_with_collision = list(self.symbol_infos)
    symbol_infos_with_collision.append(symbol_extractor.SymbolInfo(
        'secondNameAtOffset', 0x84, 42, '.text'))

    # The symbol added above should not affect the output.
    name_to_symbol_info = symbol_extractor.CreateNameToSymbolInfo(
        self.symbol_infos)
    self.assertEquals(len(name_to_symbol_info), 3)
    for i in range(3):
      name = self.symbol_infos[i].name
      self.assertIn(name, name_to_symbol_info)
      self.assertEquals(self.symbol_infos[i], name_to_symbol_info[name])

if __name__ == '__main__':
  unittest.main()
