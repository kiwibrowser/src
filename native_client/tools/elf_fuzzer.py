#!/usr/bin/python

# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module implements a fuzzer for sel_ldr's ELF parsing / NaCl
module loading functions.

The fuzzer takes as arguments a pre-built nexe and sel_ldr, and will
randomly modify a copy of the nexe and run sel_ldr with the -F flag.
If/when sel_ldr crashes, the copy of the nexe is saved.
"""

from __future__ import with_statement  # pre-2.6

import getopt
import os
import random
import re
import signal
import subprocess
import sys
import tempfile

import elf

max_bytes_to_fuzz = 16
default_progress_period = 64

def uniform_fuzz(input_string, nbytes_max):
  nbytes = random.randint(1, nbytes_max)  # fuzz at least one byte

  # pick n distinct values from [0... len(input_string)) uniformly and
  # without replacement.
  targets = random.sample(xrange(len(input_string)), nbytes)
  targets.sort()

  # each entry of keepsies is a tuple (a-1,b) of indices indicating
  # the non-fuzzed substrings of input_string.
  keepsies = zip([-1] + targets,
                 targets + [len(input_string)])

  # the map is essentially a generator of keepsie substrings followed
  # by a random byte.  joined together -- and throwing away the extra,
  # trailing random byte -- is the fuzzed string.
  return ''.join(input_string[subrange[0] + 1 : subrange[1]] +
                 chr(random.randint(0, 255))
                 for subrange in keepsies)[:-1]


def simple_fuzz(nexe_elf):
  orig = nexe_elf.elf_str
  start_offset = nexe_elf.ehdr.phoff
  length = nexe_elf.ehdr.phentsize * nexe_elf.ehdr.phnum
  end_offset = start_offset + length

  return (orig[:start_offset] +
          uniform_fuzz(orig[start_offset
                            :end_offset],
                       max_bytes_to_fuzz) +
          orig[end_offset:])


def genius_fuzz(nexe_elf):
  print >>sys.stderr, 'Genius fuzzer not implemented yet.'
  # parse as phdr and use a distribution that concentrates on certain fields
  sys.exit(1 + hash(nexe_elf))  # ARGSUSED


available_fuzzers = {
    'simple' : simple_fuzz,
    'genius' : genius_fuzz,
}


def usage(stream):
  print >>stream, """\
Usage: elf_fuzzer.py [-d destination_dir]
                     [-D destination_for_log_fatal]
                     [-f fuzzer]
                     [-i iterations]
                     [-m max_bytes_to_fuzz]
                     [-n nexe_original]
                     [-p progress_output_period]
                     [-s sel_ldr]
                     [-S seed_string_for_rng]

 -d: Directory in which fuzzed files that caused core dumps are saved.
     Default: "."
 -D: Directory for saving crashes from LOG_FATAL errors.  Default: discarded.
 -f: Fuzzer to use.  Available fuzzers are:
       %s
 -i: Number of iteration to fuzz.  Default: -1 (infinite).
     For use as a large test, set to a finite value.
 -m: Maximum number of bytes to change.  A random choice of one to this
     number of bytes in the fuzz template's program header will be replaced
     with a random value.
 -n: Nexes to fuzz.  Multiple nexes may be specified by using -n repeatedly,
     in which case each will be used in turn as the fuzz template.
 -p: Progress indicator period.  Print a character for every this many fuzzing
     runs.  Requires verbosity to be at least 1.  Default is %d.
 -S: Seed_string_for_rng is used to seed the random module's random number
     generator; any string will do -- it is hashed.
""" % (', '.join(available_fuzzers.keys()), default_progress_period)


def choose_progress_char(num_saved):
  return '0123456789abcdef'[num_saved % 16]


def main(argv):
  global max_bytes_to_fuzz

  sel_ldr_path = None
  nexe_path = []
  dest_dir = '.'
  dest_fatal_dir = None  # default: do not save
  iterations = -1
  fuzzer = 'simple'
  verbosity = 0
  progress_period = default_progress_period
  progress_char = '.'

  num_saved = 0

  try:
    opt_list, args = getopt.getopt(argv[1:], 'd:D:f:i:m:n:p:s:S:v')
  except getopt.error, e:
    print >>sys.stderr, e
    usage(sys.stderr)
    return 1

  for (opt, val) in opt_list:
    if opt == '-d':
      dest_dir = val
    elif opt == '-D':
      dest_fatal_dir = val
    elif opt == '-f':
      if available_fuzzers.has_key(val):
        fuzzer = val
      else:
        print >>sys.stderr, 'No fuzzer:', val
        usage(sys.stderr)
        return 1
    elif opt == '-i':
      iterations = long(val)
    elif opt == '-m':
      max_bytes_to_fuzz = int(val)
    elif opt == '-n':
      nexe_path.append(val)
    elif opt == '-p':
      progress_period = int(val)
    elif opt == '-s':
      sel_ldr_path = val
    elif opt == '-S':
      random.seed(val)
    elif opt == '-v':
      verbosity = verbosity + 1
    else:
      print >>sys.stderr, 'Option', opt, 'not understood.'
      return -1

  if progress_period <= 0:
    print >>sys.stderr, 'verbose progress indication period must be positive.'
    return 1

  if not nexe_path:
    print >>sys.stderr, 'No nexe specified.'
    return 2
  if sel_ldr_path is None:
    print >>sys.stderr, 'No sel_ldr specified.'
    return 3

  if verbosity > 0:
    print 'sel_ldr is at', sel_ldr_path
    print 'nexe prototype(s) are at', nexe_path

  nfa = re.compile(r'LOG_FATAL abort exit$')

  which_nexe = 0

  while iterations != 0:
    nexe_bytes = open(nexe_path[which_nexe % len(nexe_path)]).read()
    nexe_elf = elf.Elf(nexe_bytes)

    fd, path = tempfile.mkstemp()
    try:
      fstream = os.fdopen(fd, 'w')
      fuzzed_bytes = available_fuzzers[fuzzer](nexe_elf)
      fstream.write(fuzzed_bytes)
      fstream.close()

      cmd_arg_list = [ sel_ldr_path,
                       '-F',
                       '--', path]

      p = subprocess.Popen(cmd_arg_list,
                           stdin = subprocess.PIPE,  # no /dev/null on windows
                           stdout = subprocess.PIPE,
                           stderr = subprocess.PIPE)
      (out_data, err_data) = p.communicate(None)

      if p.returncode < 0:
        if verbosity > 1:
          print 'sel_ldr exited with status', p.returncode, ', output.'
          print 79 * '-'
          print 'standard output'
          print 79 * '-'
          print out_data
          print 79 * '-'
          print 'standard error'
          print 79 * '-'
          print err_data
        elif verbosity > 0:
          os.write(1, '*')

        if (os.WTERMSIG(-p.returncode) != signal.SIGABRT or
            nfa.search(err_data) == None):
          with os.fdopen(tempfile.mkstemp(dir=dest_dir)[0], 'w') as f:
            f.write(fuzzed_bytes)

          # this is a one-liner alternative, relying on the dtor of
          # file-like object to handle the flush/close.  assumption
          # here as with the 'with' statement version: write errors
          # would cause an exception.
          #
          # os.fdopen(tempfile.mkstemp(dir=dest_dir)[0],
          #           'w').write(fuzzed_bytes)
          num_saved = num_saved + 1
          progress_char = choose_progress_char(num_saved)
        else:
          if dest_fatal_dir is not None:
            with os.fdopen(tempfile.mkstemp(dir=dest_fatal_dir)[0], 'w') as f:
              f.write(fuzzed_bytes)
            num_saved = num_saved + 1
            progress_char = choose_progress_char(num_saved)
          elif verbosity > 1:
            print 'LOG_FATAL exit, not saving'
    finally:
      os.unlink(path)

    if iterations > 0:
      iterations = iterations - 1
    if verbosity > 0 and which_nexe % progress_period == 0:
      os.write(1, progress_char)

    which_nexe = which_nexe + 1

  print 'A total of', num_saved, 'nexes caused sel_ldr to exit with a signal.'


if __name__ == '__main__':
  sys.exit(main(sys.argv))
