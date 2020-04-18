#!/usr/bin/env python
# coding: utf-8
# Copyright 2012 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Runs hello_world.py, through hello_world.isolate, remotely on a Swarming
slave.

It compiles and archives via 'isolate.py archive', then discard the local files.
After, it triggers and finally collects the results.
"""

import os
import shutil
import subprocess
import sys
import tempfile

# Pylint can't find common.py that's in the same directory as this file.
# pylint: disable=F0401
import common


def main():
  args = common.parse_args(use_isolate_server=True, use_swarming=True)
  try:
    tempdir = tempfile.mkdtemp(prefix=u'hello_world')
    try:
      isolated_hash = common.archive(
          tempdir, args.isolate_server, args.verbose, args.which)

      json_file = os.path.join(tempdir, 'task.json').encode('utf-8')
      common.note('Running on %s' % args.swarming)
      cmd = [
        'swarming.py',
        'trigger',
        '--swarming', args.swarming,
        '--isolate-server', args.isolate_server,
        '--task-name', args.task_name,
        '--dump-json', json_file,
        '--isolated', isolated_hash,
        '--raw-cmd',
      ]
      for k, v in args.dimensions:
        cmd.extend(('--dimension', k, v))
      if args.idempotent:
        cmd.append('--idempotent')
      if args.priority is not None:
        cmd.extend(('--priority', str(args.priority)))
      if args.service_account:
        cmd.extend(('--service-account', args.service_account))

      cmd.extend(('--', args.which + u'.py', 'Dear 💩', '${ISOLATED_OUTDIR}'))
      common.run(cmd, args.verbose)

      common.note('Getting results from %s' % args.swarming)
      resdir = os.path.join(tempdir, 'results').encode('utf-8')
      common.run(
          [
            'swarming.py',
            'collect',
            '--swarming', args.swarming,
            '--json', json_file,
            '--task-output-dir', resdir,
          ], args.verbose)
      for root, _, files in os.walk(resdir):
        for name in files:
          p = os.path.join(root, name)
          with open(p, 'rb') as f:
            print('%s content:' % p)
            print(f.read())
      return 0
    finally:
      shutil.rmtree(tempdir)
  except subprocess.CalledProcessError as e:
    return e.returncode


if __name__ == '__main__':
  sys.exit(main())
