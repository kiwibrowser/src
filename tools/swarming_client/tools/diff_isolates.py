#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Perform a recursive diff on two isolated file hashes."""

import argparse
import ast
import os
import json
import shutil
import subprocess
import sys
import tempfile

CLIENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, CLIENT_DIR)

from utils import net


def download_isolated_file(h, workdir, isolate_server, namespace):
  """Download the isolated file with the given hash and return its contents."""
  dst = os.path.join(workdir, h)
  if not os.path.isfile(dst):
    subprocess.check_call(['python', 'isolateserver.py', 'download',
                           '-I', isolate_server,
                           '--namespace', namespace,
                           '-t', workdir,
                           '-f', h, h],
                          cwd=CLIENT_DIR)
  with open(dst) as f:
    return f.read()


def get_and_parse_isolated_file(h, workdir, isolate_server, namespace):
  """Download and parse the isolated file with the given hash."""
  contents = download_isolated_file(h, workdir, isolate_server, namespace)
  return ast.literal_eval(contents)


def build_recursive_isolate_structure(h, workdir, isolate_server, namespace):
  """Build a recursive structure from possibly nested isolated files.

  Replaces isolated hashes in the 'includes' list with the contents of the
  associated isolated file.
  """
  rv = get_and_parse_isolated_file(h, workdir, isolate_server, namespace)
  rv['includes'] = sorted(
    build_recursive_isolate_structure(i, workdir, isolate_server, namespace)
    for i in rv.get('includes', [])
  )
  return rv


def write_summary_file(h, workdir, isolate_server, namespace):
  """Write a flattened JSON summary file for the given isolated hash."""
  s = build_recursive_isolate_structure(h, workdir, isolate_server, namespace)
  filename = os.path.join(workdir, 'summary_%s.json' % h)
  with open(filename, 'wb') as f:
    json.dump(s, f, sort_keys=True, indent=2)
  return filename


def diff_isolates(h1, h2, workdir, difftool, isolate_server, namespace):
  """Flatten the given isolated hashes and diff them."""
  f1 = write_summary_file(h1, workdir, isolate_server, namespace)
  f2 = write_summary_file(h2, workdir, isolate_server, namespace)
  return subprocess.call([difftool, f1, f2])


def main():
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  parser.add_argument('hash1')
  parser.add_argument('hash2')
  parser.add_argument('--difftool', '-d', default='diff',
                      help='Diff tool to use.')
  parser.add_argument('--isolate-server', '-I',
                      default=os.environ.get('ISOLATE_SERVER', ''),
                      help='URL of the Isolate Server to use. Defaults to the'
                           'environment variable ISOLATE_SERVER if set. No '
                           'need to specify https://, this is assumed.')
  parser.add_argument('--namespace', default='default-gzip',
                      help='The namespace to use on the Isolate Server, '
                           'default: %(default)s')
  parser.add_argument('--workdir', '-w',
                      help='Working directory to use. If not specified, a '
                           'tmp dir is created.')
  args = parser.parse_args()

  if not args.isolate_server:
    parser.error('--isolate-server is required.')
  try:
    args.isolate_server = net.fix_url(args.isolate_server)
  except ValueError as e:
    parser.error('--isolate-server %s' % e)

  using_tmp_dir = False
  if not args.workdir:
    using_tmp_dir = True
    args.workdir = tempfile.mkdtemp(prefix='diff_isolates')
  else:
    args.workdir = os.path.abspath(args.workdir)
  if not os.path.isdir(args.workdir):
    os.makedirs(args.workdir)

  try:
    return diff_isolates(args.hash1, args.hash2, args.workdir, args.difftool,
                         args.isolate_server, args.namespace)
  finally:
    if using_tmp_dir:
      shutil.rmtree(args.workdir)


if __name__ == '__main__':
  sys.exit(main())
