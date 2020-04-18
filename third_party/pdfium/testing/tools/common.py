#!/usr/bin/env python
# Copyright 2015 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import glob
import os
import re
import subprocess
import sys

def os_name():
  if sys.platform.startswith('linux'):
    return 'linux'
  if sys.platform.startswith('win'):
    return 'win'
  if sys.platform.startswith('darwin'):
    return 'mac'
  raise Exception('Confused, can not determine OS, aborting.')


def RunCommand(cmd):
  try:
    subprocess.check_call(cmd)
    return None
  except subprocess.CalledProcessError as e:
    return e


def RunCommandPropagateErr(cmd, stdout_has_errors=False,
                           exit_status_on_error=None):
  """Run a command as a subprocess.

  Errors in that subprocess are printed out if it returns an error exit code.

  Args:
    cmd: Command to run as a list of strings.
    stdout_has_errors: Whether to print stdout instead of stderr on an error
        exit.
    exit_status_on_error: If specified, upon an error in the subprocess the
        caller script exits immediately with the given status.
  """
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  output, err = p.communicate()

  if p.returncode:
    PrintErr('\nError when invoking "%s"' % ' '.join(cmd))
    if stdout_has_errors:
      PrintErr(output)

    PrintErr(err)

    if exit_status_on_error is not None:
      sys.exit(exit_status_on_error)

    return None

  return output


# RunCommandExtractHashedFiles returns a tuple: (raised_exception, hashed_files)
# It runs the given command. If it fails it will return an exception and None.
# If it succeeds it will return None and the list of processed files extracted
# from the output of the command. It expects lines in this format:
#    MD5:<path_to_image_file>:<md5_hash_in_hex>
# The returned hashed_files is a list of (file_path, MD5-hash) pairs.
def RunCommandExtractHashedFiles(cmd):
  try:
    output = subprocess.check_output(cmd, universal_newlines=True)
    ret = []
    for line in output.split('\n'):
      line = line.strip()
      if line.startswith("MD5:"):
          ret.append([x.strip() for x in line.lstrip("MD5:").rsplit(":", 1)])
    return None, ret
  except subprocess.CalledProcessError as e:
    return e, None


class DirectoryFinder:
  '''A class for finding directories and paths under either a standalone
  checkout or a chromium checkout of PDFium.'''

  def __init__(self, build_location):
    # |build_location| is typically "out/Debug" or "out/Release".
    # Expect |my_dir| to be .../pdfium/testing/tools.
    self.my_dir = os.path.dirname(os.path.realpath(__file__))
    self.testing_dir = os.path.dirname(self.my_dir)
    if (os.path.basename(self.my_dir) != 'tools' or
        os.path.basename(self.testing_dir) != 'testing'):
      raise Exception('Confused, can not find pdfium root directory, aborting.')
    self.pdfium_dir = os.path.dirname(self.testing_dir)
    # Find path to build directory.  This depends on whether this is a
    # standalone build vs. a build as part of a chromium checkout. For
    # standalone, we expect a path like .../pdfium/out/Debug, but for
    # chromium, we expect a path like .../src/out/Debug two levels
    # higher (to skip over the third_party/pdfium path component under
    # which chromium sticks pdfium).
    self.base_dir = self.pdfium_dir
    one_up_dir = os.path.dirname(self.base_dir)
    two_up_dir = os.path.dirname(one_up_dir)
    if (os.path.basename(two_up_dir) == 'src' and
        os.path.basename(one_up_dir) == 'third_party'):
      self.base_dir = two_up_dir
    self.build_dir = os.path.join(self.base_dir, build_location)
    self.os_name = os_name()

  def ExecutablePath(self, name):
    '''Finds compiled binaries under the build path.'''
    result = os.path.join(self.build_dir, name)
    if self.os_name == 'win':
      result = result + '.exe'
    return result

  def ScriptPath(self, name):
    '''Finds other scripts in the same directory as this one.'''
    return os.path.join(self.my_dir, name)

  def WorkingDir(self, other_components=''):
    '''Places generated files under the build directory, not source dir.'''
    result = os.path.join(self.build_dir, 'gen', 'pdfium')
    if other_components:
      result = os.path.join(result, other_components)
    return result

  def TestingDir(self, other_components=''):
    '''Finds test files somewhere under the testing directory.'''
    result = self.testing_dir
    if other_components:
      result = os.path.join(result, other_components)
    return result


def GetBooleanGnArg(arg_name, build_dir, verbose=False):
  '''Extract the value of a boolean flag in args.gn'''
  cwd = os.getcwd()
  os.chdir(build_dir)
  gn_args_output = subprocess.check_output(
      ['gn', 'args', '.', '--list=%s' % arg_name, '--short'])
  os.chdir(cwd)
  arg_match_output = re.search('%s = (.*)' % arg_name, gn_args_output).group(1)
  if verbose:
    print >> sys.stderr, "Found '%s' for value of %s" % (arg_match_output, arg_name)
  return arg_match_output == 'true'


def PrintWithTime(s):
  """Prints s prepended by a timestamp."""
  print '[%s] %s' % (datetime.datetime.now().strftime("%Y%m%d %H:%M:%S"),
                     s)


def PrintErr(s):
  """Prints s to stderr."""
  print >> sys.stderr, s
