#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests that a set of symbols are truly pruned from the translator.

Compares the pruned down "on-device" translator with the "fat" host build
which has not been pruned down.
"""

import glob
import re
import subprocess
import sys
import unittest

class SymbolInfo(object):

  def __init__(self, lib_name, sym_name, t, size):
    self.lib_name = lib_name
    self.sym_name = sym_name
    self.type = t
    self.size = size


def is_weak(t):
  t = t.upper()
  return t == 'V' or t == 'W'


def is_local(t):
  # According to the NM documentation:
  #   "If lowercase, the symbol is usually local...  There are however a few
  #   lowercase symbols that are shown for special global symbols
  #   ("u", "v" and "w")."
  return t != 'u' and not is_weak(t) and t.islower()


def merge_symbols(sdict1, sdict2):
  for sym_name, v2 in sdict2.iteritems():
    # Check for duplicate symbols.
    if sym_name in sdict1:
      v1 = sdict1[sym_name]
      # Only print warning if they are not weak / differently sized.
      if (not (is_weak(v2.type) or is_weak(v1.type)) and
          v1.size != v2.size):
        print 'Warning symbol %s defined in both %s(%d, %s) and %s(%d, %s)' % (
            sym_name,
            v1.lib_name, v1.size, v1.type,
            v2.lib_name, v2.size, v2.type)
      # Arbitrarily take the max.  The sizes are approximate anyway,
      # since the host binaries are built from a different compiler.
      v1.size = max(v1.size, v2.size)
      continue
    # Otherwise just copy info over to sdict2.
    sdict1[sym_name] = sdict2[sym_name]
  return sdict1


class TestTranslatorPruned(unittest.TestCase):

  pruned_symbols = {}
  unpruned_symbols = {}

  @classmethod
  def get_symbol_info(cls, nm_tool, cxxfilt_tool, bin_name):
    results = {}
    nm_cmd = [nm_tool, '--size-sort', bin_name]
    print 'Getting symbols and sizes by running:\n' + ' '.join(nm_cmd)
    nm = subprocess.Popen(nm_cmd, stdout=subprocess.PIPE)
    output = subprocess.check_output(cxxfilt_tool, stdin=nm.stdout)
    nm.wait()
    for line in iter(output.splitlines()):
      (hex_size, t, sym_name) = line.split(' ', 2)
      # Only track defined and non-BSS symbols.
      if t != 'U' and t.upper() != 'B':
        info = SymbolInfo(bin_name, sym_name, t, int(hex_size, 16))
        # For local symbols, tack the library name on as a prefix.
        # That should still match the regexes later.
        if is_local(t):
          key = bin_name + '$' + sym_name
        else:
          key = sym_name
        # The same library can have the same local symbol. Just sum up sizes.
        if key in results:
          old = results[key]
          old.size = old.size + info.size
        else:
          results[key] = info
    return results

  @classmethod
  def setUpClass(cls):
    nm_tool = sys.argv[1]
    cxxfilt_tool = sys.argv[2]
    host_binaries = glob.glob(sys.argv[3])
    target_binary = sys.argv[4]
    print 'Getting symbol info from %s (host) and %s (target)' % (
        sys.argv[3], sys.argv[4])
    assert host_binaries, ('Did not glob any binaries from: ' % sys.argv[3])
    for binary in host_binaries:
      cls.unpruned_symbols = merge_symbols(cls.unpruned_symbols,
                                           cls.get_symbol_info(nm_tool,
                                                               cxxfilt_tool,
                                                               binary))
    cls.pruned_symbols = cls.get_symbol_info(nm_tool,
                                             cxxfilt_tool,
                                             target_binary)
    # Do an early check that these aren't stripped binaries.
    assert cls.unpruned_symbols, 'No symbols from host?'
    assert cls.pruned_symbols, 'No symbols from target?'

  def size_of_matching_syms(self, sym_regex, sym_infos):
    # Check if a given sym_infos has symbols matching sym_regex, and
    # return the total size of all matching symbols.
    total = 0
    for sym_name, sym_info in sym_infos.iteritems():
      if re.search(sym_regex, sym_info.sym_name):
        total += sym_info.size
    return total

  def test_prunedNotFullyStripped(self):
    """Make sure that the test isn't accidentally passing.

    The test can accidentally pass if the translator is stripped of symbols.
    Then it would look like everything is pruned out.  Look for a symbol
    that's guaranteed not to be pruned out.
    """
    pruned = self.size_of_matching_syms('main',
                                        TestTranslatorPruned.pruned_symbols)
    self.assertNotEqual(pruned, 0)

  def test_didPrune(self):
    """Check for classes/namespaces/symbols that we have intentionally pruned.

    Check that the symbols are not present anymore in the translator,
    and check that the symbols actually do exist in the developer tools.
    That prevents the test from accidentally passing if the symbols
    have been renamed to something else.
    """
    total = 0
    pruned_list = [
        'LLParser', 'LLLexer',
        'MCAsmParser', '::AsmParser',
        'ARMAsmParser', 'X86AsmParser',
        'ELFAsmParser', 'COFFAsmParser', 'DarwinAsmParser',
        'MCAsmLexer', '::AsmLexer',
        # Gigantic Asm MatchTable (globbed for all targets),
        'MatchTable',
        # Pruned out PBQP mostly, except the getCustomPBQPConstraints virtual.
        'RegAllocPBQP', 'PBQP::RegAlloc',
        # Can only check *InstPrinter::print*, not *::getRegisterName():
        # https://code.google.com/p/nativeclient/issues/detail?id=3326
        'ARMInstPrinter::print', 'X86.*InstPrinter::print',
        # Currently pruned by hacking Triple.h. That covers most things,
        # but not all.  E.g., container-specific relocation handling.
        '.*MachObjectWriter', 'TargetLoweringObjectFileMachO',
        'MCMachOStreamer', '.*MCAsmInfoDarwin',
        '.*COFFObjectWriter', 'TargetLoweringObjectFileCOFF',
        '.*COFFStreamer', '.*AsmInfoGNUCOFF',
        # This is not pruned out: 'MCSectionMachO', 'MCSectionCOFF',
        # 'MachineModuleInfoMachO', ...
        ]
    for sym_regex in pruned_list:
      unpruned = self.size_of_matching_syms(
          sym_regex, TestTranslatorPruned.unpruned_symbols)
      pruned = self.size_of_matching_syms(
          sym_regex, TestTranslatorPruned.pruned_symbols)
      self.assertNotEqual(unpruned, 0, 'Unpruned never had ' + sym_regex)
      self.assertEqual(pruned, 0, 'Pruned still has ' + sym_regex)
      # Bytes pruned is approximate since the host build is different
      # from the target build (different inlining / optimizations).
      print 'Pruned out approx %d bytes worth of %s symbols' % (unpruned,
                                                                sym_regex)
      total += unpruned
    print 'Total %d bytes' % total


if __name__ == '__main__':
  if len(sys.argv) != 5:
    print 'Usage: %s <nm_tool> <cxxfilt_tool> <unpruned_host_binary> <pruned_target_binary>'
    sys.exit(1)
  suite = unittest.TestLoader().loadTestsFromTestCase(TestTranslatorPruned)
  result = unittest.TextTestRunner(verbosity=2).run(suite)
  if result.wasSuccessful():
    sys.exit(0)
  else:
    sys.exit(1)
