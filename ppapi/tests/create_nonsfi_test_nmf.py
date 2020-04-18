#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple tool to generate NMF file by just reformatting given arguments.

This tool is similar to native_client_sdk/src/tools/create_nmf.py.
create_nmf.py handles most cases, with the exception of Non-SFI nexes.
create_nmf.py tries to auto-detect nexe and pexe types based on their contents,
but it does not work for Non-SFI nexes (which don't have a marker to
distinguish them from SFI nexes).

This script simply reformats the command line arguments into NMF JSON format.
"""

import argparse
import collections
import json
import logging
import os
import sys

_FILES_KEY = 'files'
_PORTABLE_KEY = 'portable'
_PROGRAM_KEY = 'program'
_URL_KEY = 'url'

def ParseArgs():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--program', metavar='FILE', help='Main program nexe')
  parser.add_argument(
      '--arch', metavar='ARCH', choices=('x86-32', 'x86-64', 'arm'),
      help='The archtecture of main program nexe')
  # To keep compatibility with create_nmf.py, we use -x and --extra-files
  # as flags.
  parser.add_argument(
      '-x', '--extra-files', action='append', metavar='KEY:FILE', default=[],
      help=('Add extra key:file tuple to the "files" '
            'section of the .nmf'))
  parser.add_argument(
      '--output', metavar='FILE', help='Path to the output nmf file.')

  return parser.parse_args()


def BuildNmfMap(root_path, program, arch, extra_files):
  """Build simple map representing nmf json."""
  nonsfi_key = arch + '-nonsfi'
  result = {
    _PROGRAM_KEY: {
      nonsfi_key: {
        # The program path is relative to the root_path.
        _URL_KEY: os.path.relpath(program, root_path)
      },
    }
  }

  if extra_files:
    files = {}
    for named_file in extra_files:
      name, path = named_file.split(':', 1)
      files[name] = {
        _PORTABLE_KEY: {
          # Note: use path as is, unlike program path.
          _URL_KEY: path
        }
      }
    if files:
      result[_FILES_KEY] = files

  return result


def OutputNmf(nmf_map, output_path):
  """Writes the nmf to an output file at given path in JSON format."""
  with open(output_path, 'w') as output:
    json.dump(nmf_map, output, indent=2)


def main():
  args = ParseArgs()
  if not args.program:
    logging.error('--program is not specified.')
    sys.exit(1)
  if not args.arch:
    logging.error('--arch is not specified.')
    sys.exit(1)
  if not args.output:
    logging.error('--output is not specified.')
    sys.exit(1)

  nmf_map = BuildNmfMap(os.path.dirname(args.output),
                        args.program, args.arch, args.extra_files)
  OutputNmf(nmf_map, args.output)


if __name__ == '__main__':
  main()
