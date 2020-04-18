# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import subprocess
import threading

_CHROME_SRC = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)
_LLVM_SYMBOLIZER_PATH = os.path.join(
    _CHROME_SRC, 'third_party', 'llvm-build', 'Release+Asserts', 'bin',
    'llvm-symbolizer')

_BINARY = re.compile(r'0b[0,1]+')
_HEX = re.compile(r'0x[0-9,a-e]+')
_OCTAL = re.compile(r'0[0-7]+')

_UNKNOWN = '<UNKNOWN>'


def _CheckValidAddr(addr):
  """
  Check whether the addr is valid input to llvm symbolizer.
  Valid addr has to be octal, binary, or hex number.

  Args:
    addr: addr to be entered to llvm symbolizer.

  Returns:
    whether the addr is valid input to llvm symbolizer.
  """
  return _HEX.match(addr) or _OCTAL.match(addr) or _BINARY.match(addr)


class LLVMSymbolizer(object):
  def __init__(self):
    """Create a LLVMSymbolizer instance that interacts with the llvm symbolizer.

    The purpose of the LLVMSymbolizer is to get function names and line
    numbers of an address from the symbols library.
    """
    self._llvm_symbolizer_subprocess = None
    # Allow only one thread to call GetSymbolInformation at a time.
    self._lock = threading.Lock()

  def Start(self):
    """Start the llvm symbolizer subprocess.

    Create a subprocess of the llvm symbolizer executable, which will be used
    to retrieve function names etc.
    """
    if os.path.isfile(_LLVM_SYMBOLIZER_PATH):
      self._llvm_symbolizer_subprocess = subprocess.Popen(
        [_LLVM_SYMBOLIZER_PATH], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    else:
      logging.error('Cannot find llvm_symbolizer here: %s.' %
                    _LLVM_SYMBOLIZER_PATH)
      self._llvm_symbolizer_subprocess = None

  def Close(self):
    """Close the llvm symbolizer subprocess.

    Close the subprocess by closing stdin, stdout and killing the subprocess.
    """
    with self._lock:
      if self._llvm_symbolizer_subprocess:
        self._llvm_symbolizer_subprocess.kill()
        self._llvm_symbolizer_subprocess = None

  def __enter__(self):
    """Start the llvm symbolizer subprocess."""
    self.Start()
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    """Close the llvm symbolizer subprocess."""
    self.Close()

  def GetSymbolInformation(self, lib, addr):
    """Return the corresponding function names and line numbers.

    Args:
      lib: library to search for info.
      addr: address to look for info.

    Returns:
      A list of (function name, line numbers) tuple.
    """
    if (self._llvm_symbolizer_subprocess is None or not lib
        or not _CheckValidAddr(addr) or not os.path.isfile(lib)):
      return [(_UNKNOWN, lib)]

    with self._lock:
      self._llvm_symbolizer_subprocess.stdin.write('%s %s\n' % (lib, addr))
      self._llvm_symbolizer_subprocess.stdin.flush()

      result = []
      # Read till see new line, which is a symbol of end of output.
      # One line of function name is always followed by one line of line number.
      while True:
        line = self._llvm_symbolizer_subprocess.stdout.readline()
        if line != '\n':
          line_numbers = self._llvm_symbolizer_subprocess.stdout.readline()
          result.append(
            (line[:-1],
             line_numbers[:-1]))
        else:
          return result
