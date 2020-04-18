#!/usr/bin/env vpython
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Check that symbols are ordered into a binary as they appear in the orderfile.
"""

import logging
import optparse
import sys

import cyglog_to_orderfile
import cygprofile_utils
import patch_orderfile
import symbol_extractor


_MAX_WARNINGS_TO_PRINT = 200


def _IsSameMethod(name1, name2):
  """Returns true if name1 or name2 are split method forms of the other."""
  return patch_orderfile.RemoveSuffixes(name1) == \
         patch_orderfile.RemoveSuffixes(name2)


def _CountMisorderedSymbols(symbols, symbol_infos):
  """Count the number of misordered symbols, and log them.

  Args:
    symbols: ordered sequence of symbols from the orderfile
    symbol_infos: ordered list of SymbolInfo from the binary

  Returns:
    (misordered_pairs_count, matched_symbols_count, unmatched_symbols_count)
  """
  name_to_symbol_info = symbol_extractor.CreateNameToSymbolInfo(symbol_infos)
  matched_symbol_infos = []
  missing_count = 0
  misordered_count = 0

  # Find the SymbolInfo matching the orderfile symbols in the binary.
  for symbol in symbols:
    if symbol in name_to_symbol_info:
      matched_symbol_infos.append(name_to_symbol_info[symbol])
    else:
      missing_count += 1
      if missing_count < _MAX_WARNINGS_TO_PRINT:
        logging.warning('Symbol "%s" is in the orderfile, not in the binary' %
                        symbol)
  logging.info('%d matched symbols, %d un-matched (Only the first %d unmatched'
               ' symbols are shown)' % (
                   len(matched_symbol_infos), missing_count,
                   _MAX_WARNINGS_TO_PRINT))

  # In the order of the orderfile, find all the symbols that are at an offset
  # smaller than their immediate predecessor, and record the pair.
  previous_symbol_info = symbol_extractor.SymbolInfo(
      name='', offset=-1, size=0, section='')
  for symbol_info in matched_symbol_infos:
    if symbol_info.offset < previous_symbol_info.offset and not (
        _IsSameMethod(symbol_info.name, previous_symbol_info.name)):
      logging.warning('Misordered pair: %s - %s' % (
          str(previous_symbol_info), str(symbol_info)))
      misordered_count += 1
    previous_symbol_info = symbol_info
  return (misordered_count, len(matched_symbol_infos), missing_count)


def main():
  parser = optparse.OptionParser(usage=
      'usage: %prog [options] <binary> <orderfile>')
  parser.add_option('--target-arch', action='store', dest='arch',
                    choices=['arm', 'arm64', 'x86', 'x86_64', 'x64', 'mips'],
                    help='The target architecture for the binary.')
  parser.add_option('--threshold', action='store', dest='threshold', default=20,
                    help='The maximum allowed number of out-of-order symbols.')
  options, argv = parser.parse_args(sys.argv)
  if not options.arch:
    options.arch = cygprofile_utils.DetectArchitecture()
  if len(argv) != 3:
    parser.print_help()
    return 1
  (binary_filename, orderfile_filename) = argv[1:]

  symbol_extractor.SetArchitecture(options.arch)
  obj_dir = cygprofile_utils.GetObjDir(binary_filename)
  symbol_to_sections_map = cyglog_to_orderfile.ObjectFileProcessor(
      obj_dir).GetSymbolToSectionsMap()
  section_to_symbols_map = cygprofile_utils.InvertMapping(
      symbol_to_sections_map)
  symbols = patch_orderfile.GetSymbolsFromOrderfile(orderfile_filename,
                                                    section_to_symbols_map)
  symbol_infos = symbol_extractor.SymbolInfosFromBinary(binary_filename)
  # Missing symbols is not an error since some of them can be eliminated through
  # inlining.
  (misordered_pairs_count, matched_symbols, _) = _CountMisorderedSymbols(
      symbols, symbol_infos)
  return (misordered_pairs_count > options.threshold) or (matched_symbols == 0)


if __name__ == '__main__':
  logging.basicConfig(level=logging.INFO)
  sys.exit(main())
