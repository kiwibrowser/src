# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate minidump symbols for use by the Crash server.

This script takes expanded crash symbols published by the Android build, and
converts them to breakpad format.
"""

from __future__ import print_function

import multiprocessing
import os
import re
import zipfile

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.scripts import cros_generate_breakpad_symbols


RELOCATION_PACKER_BIN = 'relocation_packer'

# These regexps match each type of address we have to adjust.
ADDRESS_REGEXPS = (
    re.compile(r'^FUNC ([0-9a-f]+)'),
    re.compile(r'^([0-9a-f]+)'),
    re.compile(r'^PUBLIC ([0-9a-f]+)'),
    re.compile(r'^STACK CFI INIT ([0-9a-f]+)'),
    re.compile(r'^STACK CFI ([0-9a-f]+)'),
)


class OffsetDiscoveryError(Exception):
  """Raised if we can't find the offset after unpacking symbols."""


def FindExpansionOffset(unpack_result):
  """Helper to extract symbols offset from relocation_packer output.

  This can accept and handle both successful and failed unpack command output.

  Will return 0 if no adjustment is needed.

  Args:
    unpack_result: CommandResult from the relocation_packer command.

  Returns:
    Integer offset to adjust symbols by. May be 0.

  Raises:
    OffsetDiscoveryError if the unpack succeeds, but we can't parse the output.
  """
  if unpack_result.returncode != 0:
    return 0

  offset_match = re.search(r'INFO: Expansion +: +(\d+) bytes',
                           unpack_result.output)

  if not offset_match:
    raise OffsetDiscoveryError('No Expansion in: %s' % unpack_result.output)

  # Return offset as a negative number.
  return -int(offset_match.group(1))


def _AdjustLineSymbolOffset(line, offset):
  """Adjust the symbol offset for one line of a breakpad file.

  Args:
    line: One line of the file.
    offset: int to adjust the symbol by.

  Returns:
    The adjusted line, or original line if there is no change.
  """
  for regexp in ADDRESS_REGEXPS:
    m = regexp.search(line)
    if m:
      address = int(m.group(1), 16)

      # We ignore 0 addresses, since the zero's are fillers for unknowns.
      if address:
        address += offset

      # Return the same line with address adjusted.
      return '%s%x%s' % (line[:m.start(1)], address, line[m.end(1):])

  # Nothing recognized, no adjustment.
  return line


def _AdjustSymbolOffset(breakpad_file, offset):
  """Given a breakpad file, adjust the symbols by offset.

  Updates the file in place.

  Args:
    breakpad_file: File to read and update in place.
    offset: Integer to move symbols by.
  """
  logging.info('Adjusting symbols in %s with offset %d.',
               breakpad_file, offset)

  # Keep newlines.
  lines = osutils.ReadFile(breakpad_file).splitlines(True)
  adjusted_lines = [_AdjustLineSymbolOffset(line, offset) for line in lines]
  osutils.WriteFile(breakpad_file, ''.join(adjusted_lines))


def _UnpackGenerateBreakpad(elf_file, *args, **kwargs):
  """Unpack Android relocation symbols, and GenerateBreakpadSymbol

  This method accepts exactly the same arguments as
  cros_generate_breakpad_symbols.GenerateBreakpadSymbol, except that it requires
  elf_file, and fills in dump_sym_cmd.

  Args:
    elf_file: Name of the file to generate breakpad symbols for.
    args: See cros_generate_breakpad_symbols.GenerateBreakpadSymbol.
    kwargs: See cros_generate_breakpad_symbols.GenerateBreakpadSymbol.
  """
  # We try to unpack, and just see if it works. Real failures caused by
  # something other than a binary that's already unpacked will be logged and
  # ignored. We'll notice them when dump_syms fails later (which it will on
  # packed binaries.).
  unpack_cmd = [RELOCATION_PACKER_BIN, '-u', elf_file]
  unpack_result = cros_build_lib.RunCommand(
      unpack_cmd, redirect_stdout=True, error_code_ok=True)

  # If we unpacked, extract the offset, and remember it.
  offset = FindExpansionOffset(unpack_result)

  if offset:
    logging.info('Unpacked relocation symbols for %s with offset %d.',
                 elf_file, offset)

  # Now generate breakpad symbols from the binary.
  breakpad_file = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
      elf_file, *args, **kwargs)

  if offset:
    _AdjustSymbolOffset(breakpad_file, offset)


def GenerateBreakpadSymbols(breakpad_dir, symbols_dir):
  """Generate symbols for all binaries in symbols_dir.

  Args:
    breakpad_dir: The full path in which to write out breakpad symbols.
    symbols_dir: The full path to the binaries to process from.

  Returns:
    The number of errors that were encountered.
  """
  osutils.SafeMakedirs(breakpad_dir)
  logging.info('generating breakpad symbols from %s', symbols_dir)

  num_errors = multiprocessing.Value('i')

  # Now start generating symbols for the discovered elfs.
  with parallel.BackgroundTaskRunner(
      _UnpackGenerateBreakpad,
      breakpad_dir=breakpad_dir,
      num_errors=num_errors) as queue:

    for root, _, files in os.walk(symbols_dir):
      for f in files:
        queue.put([os.path.join(root, f)])

  return num_errors.value


def ProcessSymbolsZip(zip_archive, breakpad_dir):
  """Extract, process, and upload all symbols in a symbols zip file.

  Take the symbols file build artifact from an Android build, process it into
  breakpad format, and upload the results to the ChromeOS crashreporter.
  Significant multiprocessing is done by helper libraries, and a remote swarm
  server is used to reduce processing of duplicate symbol files.

  The symbols files are really expected to be unstripped elf files (or
  libraries), possibly using packed relocation tables. No other file types are
  expected in the zip.

  Args:
    zip_archive: Name of the zip file to process.
    breakpad_dir: Root directory for writing out breakpad files.
  """
  with osutils.TempDir(prefix='extracted-') as extract_dir:
    logging.info('Extracting %s into %s', zip_archive, extract_dir)
    with zipfile.ZipFile(zip_archive, 'r') as zf:
      # We are trusting the contents from a security point of view.
      zf.extractall(extract_dir)

    logging.info('Generate breakpad symbols from %s into %s',
                 extract_dir, breakpad_dir)
    GenerateBreakpadSymbols(breakpad_dir, extract_dir)


def main(argv):
  """Helper method mostly used for manual testing."""

  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('--symbols_file', type='path', required=True,
                      help='Zip file containing')
  parser.add_argument('--breakpad_dir', type='path', default='/tmp/breakpad',
                      help='Root directory for breakpad symbol files.')

  opts = parser.parse_args(argv)
  opts.Freeze()

  ProcessSymbolsZip(opts.symbols_file, opts.breakpad_dir)
