#!/usr/bin/env python
# Copyright 2014 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Automated maintenance tool to run a script on bots.

To use this script, write a self-contained python script (use a .zip if
necessary), specify it on the command line and it will be packaged and triggered
on all the swarming bots corresponding to the --dimension filters specified, or
all the bots if no filter is specified.
"""

__version__ = '0.2'

import json
import os
import shutil
import string
import subprocess
import sys
import tempfile
import threading

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))

# Must be first import.
import parallel_execution

from third_party import colorama
from third_party.chromium import natsort
from third_party.depot_tools import fix_encoding
from utils import file_path
from utils import tools


def get_bot_list(swarming_server, dimensions):
  """Returns a list of swarming bots: health, quarantined, dead."""
  q = '&'.join(
      'dimensions=%s:%s' % (k, v) for k, v in sorted(dimensions.iteritems()))
  cmd = [
    sys.executable, 'swarming.py', 'query',
    '--swarming', swarming_server,
    '--limit', '0',
    'bots/list?' + q,
  ]
  healthy = []
  quarantined = []
  dead = []
  results = json.loads(subprocess.check_output(cmd, cwd=ROOT_DIR))
  if not results.get('items'):
    return (), (), ()
  for b in results['items']:
    if b['is_dead']:
      dead.append(b['bot_id'])
    elif b['quarantined']:
      quarantined.append(b['bot_id'])
    else:
      healthy.append(b['bot_id'])
  return natsort.natsorted(healthy), quarantined, dead


def archive(isolate_server, script):
  """Archives the tool and return the sha-1."""
  base_script = os.path.basename(script)
  isolate = {
    'variables': {
      'command': ['python', base_script],
      'files': [base_script],
    },
  }
  tempdir = tempfile.mkdtemp(prefix=u'run_on_bots')
  try:
    isolate_file = os.path.join(tempdir, 'tool.isolate')
    isolated_file = os.path.join(tempdir, 'tool.isolated')
    with open(isolate_file, 'wb') as f:
      f.write(str(isolate))
    shutil.copyfile(script, os.path.join(tempdir, base_script))
    cmd = [
      sys.executable, 'isolate.py', 'archive',
      '--isolate-server', isolate_server,
      '-i', isolate_file,
      '-s', isolated_file,
    ]
    return subprocess.check_output(cmd, cwd=ROOT_DIR).split()[0]
  finally:
    file_path.rmtree(tempdir)


def batched_subprocess(cmd, sem):
    def run(cmd, sem):
      subprocess.call(cmd, cwd=ROOT_DIR)
      sem.release()
    sem.acquire()
    thread = threading.Thread(target=run, args=(cmd, sem))
    thread.start()
    return thread


def run_batches(
    swarming_server, isolate_server, dimensions, tags, env, priority, deadline,
    batches, repeat, isolated_hash, name, bots, args):
  """Runs the task |batches| at a time.

  This will be mainly bound by task scheduling latency, especially if the bots
  are busy and the priority is low.
  """
  sem = threading.Semaphore(batches)
  threads = []
  for i in xrange(repeat):
    for bot in bots:
      suffix = '/%d' % i if repeat > 1 else ''
      task_name = parallel_execution.task_to_name(
            name, {'id': bot}, isolated_hash) + suffix
      cmd = [
        sys.executable, 'swarming.py', 'run',
        '--swarming', swarming_server,
        '--isolate-server', isolate_server,
        '--priority', priority,
        '--deadline', deadline,
        '--dimension', 'id', bot,
        '--task-name', task_name,
        '-s', isolated_hash,
      ]
      for k, v in sorted(dimensions.iteritems()):
        cmd.extend(('-d', k, v))
      for t in sorted(tags):
        cmd.extend(('--tags', t))
      for k, v in env:
        cmd.extend(('--env', k, v))
      if args:
        cmd.append('--')
        cmd.extend(args)
      threads.append(batched_subprocess(cmd, sem))
  for t in threads:
     t.join()


def run_serial(
    swarming_server, isolate_server, dimensions, tags, env, priority, deadline,
    repeat, isolated_hash, name, bots, args):
  """Runs the task one at a time.

  This will be mainly bound by task scheduling latency, especially if the bots
  are busy and the priority is low.
  """
  result = 0
  for i in xrange(repeat):
    for bot in bots:
      suffix = '/%d' % i if repeat > 1 else ''
      task_name = parallel_execution.task_to_name(
          name, {'id': bot}, isolated_hash) + suffix
      cmd = [
        sys.executable, 'swarming.py', 'run',
        '--swarming', swarming_server,
        '--isolate-server', isolate_server,
        '--priority', priority,
        '--deadline', deadline,
        '--dimension', 'id', bot,
        '--task-name', task_name,
        '-s', isolated_hash,
      ]
      for k, v in sorted(dimensions.iteritems()):
        cmd.extend(('-d', k, v))
      for t in sorted(tags):
        cmd.extend(('--tags', t))
      for k, v in env:
        cmd.extend(('--env', k, v))
      if args:
        cmd.append('--')
        cmd.extend(args)
      r = subprocess.call(cmd, cwd=ROOT_DIR)
      result = max(r, result)
  return result


def run_parallel(
    swarming_server, isolate_server, dimensions, env, priority, deadline,
    repeat, isolated_hash, name, bots, args):
  tasks = []
  for i in xrange(repeat):
    suffix = '/%d' % i if repeat > 1 else ''
    for bot in bots:
      d = {'id': bot}
      tname = parallel_execution.task_to_name(name, d, isolated_hash) + suffix
      d.update(dimensions)
      tasks.append((tname, isolated_hash, d, env))
  extra_args = ['--priority', priority, '--deadline', deadline]
  extra_args.extend(args)
  print('Using priority %s' % priority)
  for failed_task in parallel_execution.run_swarming_tasks_parallel(
      swarming_server, isolate_server, extra_args, tasks):
    _name, dimensions, stdout = failed_task
    print('%sFailure: %s%s\n%s' % (
      colorama.Fore.RED, dimensions, colorama.Fore.RESET, stdout))


def main():
  parser = parallel_execution.OptionParser(
      usage='%prog [options] (script.py|isolated hash) '
            '-- [script.py arguments]',
      version=__version__)
  parser.add_option(
      '--serial', action='store_true',
      help='Runs the task serially, to be used when debugging problems since '
           'it\'s slow')
  parser.add_option(
      '--batches', type='int', default=0,
      help='Runs a task in parallel |batches| at a time.')
  parser.add_option(
      '--tags', action='append', default=[], metavar='FOO:BAR',
      help='Tags to assign to the task.')
  parser.add_option(
      '--repeat', type='int', default=1,
      help='Runs the task multiple time on each bot, meant to be used as a '
           'load test')
  parser.add_option(
      '--name',
      help='Name to use when providing an isolated hash')
  options, args = parser.parse_args()

  if len(args) < 1:
    parser.error(
        'Must pass one python script to run. Use --help for more details')

  if not options.priority:
    parser.error(
        'Please provide the --priority option. Either use a very low number\n'
        'so the task completes as fast as possible, or an high number so the\n'
        'task only runs when the bot is idle.')

  # 1. Archive the script to run.
  if not os.path.exists(args[0]):
    if not options.name:
      parser.error(
          'Please provide --name when using an isolated hash.')
    if len(args[0]) not in (40, 64):
      parser.error(
          'Hash wrong length %d (%r)' % (len(args.hash), args[0]))
    for i, c in enumerate(args[0]):
      if c not in string.hexdigits:
        parser.error(
            'Hash character invalid\n'
            ' %s\n' % args[0] +
            ' '+'-'*i+'^\n'
            )

    isolated_hash = args[0]
    name = options.name
  else:
    isolated_hash = archive(options.isolate_server, args[0])
    name = os.path.basename(args[0])

  print('Running %s' % isolated_hash)

  # 2. Query the bots list.
  bots, quarantined_bots, dead_bots = get_bot_list(
      options.swarming, options.dimensions)
  print('Found %d bots to process' % len(bots))
  if quarantined_bots:
    print('Warning: found %d quarantined bots' % len(quarantined_bots))
  if dead_bots:
    print('Warning: found %d dead bots' % len(dead_bots))
  if not bots:
    return 1

  # 3. Trigger the tasks.
  if options.batches > 0:
    return run_batches(
        options.swarming,
        options.isolate_server,
        options.dimensions,
        options.tags,
        options.env,
        str(options.priority),
        str(options.deadline),
        options.batches,
        options.repeat,
        isolated_hash,
        name,
        bots,
        args[1:])

  if options.serial:
    return run_serial(
        options.swarming,
        options.isolate_server,
        options.dimensions,
        options.tags,
        options.env,
        str(options.priority),
        str(options.deadline),
        options.repeat,
        isolated_hash,
        name,
        bots,
        args[1:])

  return run_parallel(
      options.swarming,
      options.isolate_server,
      options.dimensions,
      options.env,
      str(options.priority),
      str(options.deadline),
      options.repeat,
      isolated_hash,
      name,
      bots,
      args[1:])


if __name__ == '__main__':
  fix_encoding.fix_encoding()
  tools.disable_buffering()
  colorama.init()
  sys.exit(main())
