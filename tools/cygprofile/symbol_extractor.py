# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities to get and manipulate symbols from a binary."""

import collections
import logging
import os
import re
import subprocess
import sys

import cygprofile_utils

sys.path.insert(
    0, os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                    'build', 'android'))

from pylib import constants
from pylib.constants import host_paths

_MAX_WARNINGS_TO_PRINT = 200

SymbolInfo = collections.namedtuple('SymbolInfo', ('name', 'offset', 'size',
                                                   'section'))

# Unfortunate global variable :-/
_arch = 'arm'


def SetArchitecture(arch):
  """Set the architecture for binaries to be symbolized."""
  global _arch
  _arch = arch


def _FromObjdumpLine(line):
  """Create a SymbolInfo by parsing a properly formatted objdump output line.

  Args:
    line: line from objdump

  Returns:
    An instance of SymbolInfo if the line represents a symbol, None otherwise.
  """
  # All of the symbol lines we care about are in the form
  # 0000000000  g    F   .text.foo     000000000 [.hidden] foo
  # where g (global) might also be l (local) or w (weak).
  parts = line.split()
  if len(parts) < 6 or parts[2] != 'F':
    return None

  assert len(parts) == 6 or (len(parts) == 7 and parts[5] == '.hidden')
  accepted_scopes = set(['g', 'l', 'w'])
  assert parts[1] in accepted_scopes

  offset = int(parts[0], 16)
  section = parts[3]
  size = int(parts[4], 16)
  name = parts[-1].rstrip('\n')
  # Forbid ARM mapping symbols and other unexpected symbol names, but allow $
  # characters in a non-initial position, which can appear as a component of a
  # mangled name, e.g. Clang can mangle a lambda function to:
  # 02cd61e0 l     F .text  000000c0 _ZZL11get_globalsvENK3$_1clEv
  # The equivalent objdump line from GCC is:
  # 0325c58c l     F .text  000000d0 _ZZL11get_globalsvENKUlvE_clEv
  assert re.match('^[a-zA-Z0-9_.][a-zA-Z0-9_.$]*$', name)
  return SymbolInfo(name=name, offset=offset, section=section, size=size)


def _SymbolInfosFromStream(objdump_lines):
  """Parses the output of objdump, and get all the symbols from a binary.

  Args:
    objdump_lines: An iterable of lines

  Returns:
    A list of SymbolInfo.
  """
  symbol_infos = []
  for line in objdump_lines:
    symbol_info = _FromObjdumpLine(line)
    if symbol_info is not None:
      symbol_infos.append(symbol_info)
  return symbol_infos


def SymbolInfosFromBinary(binary_filename):
  """Runs objdump to get all the symbols from a binary.

  Args:
    binary_filename: path to the binary.

  Returns:
    A list of SymbolInfo from the binary.
  """
  command = (host_paths.ToolPath('objdump', _arch), '-t', '-w', binary_filename)
  p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE)
  try:
    result = _SymbolInfosFromStream(p.stdout)
    return result
  finally:
    p.stdout.close()
    p.wait()


def GroupSymbolInfosByOffset(symbol_infos):
  """Create a dict {offset: [symbol_info1, ...], ...}.

  As several symbols can be at the same offset, this is a 1-to-many
  relationship.

  Args:
    symbol_infos: iterable of SymbolInfo instances

  Returns:
    a dict {offset: [symbol_info1, ...], ...}
  """
  offset_to_symbol_infos = collections.defaultdict(list)
  for symbol_info in symbol_infos:
    offset_to_symbol_infos[symbol_info.offset].append(symbol_info)
  return dict(offset_to_symbol_infos)


def GroupSymbolInfosByName(symbol_infos):
  """Create a dict {name: [symbol_info1, ...], ...}.

  A symbol can have several offsets, this is a 1-to-many relationship.

  Args:
    symbol_infos: iterable of SymbolInfo instances

  Returns:
    a dict {name: [symbol_info1, ...], ...}
  """
  name_to_symbol_infos = collections.defaultdict(list)
  for symbol_info in symbol_infos:
    name_to_symbol_infos[symbol_info.name].append(symbol_info)
  return dict(name_to_symbol_infos)


def CreateNameToSymbolInfo(symbol_infos):
  """Create a dict {name: symbol_info, ...}.

  Args:
    symbol_infos: iterable of SymbolInfo instances

  Returns:
    a dict {name: symbol_info, ...}
    If a symbol name corresponds to more than one symbol_info, the symbol_info
    with the lowest offset is chosen.
  """
  # TODO(lizeb,pasko): move the functionality in this method into
  # check_orderfile.
  symbol_infos_by_name = {}
  warnings = cygprofile_utils.WarningCollector(_MAX_WARNINGS_TO_PRINT)
  for infos in GroupSymbolInfosByName(symbol_infos).itervalues():
    first_symbol_info = min(infos, key=lambda x:x.offset)
    symbol_infos_by_name[first_symbol_info.name] = first_symbol_info
    if len(infos) > 1:
      warnings.Write('Symbol %s appears at %d offsets: %s' %
                     (first_symbol_info.name,
                      len(infos),
                      ','.join([hex(x.offset) for x in infos])))
  warnings.WriteEnd('symbols at multiple offsets.')
  return symbol_infos_by_name


def DemangleSymbol(mangled_symbol):
  """Return the demangled form of mangled_symbol."""
  cmd = [host_paths.ToolPath("c++filt", _arch)]
  process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
  demangled_symbol, _ = process.communicate(mangled_symbol + '\n')
  return demangled_symbol
