#!/usr/bin/env vpython
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Patch an orderfile.

Starting with a list of symbols in a binary and an orderfile (ordered list of
sections), matches the symbols in the orderfile and augments each symbol with
the symbols residing at the same address (due to having identical code).  The
output is a list of section or symbols matching rules appropriate for the linker
option -section-ordering-file for gold and --symbol-ordering-file for lld. Both
linkers are fine with extra directives that aren't matched in the binary, so we
construct a file suitable for both, concatenating sections and symbols. We
assume that the unpatched orderfile is built for gold, that is, it only contains
sections.

Note: It is possible to have.
- Several symbols mapping to the same offset in the binary.
- Several offsets for a given symbol (because we strip the ".clone." and other
  suffixes)

The general pipeline is:
1. Get the symbol infos (name, offset, size, section) from the binary
2. Get the symbol names from the orderfile
3. Find the orderfile symbol names in the symbols coming from the binary
4. For each symbol found, get all the symbols at the same address
5. Output them to an updated orderfile suitable for gold and lld
6. Output catch-all section matching rules for unprofiled methods. This is
   ineffective for lld, as it doesn't handle wildcards, but puts unordered
   symbols after the ordered ones.
"""

import argparse
import collections
import logging
import sys

import cyglog_to_orderfile
import cygprofile_utils
import symbol_extractor

# Prefixes for the symbols. We strip them from the incoming symbols, and add
# them back in the output file.
# Output sections are constructed as prefix + symbol_name, hence the empty
# prefix is used to generate the symbol entry for lld.
_PREFIXES = ('.text.hot.', '.text.unlikely.', '.text.', '')

# Suffixes for the symbols.  These are due to method splitting for inlining and
# method cloning for various reasons including constant propagation and
# inter-procedural optimization.
_SUFFIXES = ('.clone.', '.part.', '.isra.', '.constprop.')


def RemoveSuffixes(name):
  """Strips method name suffixes from cloning and splitting.

  .clone. comes from cloning in -O3.
  .part.  comes from partial method splitting for inlining.
  .isra.  comes from inter-procedural optimizations.
  .constprop. is cloning for constant propagation.
  """
  for suffix in _SUFFIXES:
    name = name.split(suffix)[0]
  return name


def _UniqueGenerator(generator):
  """Converts a generator to skip yielding elements already seen.

  Example:
    @_UniqueGenerator
    def Foo():
      yield 1
      yield 2
      yield 1
      yield 3

    Foo() yields 1,2,3.
  """
  def _FilteringFunction(*args, **kwargs):
    returned = set()
    for item in generator(*args, **kwargs):
      if item in returned:
        continue
      returned.add(item)
      yield item

  return _FilteringFunction


def _GroupSymbolInfosFromBinary(binary_filename):
  """Group all the symbols from a binary by name and offset.

  Args:
    binary_filename: path to the binary.

  Returns:
    A tuple of dict:
    (offset_to_symbol_infos, name_to_symbol_infos):
    - offset_to_symbol_infos: {offset: [symbol_info1, ...]}
    - name_to_symbol_infos: {name: [symbol_info1, ...]}
  """
  symbol_infos = symbol_extractor.SymbolInfosFromBinary(binary_filename)
  symbol_infos_no_suffixes = [
      s._replace(name=RemoveSuffixes(s.name)) for s in symbol_infos]
  return (symbol_extractor.GroupSymbolInfosByOffset(symbol_infos_no_suffixes),
          symbol_extractor.GroupSymbolInfosByName(symbol_infos_no_suffixes))


def _StripPrefix(line):
  """Strips the linker section name prefix from a symbol line.

  Args:
    line: a line from an orderfile, usually in the form:
          .text.SymbolName

  Returns:
    The symbol, SymbolName in the example above.
  """
  # Went away with GCC, make sure it doesn't come back, as the orderfile
  # no longer contains it.
  assert not line.startswith('.text.startup.')
  for prefix in _PREFIXES:
    if prefix and line.startswith(prefix):
      return line[len(prefix):]
  return line  # Unprefixed case


def _SectionNameToSymbols(section_name, section_to_symbols_map):
  """Yields all symbols which could be referred to by section_name.

  If the section name is present in the map, the names in the map are returned.
  Otherwise, any clone annotations and prefixes are stripped from the section
  name and the remainder is returned.
  """
  if (not section_name or
      section_name == '.text' or
      section_name.endswith('*')):
    return  # Don't return anything for catch-all sections
  if section_name in section_to_symbols_map:
    for symbol in section_to_symbols_map[section_name]:
      yield symbol
  else:
    name = _StripPrefix(section_name)
    if name:
      yield name


def GetSectionsFromOrderfile(filename):
  """Yields the sections from an orderfile.

  Args:
    filename: The name of the orderfile.

  Yields:
    A list of symbol names.
  """
  with open(filename, 'r') as f:
    for line in f.xreadlines():
      line = line.rstrip('\n')
      if line:
        yield line


@_UniqueGenerator
def GetSymbolsFromOrderfile(filename, section_to_symbols_map):
  """Yields the symbols from an orderfile.  Output elements do not repeat.

  Args:
    filename: The name of the orderfile.
    section_to_symbols_map: The mapping from section to symbol names.  If a
                            section name is missing from the mapping, the
                            symbol name is assumed to be the section name with
                            prefixes and suffixes stripped.

  Yields:
    A list of symbol names.
  """
  # TODO(lizeb,pasko): Move this method to symbol_extractor.py
  for section in GetSectionsFromOrderfile(filename):
    for symbol in _SectionNameToSymbols(RemoveSuffixes(section),
                                        section_to_symbols_map):
      yield symbol


def _SymbolsWithSameOffset(profiled_symbol, name_to_symbol_info,
                           offset_to_symbol_info):
  """Expands a symbol to include all symbols with the same offset.

  Args:
    profiled_symbol: the string symbol name to be expanded.
    name_to_symbol_info: {name: [symbol_info1], ...}, as returned by
        GetSymbolInfosFromBinary
    offset_to_symbol_info: {offset: [symbol_info1, ...], ...}

  Returns:
    A list of symbol names, or an empty list if profiled_symbol was not in
    name_to_symbol_info.
  """
  if profiled_symbol not in name_to_symbol_info:
    return []
  symbol_infos = name_to_symbol_info[profiled_symbol]
  expanded = []
  for symbol_info in symbol_infos:
    expanded += (s.name for s in offset_to_symbol_info[symbol_info.offset])
  return expanded


@_UniqueGenerator
def _SectionMatchingRules(section_name, name_to_symbol_infos,
                          offset_to_symbol_infos, section_to_symbols_map,
                          symbol_to_sections_map, suffixed_sections):
  """Gets the set of section matching rules for section_name.

  These rules will include section_name, but also any sections which may
  contain the same code due to cloning, splitting, or identical code folding.

  Args:
    section_name: The section to expand.
    name_to_symbol_infos: {name: [symbol_info1], ...}, as returned by
        GetSymbolInfosFromBinary.
    offset_to_symbol_infos: {offset: [symbol_info1, ...], ...}
    section_to_symbols_map: The mapping from section to symbol name.  Missing
        section names are treated as per _SectionNameToSymbols.
    symbol_to_sections_map: The mapping from symbol name to names of linker
        sections containing the symbol.  If a symbol isn't in the mapping, the
        section names are generated from the set of _PREFIXES with the symbol
        name.
    suffixed_sections: A set of sections which can have suffixes.

  Yields:
    Section names including at least section_name.
  """
  for name in _ExpandSection(section_name, name_to_symbol_infos,
                             offset_to_symbol_infos, section_to_symbols_map,
                             symbol_to_sections_map):
    yield name
    # Since only a subset of methods (mostly those compiled with O2) ever get
    # suffixes, don't emit the wildcards for ones where it won't be helpful.
    # Otherwise linking takes too long.
    if name in suffixed_sections:
      # TODO(lizeb,pasko): instead of just appending .*, append .suffix.* for
      # _SUFFIXES.  We can't do this right now because that many wildcards
      # seems to kill the linker (linking libchrome takes 3 hours).  This gets
      # almost all the benefit at a much lower link-time cost, but could cause
      # problems with unexpected suffixes.
      yield name + '.*'


def _ExpandSection(section_name, name_to_symbol_infos, offset_to_symbol_infos,
                   section_to_symbols_map, symbol_to_sections_map):
  """Yields the set of section names for section_name.

  This set will include section_name, but also any sections which may contain
  the same code due to identical code folding.

  Args:
    section_name: The section to expand.
    name_to_symbol_infos: {name: [symbol_info1], ...}, as returned by
        GetSymbolInfosFromBinary.
    offset_to_symbol_infos: {offset: [symbol_info1, ...], ...}
    section_to_symbols_map: The mapping from section to symbol name.  Missing
        section names are treated as per _SectionNameToSymbols.
    symbol_to_sections_map: The mapping from symbol name to names of linker
        sections containing the symbol.  If a symbol isn't in the mapping, the
        section names are generated from the set of _PREFIXES with the symbol
        name.

  Yields:
    Section names including at least section_name.
  """
  yield section_name
  for first_sym in _SectionNameToSymbols(section_name,
                                         section_to_symbols_map):
    for symbol in _SymbolsWithSameOffset(first_sym, name_to_symbol_infos,
                                         offset_to_symbol_infos):
      if symbol in symbol_to_sections_map:
        for section in symbol_to_sections_map[symbol]:
          yield section
      for prefix in _PREFIXES:
        yield prefix + symbol


@_UniqueGenerator
def _ExpandSections(section_names, name_to_symbol_infos,
                    offset_to_symbol_infos, section_to_symbols_map,
                    symbol_to_sections_map, suffixed_sections):
  """Gets an ordered set of section matching rules for a list of sections.

  Rules will not be repeated.

  Args:
    section_names: The sections to expand.
    name_to_symbol_infos: {name: [symbol_info1], ...}, as returned by
                          _GroupSymbolInfosFromBinary.
    offset_to_symbol_infos: {offset: [symbol_info1, ...], ...}
    section_to_symbols_map: The mapping from section to symbol names.
    symbol_to_sections_map: The mapping from symbol name to names of linker
                            sections containing the symbol.
    suffixed_sections:      A set of sections which can have suffixes.

  Yields:
    Section matching rules including at least section_names.
  """
  for profiled_section in section_names:
    for section in _SectionMatchingRules(
        profiled_section, name_to_symbol_infos, offset_to_symbol_infos,
        section_to_symbols_map, symbol_to_sections_map, suffixed_sections):
      yield section


def _CombineSectionListsByPrimaryName(symbol_to_sections_map):
  """Combines values of the symbol_to_sections_map by stripping suffixes.

  Example:
    {foo: [.text.foo, .text.bar.part.1],
     foo.constprop.4: [.text.baz.constprop.3]} ->
    {foo: [.text.foo, .text.bar, .text.baz]}

  Args:
    symbol_to_sections_map: Mapping from symbol name to list of section names

  Returns:
    The same mapping, but with symbol and section names suffix-stripped.
  """
  simplified = {}
  for suffixed_symbol, suffixed_sections in symbol_to_sections_map.iteritems():
    symbol = RemoveSuffixes(suffixed_symbol)
    sections = [RemoveSuffixes(section) for section in suffixed_sections]
    simplified.setdefault(symbol, []).extend(sections)
  return simplified


def _SectionsWithSuffixes(symbol_to_sections_map):
  """Finds sections which have suffixes applied.

  Args:
    symbol_to_sections_map: a map where the values are lists of section names.

  Returns:
    A set containing all section names which were seen with suffixes applied.
  """
  sections_with_suffixes = set()
  for suffixed_sections in symbol_to_sections_map.itervalues():
    for suffixed_section in suffixed_sections:
      section = RemoveSuffixes(suffixed_section)
      if section != suffixed_section:
        sections_with_suffixes.add(section)
  return sections_with_suffixes


def _StripSuffixes(section_list):
  """Remove all suffixes on items in a list of sections or symbols."""
  return [RemoveSuffixes(section) for section in section_list]


def GeneratePatchedOrderfile(unpatched_orderfile, native_lib_filename,
                             output_filename):
  """Writes a patched orderfile.

  Args:
    unpatched_orderfile: (str) Path to the unpatched orderfile.
    native_lib_filename: (str) Path to the native library.
    output_filename: (str) Path to the patched orderfile.
  """
  (offset_to_symbol_infos, name_to_symbol_infos) = _GroupSymbolInfosFromBinary(
      native_lib_filename)
  obj_dir = cygprofile_utils.GetObjDir(native_lib_filename)
  raw_symbol_map = cyglog_to_orderfile.ObjectFileProcessor(
      obj_dir).GetSymbolToSectionsMap()
  suffixed = _SectionsWithSuffixes(raw_symbol_map)
  symbol_to_sections_map = _CombineSectionListsByPrimaryName(raw_symbol_map)
  section_to_symbols_map = cygprofile_utils.InvertMapping(
      symbol_to_sections_map)
  profiled_sections = _StripSuffixes(
      GetSectionsFromOrderfile(unpatched_orderfile))
  expanded_sections = _ExpandSections(
      profiled_sections, name_to_symbol_infos, offset_to_symbol_infos,
      section_to_symbols_map, symbol_to_sections_map, suffixed)

  with open(output_filename, 'w') as f:
    # Make sure the anchor functions are located in the right place, here and
    # after everything else.
    # See the comment in //base/android/library_loader/anchor_functions.cc.
    #
    # __cxx_global_var_init is one of the largest symbols (~38kB as of May
    # 2018), called extremely early, and not instrumented.
    first_sections = ('dummy_function_start_of_ordered_text',
                      '__cxx_global_var_init')
    for section in first_sections:
      for prefix in _PREFIXES:
        f.write(prefix + section + '\n')

    for section in expanded_sections:
      f.write(section + '\n')

    for prefix in _PREFIXES:
      f.write(prefix + 'dummy_function_end_of_ordered_text\n')

    # The following is needed otherwise Gold only applies a partial sort.
    f.write('.text\n')  # gets methods not in a section, such as assembly
    f.write('.text.*\n')  # gets everything else

    # Since wildcards are not supported by lld, the "end of text" anchor symbol
    # is not emitted, a different mechanism is used instead. See comments in the
    # file above.
    for prefix in _PREFIXES:
      if prefix:
        f.write(prefix + 'dummy_function_at_the_end_of_text\n')


def _CreateArgumentParser():
  """Creates and returns the argument parser."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--target-arch', action='store',
                      choices=['arm', 'arm64', 'x86', 'x86_64', 'x64', 'mips'],
                      help='The target architecture for the library.')
  parser.add_argument('--unpatched-orderfile', required=True,
                      help='Path to the unpatched orderfile')
  parser.add_argument('--native-library', required=True,
                      help='Path to the native library')
  parser.add_argument('--output-file', required=True, help='Output filename')
  return parser


def main():
  parser = _CreateArgumentParser()
  options = parser.parse_args()
  if not options.target_arch:
    options.arch = cygprofile_utils.DetectArchitecture()
  symbol_extractor.SetArchitecture(options.target_arch)
  GeneratePatchedOrderfile(options.unpatched_orderfile, options.native_library,
                           options.output_file)
  return 0


if __name__ == '__main__':
  logging.basicConfig(level=logging.INFO)
  sys.exit(main())
