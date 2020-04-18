#!/usr/bin/env python
# Copyright 2017 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Triggers a task that can be used to debug another task."""

import argparse
import json
import os
import subprocess
import sys
import tempfile

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# URL to point people to. *Chromium specific*
URL = 'http://go/swarming-ssh'


COMMAND = """import os,sys,time
print 'Mapping task: %(task_url)s'
print 'Files are mapped into: ' + os.getcwd()
print 'Original command: %(original_cmd)s'
print ''
print 'Bot id: ' + os.environ['SWARMING_BOT_ID']
print 'Bot leased for: %(duration)d seconds'
print 'How to access this bot: %(help_url)s'
print 'When done, reboot the host'
sys.stdout.flush()
time.sleep(%(duration)d)
"""


class Failed(Exception):
  pass


def retrieve_task_props(swarming, taskid):
  """Retrieves the task request metadata."""
  cmd = [
      sys.executable, 'swarming.py', 'query', '-S', swarming,
      'task/%s/request' % taskid,
  ]
  try:
    return json.loads(subprocess.check_output(cmd, cwd=ROOT_DIR))
  except subprocess.CalledProcessError:
    raise Failed('Are you sure the task ID and swarming server is valid?')


def retrieve_task_results(swarming, taskid):
  """Retrieves the task request metadata."""
  cmd = [
      sys.executable, 'swarming.py', 'query', '-S', swarming,
      'task/%s/result' % taskid,
  ]
  try:
    return json.loads(subprocess.check_output(cmd, cwd=ROOT_DIR))
  except subprocess.CalledProcessError:
    raise Failed('Are you sure the task ID and swarming server is valid?')


def generate_command(swarming, taskid, task, duration):
  """Generats a command that sleep and prints the original command."""
  original = get_swarming_args_from_task(task)
  return [
    'python', '-c',
    COMMAND.replace('\n', ';') % {
      'duration': duration,
      'original_cmd': ' '.join(original),
      'task_url': 'https://%s/task?id=%s' % (swarming, taskid),
      'help_url': URL,
    },
  ]


def get_swarming_args_from_task(task):
  """Extracts the original command from a task."""
  if task.get('command'):
    return task['command']

  command = []
  isolated = task['properties'].get('inputs_ref', {}).get('isolated')
  if isolated:
    f, name = tempfile.mkstemp(prefix='debug_task')
    os.close(f)
    try:
      cmd = [
        sys.executable, 'isolateserver.py', 'download',
        '-I',  task['properties']['inputs_ref']['isolatedserver'],
        '--namespace',  task['properties']['inputs_ref']['namespace'],
        '-f', isolated, name,
      ]
      subprocess.check_call(cmd, cwd=ROOT_DIR)
      with open(name, 'rb') as f:
        command = json.load(f).get('command')
    finally:
      os.remove(name)
  extra_args = task.get('extra_args')
  if extra_args:
    command.extend(extra_args)
  return command


def trigger(swarming, taskid, task, duration, reuse_bot):
  """Triggers a task runs a command that sleeps mapping the same input files as
  'task'.
  """
  cmd = [
    sys.executable, 'swarming.py', 'trigger', '-S', swarming,
    '-S', swarming,
    '--hard-timeout', str(duration),
    '--io-timeout', str(duration),
    '--task-name', 'Debug Task for %s' % taskid,
    '--raw-cmd',
    '--tags', 'debug_task:1',
  ]
  if reuse_bot:
    pool = [
        i['value'] for i in task['properties']['dimensions']
        if i['key'] == 'pool'][0]
    cmd.extend(('-d', 'pool', pool))
    # Need to query the task's bot.
    res = retrieve_task_results(swarming, taskid)
    cmd.extend(('-d', 'id', res['bot_id']))
  else:
    for i in task['properties']['dimensions']:
      cmd.extend(('-d', i['key'], i['value']))

  if task['properties'].get('inputs_ref'):
    cmd.extend(
        [
          '-s', task['properties']['inputs_ref']['isolated'],
          '-I',  task['properties']['inputs_ref']['isolatedserver'],
          '--namespace',  task['properties']['inputs_ref']['namespace'],
        ])

  for i in task['properties'].get('env', []):
    cmd.extend(('--env', i['key'], i['value']))

  cipd = task['properties'].get('cipd_input', {})
  if cipd:
    for p in cipd['packages']:
      cmd.extend(('--cipd-package', p['package_name'], p['version'], p['path']))

  for i in task['properties'].get('caches', []):
    cmd.extend(('--named-cache', i['name'], i['path']))

  cmd.append('--')
  cmd.extend(generate_command(swarming, taskid, task, duration))
  try:
    return subprocess.call(cmd, cwd=ROOT_DIR)
  except KeyboardInterrupt:
    return 0


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('taskid', help='Task\'s input files to map onto the bot')
  parser.add_argument(
      '-S', '--swarming',
      metavar='URL', default=os.environ.get('SWARMING_SERVER', ''),
      help='Swarming server to use')
  parser.add_argument(
      '-r', '--reuse-bot', action='store_true',
      help='Locks the debug task to the original bot that ran the task')
  parser.add_argument(
      '-l', '--lease', type=int, default=6*60*60, metavar='SECS',
      help='Duration of the lease')
  args = parser.parse_args()

  try:
    org = retrieve_task_props(args.swarming, args.taskid)
    return trigger(args.swarming, args.taskid, org, args.lease, args.reuse_bot)
  except Failed as e:
    sys.stderr.write('\n%s\n' % e)
    return 1


if __name__ == '__main__':
  sys.exit(main())
