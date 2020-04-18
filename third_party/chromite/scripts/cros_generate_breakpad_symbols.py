# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate minidump symbols for use by the Crash server.

Note: This should be run inside the chroot.

This produces files in the breakpad format required by minidump_stackwalk and
the crash server to dump stack information.

Basically it scans all the split .debug files in /build/$BOARD/usr/lib/debug/
and converts them over using the `dump_syms` programs.  Those plain text .sym
files are then stored in /build/$BOARD/usr/lib/debug/breakpad/.

If you want to actually upload things, see upload_symbols.py.
"""

from __future__ import print_function

import collections
import ctypes
import multiprocessing
import os
import tempfile

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import signals


SymbolHeader = collections.namedtuple('SymbolHeader',
                                      ('cpu', 'id', 'name', 'os',))


def ReadSymsHeader(sym_file):
  """Parse the header of the symbol file

  The first line of the syms file will read like:
    MODULE Linux arm F4F6FA6CCBDEF455039C8DE869C8A2F40 blkid

  https://code.google.com/p/google-breakpad/wiki/SymbolFiles

  Args:
    sym_file: The symbol file to parse

  Returns:
    A SymbolHeader object

  Raises:
    ValueError if the first line of |sym_file| is invalid
  """
  with cros_build_lib.Open(sym_file) as f:
    header = f.readline().split()

  if header[0] != 'MODULE' or len(header) != 5:
    raise ValueError('header of sym file is invalid')

  return SymbolHeader(os=header[1], cpu=header[2], id=header[3], name=header[4])


def GenerateBreakpadSymbol(elf_file, debug_file=None, breakpad_dir=None,
                           strip_cfi=False, num_errors=None,
                           dump_syms_cmd='dump_syms'):
  """Generate the symbols for |elf_file| using |debug_file|

  Args:
    elf_file: The file to dump symbols for
    debug_file: Split debug file to use for symbol information
    breakpad_dir: The dir to store the output symbol file in
    strip_cfi: Do not generate CFI data
    num_errors: An object to update with the error count (needs a .value member)
    dump_syms_cmd: Command to use for dumping symbols.

  Returns:
    The name of symbol file written out.
  """
  assert breakpad_dir
  if num_errors is None:
    num_errors = ctypes.c_int()

  cmd_base = [dump_syms_cmd, '-v']
  if strip_cfi:
    cmd_base += ['-c']
  # Some files will not be readable by non-root (e.g. set*id /bin/su).
  needs_sudo = not os.access(elf_file, os.R_OK)

  def _DumpIt(cmd_args):
    if needs_sudo:
      run_command = cros_build_lib.SudoRunCommand
    else:
      run_command = cros_build_lib.RunCommand
    return run_command(
        cmd_base + cmd_args, redirect_stderr=True, log_stdout_to_file=temp.name,
        error_code_ok=True, debug_level=logging.DEBUG)

  def _CrashCheck(ret, msg):
    if ret < 0:
      logging.PrintBuildbotStepWarnings()
      logging.warning('dump_syms crashed with %s; %s',
                      signals.StrSignal(-ret), msg)

  osutils.SafeMakedirs(breakpad_dir)
  with tempfile.NamedTemporaryFile(dir=breakpad_dir, bufsize=0) as temp:
    if debug_file:
      # Try to dump the symbols using the debug file like normal.
      cmd_args = [elf_file, os.path.dirname(debug_file)]
      result = _DumpIt(cmd_args)

      if result.returncode:
        # Sometimes dump_syms can crash because there's too much info.
        # Try dumping and stripping the extended stuff out.  At least
        # this way we'll get the extended symbols.  http://crbug.com/266064
        _CrashCheck(result.returncode, 'retrying w/out CFI')
        cmd_args = ['-c', '-r'] + cmd_args
        result = _DumpIt(cmd_args)
        _CrashCheck(result.returncode, 'retrying w/out debug')

      basic_dump = result.returncode
    else:
      basic_dump = True

    if basic_dump:
      # If that didn't work (no debug, or dump_syms still failed), try
      # dumping just the file itself directly.
      result = _DumpIt([elf_file])
      if result.returncode:
        # A lot of files (like kernel files) contain no debug information,
        # do not consider such occurrences as errors.
        logging.PrintBuildbotStepWarnings()
        _CrashCheck(result.returncode, 'giving up entirely')
        if 'file contains no debugging information' in result.error:
          logging.warning('no symbols found for %s', elf_file)
        else:
          num_errors.value += 1
          logging.error('dumping symbols for %s failed:\n%s', elf_file,
                        result.error)
        return num_errors.value

    # Move the dumped symbol file to the right place:
    # /build/$BOARD/usr/lib/debug/breakpad/<module-name>/<id>/<module-name>.sym
    header = ReadSymsHeader(temp)
    logging.info('Dumped %s as %s : %s', elf_file, header.name, header.id)
    sym_file = os.path.join(breakpad_dir, header.name, header.id,
                            header.name + '.sym')
    osutils.SafeMakedirs(os.path.dirname(sym_file))
    os.rename(temp.name, sym_file)
    os.chmod(sym_file, 0o644)
    temp.delete = False

  return sym_file


def GenerateBreakpadSymbols(board, breakpad_dir=None, strip_cfi=False,
                            generate_count=None, sysroot=None,
                            num_processes=None, clean_breakpad=False,
                            exclude_dirs=(), file_list=None):
  """Generate symbols for this board.

  If |file_list| is None, symbols are generated for all executables, otherwise
  only for the files included in |file_list|.

  TODO(build):
  This should be merged with buildbot_commands.GenerateBreakpadSymbols()
  once we rewrite cros_generate_breakpad_symbols in python.

  Args:
    board: The board whose symbols we wish to generate
    breakpad_dir: The full path to the breakpad directory where symbols live
    strip_cfi: Do not generate CFI data
    generate_count: If set, only generate this many symbols (meant for testing)
    sysroot: The root where to find the corresponding ELFs
    num_processes: Number of jobs to run in parallel
    clean_breakpad: Should we `rm -rf` the breakpad output dir first; note: we
      do not do any locking, so do not run more than one in parallel when True
    exclude_dirs: List of dirs (relative to |sysroot|) to not search
    file_list: Only generate symbols for files in this list. Each file must be a
      full path (including |sysroot| prefix).
      TODO(build): Support paths w/o |sysroot|.

  Returns:
    The number of errors that were encountered.
  """
  if breakpad_dir is None:
    breakpad_dir = FindBreakpadDir(board)
  if sysroot is None:
    sysroot = cros_build_lib.GetSysroot(board=board)
  if clean_breakpad:
    logging.info('cleaning out %s first', breakpad_dir)
    osutils.RmDir(breakpad_dir, ignore_missing=True, sudo=True)
  # Make sure non-root can write out symbols as needed.
  osutils.SafeMakedirs(breakpad_dir, sudo=True)
  if not os.access(breakpad_dir, os.W_OK):
    cros_build_lib.SudoRunCommand(['chown', '-R', str(os.getuid()),
                                   breakpad_dir])
  debug_dir = FindDebugDir(board)
  exclude_paths = [os.path.join(debug_dir, x) for x in exclude_dirs]
  if file_list is None:
    file_list = []
  file_filter = dict.fromkeys([os.path.normpath(x) for x in file_list], False)

  logging.info('generating breakpad symbols using %s', debug_dir)

  # Let's locate all the debug_files and elfs first along with the debug file
  # sizes.  This way we can start processing the largest files first in parallel
  # with the small ones.
  # If |file_list| was given, ignore all other files.
  targets = []
  for root, dirs, files in os.walk(debug_dir):
    if root in exclude_paths:
      logging.info('Skipping excluded dir %s', root)
      del dirs[:]
      continue

    for debug_file in files:
      debug_file = os.path.join(root, debug_file)
      # Turn /build/$BOARD/usr/lib/debug/sbin/foo.debug into
      # /build/$BOARD/sbin/foo.
      elf_file = os.path.join(sysroot, debug_file[len(debug_dir) + 1:-6])

      if file_filter:
        if elf_file in file_filter:
          file_filter[elf_file] = True
        elif debug_file in file_filter:
          file_filter[debug_file] = True
        else:
          continue

      # Filter out files based on common issues with the debug file.
      if not debug_file.endswith('.debug'):
        continue

      elif debug_file.endswith('.ko.debug'):
        logging.debug('Skipping kernel module %s', debug_file)
        continue

      elif os.path.islink(debug_file):
        # The build-id stuff is common enough to filter out by default.
        if '/.build-id/' in debug_file:
          msg = logging.debug
        else:
          msg = logging.warning
        msg('Skipping symbolic link %s', debug_file)
        continue

      # Filter out files based on common issues with the elf file.
      if not os.path.exists(elf_file):
        # Sometimes we filter out programs from /usr/bin but leave behind
        # the .debug file.
        logging.warning('Skipping missing %s', elf_file)
        continue

      targets.append((os.path.getsize(debug_file), elf_file, debug_file))

  bg_errors = multiprocessing.Value('i')
  if file_filter:
    files_not_found = [x for x, found in file_filter.iteritems() if not found]
    bg_errors.value += len(files_not_found)
    if files_not_found:
      logging.error('Failed to find requested files: %s', files_not_found)

  # Now start generating symbols for the discovered elfs.
  with parallel.BackgroundTaskRunner(GenerateBreakpadSymbol,
                                     breakpad_dir=breakpad_dir,
                                     strip_cfi=strip_cfi,
                                     num_errors=bg_errors,
                                     processes=num_processes) as queue:
    for _, elf_file, debug_file in sorted(targets, reverse=True):
      if generate_count == 0:
        break

      queue.put([elf_file, debug_file])
      if generate_count is not None:
        generate_count -= 1
        if generate_count == 0:
          break

  return bg_errors.value


def FindDebugDir(board):
  """Given a |board|, return the path to the split debug dir for it"""
  sysroot = cros_build_lib.GetSysroot(board=board)
  return os.path.join(sysroot, 'usr', 'lib', 'debug')


def FindBreakpadDir(board):
  """Given a |board|, return the path to the breakpad dir for it"""
  return os.path.join(FindDebugDir(board), 'breakpad')


def main(argv):
  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('--board', default=None,
                      help='board to generate symbols for')
  parser.add_argument('--breakpad_root', type='path', default=None,
                      help='root directory for breakpad symbols')
  parser.add_argument('--exclude-dir', type=str, action='append',
                      default=[],
                      help='directory (relative to |board| root) to not search')
  parser.add_argument('--generate-count', type=int, default=None,
                      help='only generate # number of symbols')
  parser.add_argument('--noclean', dest='clean', action='store_false',
                      default=True,
                      help='do not clean out breakpad dir before running')
  parser.add_argument('--jobs', type=int, default=None,
                      help='limit number of parallel jobs')
  parser.add_argument('--strip_cfi', action='store_true', default=False,
                      help='do not generate CFI data (pass -c to dump_syms)')
  parser.add_argument('file_list', nargs='*', default=None,
                      help='generate symbols for only these files '
                           '(e.g. /build/$BOARD/usr/bin/foo)')

  opts = parser.parse_args(argv)
  opts.Freeze()

  if opts.board is None:
    cros_build_lib.Die('--board is required')

  ret = GenerateBreakpadSymbols(opts.board, breakpad_dir=opts.breakpad_root,
                                strip_cfi=opts.strip_cfi,
                                generate_count=opts.generate_count,
                                num_processes=opts.jobs,
                                clean_breakpad=opts.clean,
                                exclude_dirs=opts.exclude_dir,
                                file_list=opts.file_list)
  if ret:
    logging.error('encountered %i problem(s)', ret)
    # Since exit(status) gets masked, clamp it to 1 so we don't inadvertently
    # return 0 in case we are a multiple of the mask.
    ret = 1

  return ret
