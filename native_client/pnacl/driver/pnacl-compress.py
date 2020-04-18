#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Uses llvm tool pnacl-bccompress to add abbreviations into the input file.
# It runs pnacl-bccompress multiple times, using a hill-climbing solution
# to try and find a good local minima for file size.

from driver_env import env
from driver_log import Log
import driver_tools
import pathtools
import os
import shutil

EXTRA_ENV = {
  'INPUTS'             : '',
  'OUTPUT'             : '',
  'MAX_ATTEMPTS'       : '25',
  'RETRIES'            : '3',
  'SUFFIX'             :'-c',
  'VERBOSE'            : '0',
}

PrepPatterns = [
    ( ('-o','(.*)'),     "env.set('OUTPUT', pathtools.normalize($0))"),
    ( '--max-attempts=(.*)', "env.set('MAX_ATTEMPTS', $0)"),
    ( '--retries=(.*)',  "env.set('RETRIES', $0)"),
    ( ('--suffix','(.*)'), "env.set('SUFFIX', $0)"),
    ( '--verbose',       "env.set('VERBOSE', '1')"),
    ( '-v',               "env.set('VERBOSE', '1')"),
    ( '(-.*)',           driver_tools.UnrecognizedOption),
    ( '(.*)',            "env.append('INPUTS', pathtools.normalize($0))"),
]


def Compress(f_input, f_output):
  """ Hill climb to smallest file.

      This code calls pnacl-compress multiple times to attempt to
      compress the file. That tool works by adding abbreviations that
      have a good likelyhood of shrinking the bitcode file. Unfortunately,
      any time abbreviations are added to PNaCl bitcode files, they can
      get larger because the addition of more abbreviations will require
      more bits to save abbreviation indices, resulting in the file
      actually increasing in size.

      To mitigate this, this driver hill climbs assuming that there
      may be local minima that are the best solution. Hence, anytime
      a local minima is reached, an additional number of attempts
      to see if we can find a smaller bitcode file, which implies
      you are moving closer to another local minima.
  """

  verbose = env.getbool('VERBOSE')

  # Number of times we will continue to retry after finding local
  # minimum file size.
  #    max_retry_count: The maximum number of retries.
  #    retry_count: The number of retries left before we give up.
  max_retry_count = int(env.getone('RETRIES'))
  retry_count = max_retry_count
  if max_retry_count < 1:
    Log.Fatal("RETRIES must be >= 1")

  # The suffix to append to the input file, to generate intermediate files.
  #    test_suffix: The prefix of the suffix.
  #    test_index: The index of the current test file (appened to test_suffix).
  test_suffix = env.getone('SUFFIX')
  test_index = 1

  # The maximum number of times we will attempt to compress the file before
  # giving up.
  max_attempts = int(env.getone('MAX_ATTEMPTS'))
  if max_attempts < 1:
    Log.Fatal("MAX_ATTEMPTS must be >= 1")

  # The result of the last attempt to compress a file.
  #    last_file: The name of the file
  #    last_size: The number of bytes in last_file.
  #    last_saved: True if we did not remove last_file.
  last_file = f_input
  last_size = pathtools.getsize(f_input)
  last_saved = True

  # Keeps track of the current best compressed file.
  current_smallest_file = last_file
  current_smallest_size = last_size

  while max_attempts > 0 and retry_count > 0:

    next_file = f_input + test_suffix + str(test_index)
    if verbose:
      print "Compressing %s: %s bytes" % (last_file, last_size)

    driver_tools.Run('"${PNACL_COMPRESS}" ' + last_file + ' -o ' + next_file)
    next_size = pathtools.getsize(next_file)
    if not last_saved:
      os.remove(last_file)

    if next_size < current_smallest_size:
      old_file = current_smallest_file
      current_smallest_file = next_file
      current_smallest_size = next_size
      if (f_input != old_file):
        os.remove(old_file)
      retry_count = max_retry_count
      next_saved = True
    else:
      next_saved = False
      retry_count -= 1
    last_file = next_file
    last_size = next_size
    last_saved = next_saved
    max_attempts -= 1
    test_index += 1

  # Install results.
  if verbose:
    print "Compressed  %s: %s bytes" % (last_file, last_size)
    print "Best        %s: %s bytes" % (current_smallest_file,
                                        current_smallest_size)
  if not last_saved:
    os.remove(last_file)

  if (f_input == f_output):
    if (f_input == current_smallest_file): return
    # python os.rename/shutil.move on Windows will raise an error when
    # dst already exists, and f_input already exists.
    f_temp = f_input + test_suffix + "0"
    shutil.move(f_input, f_temp)
    shutil.move(current_smallest_file, f_input)
    os.remove(f_temp)
  elif f_input == current_smallest_file:
    shutil.copyfile(current_smallest_file, f_output)
  else:
    shutil.move(current_smallest_file, f_output)


def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PrepPatterns)

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  for path in inputs + [output]:
    driver_tools.CheckPathLength(path)

  if len(inputs) != 1:
    Log.Fatal('Can only have one input')
  f_input = inputs[0]

  # Allow in-place file changes if output isn't specified.
  if output != '':
    f_output = output
  else:
    f_output = f_input

  Compress(f_input, f_output)
  return 0


def get_help(unused_argv):
  script = env.getone('SCRIPT_NAME')
  return """Usage: %s <options> in-file
  This tool compresses a pnacl bitcode (PEXE) file. It does so by
  generating a series of intermediate files. Each file represents
  an attempt to compress the previous file in the series. Uses
  hill-climbing to find the smallest file to use, and sets the
  output file to the best found case.

  The options are:
  -h --help                 Display this output
  -o <file>                 Place the output into <file>. Otherwise, the
                            input file is modified in-place.
  --max-attempts=N          Maximum number of attempts to reduce file size.
  --retries=N               Number of additional attempts to try after
                            a local minimum is found before quiting.
  --suffix XX               Create intermediate compressed files by adding
                            suffix XXN (where N is a number).
  -v --verbose              Show generated intermediate files and corresponding
                            sizes.
""" % script
