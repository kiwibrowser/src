#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script runs a test program on a remote host via SSH.  It copies
input files to the remote host using rsync.

Arguments to the test program may be explicitly marked as filenames so
that the files will be copied.

For example, "./sel_ldr -B irt.nexe test_prog.nexe" should be written
as follows so that the input files get copied:

  run_test_via_ssh.py --host=foo --subdir=bar \
      -f ./sel_ldr \
      -a -B \
      -f irt.nexe \
      -f test_prog.nexe
"""

import optparse
import os
import subprocess
import sys


def ShellEscape(value):
  return '"%s"' % (
      value
      .replace('\\', '\\\\')
      .replace('"', '\\"')
      .replace('$', '\\$')
      .replace('`', '\\`'))


def Main():
  input_files = []
  command_args = []

  def AddLiteralArg(option, opt, value, parser):
    command_args.append(ShellEscape(value))

  def AddFileArg(option, opt, value, parser):
    # We do not attempt to preserve the relative locations of the
    # input files.  We copy all the input files into the same
    # directory on the remote host.  We prepend "./" in case the
    # filename is used as an executable name, to prevent it from being
    # looked up in PATH.
    command_args.append('./' + os.path.basename(value))
    input_files.append(value)

  def AddFileDep(option, opt, value, parser):
    input_files.append(value)

  parser = optparse.OptionParser('%prog [options]\n\n' + __doc__.strip())
  parser.add_option('--host', type='string', dest='host',
                    metavar='[USER@]HOSTNAME',
                    help='Destination hostname, to be passed to SSH.')
  parser.add_option('--subdir', type='string', dest='subdir',
                    metavar='PATH',
                    help='Subdirectory on the remote host to copy files to.')
  parser.add_option('-a', action='callback', type='string',
                    callback=AddLiteralArg, metavar='STRING',
                    help='Add a string literal argument.')
  parser.add_option('-f', action='callback', type='string',
                    callback=AddFileArg, metavar='FILENAME',
                    help=('Add an input file argument.  The file will be '
                          'copied to the remote host, and the filename will be '
                          'added as a command line argument.'))
  parser.add_option('-F', action='callback', type='string',
                    callback=AddFileDep, metavar='FILENAME',
                    help=('Add an input file argument, without adding the '
                          'filename as a command line argument.'))
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Produce extra output')
  options, parsed_args = parser.parse_args()
  if len(parsed_args) != 0:
    parser.error('Unexpected arguments: %r' % parsed_args)
  if options.host is None:
    parser.error('No host argument given')
  if options.subdir is None:
    parser.error('No subdir argument given')

  # Copy the input files to remote host.
  rsync_opts = []
  if options.verbose:
    rsync_opts.append('-v')
  ssh_dest_dir = '%s:%s/' % (options.host, options.subdir)
  subprocess.check_call(['rsync', '-a', '-z'] + rsync_opts + ['--']
                        + input_files + [ssh_dest_dir])

  # Run the test command on the remote host.
  remote_cmd = ' '.join(command_args)
  if options.verbose:
    print remote_cmd
  full_remote_cmd = 'cd %s && %s' % (options.subdir, remote_cmd)
  rc = subprocess.call(['ssh', options.host, full_remote_cmd])
  sys.exit(rc)


if __name__ == '__main__':
  Main()
