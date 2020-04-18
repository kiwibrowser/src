#!/usr/bin/env python
# coding=utf-8
# Copyright 2017 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Reproduce one or multiple tasks from one Swarming server on another one."""

import argparse
import json
import logging
import os
import subprocess
import sys
import tempfile
import urllib


BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def capture_swarming(server, cmd, *args):
  cmd = [
      sys.executable, os.path.join(BASE_DIR, 'swarming.py'), cmd, '-S', server]
  cmd.extend(args)
  logging.info('%s', ' '.join(args))
  return subprocess.check_output(cmd)


def query_swarming(server, *args):
  return json.loads(capture_swarming(server, 'query', '--limit', '0', *args))


def get_all_by_tags(server, tags):
  # For example:
  # master:tryserver.chromium.win
  # buildername:win_chromium_rel_ng
  # buildnumber:516399
  url = 'tasks/list?' + '&'.join('tags=' + urllib.quote(t) for t in tags)
  tasks = query_swarming(server, url)['items']
  print('Found %d original tasks' % len(tasks))
  return [t['task_id'] for t in tasks]


def reproduce(server_old, server_new, task_id):
  request = query_swarming(server_old, 'task/%s/request' % task_id)
  logging.debug('%s', json.dumps(request, indent=2, sort_keys=True))
  prop = request['properties']
  inputs = prop['inputs_ref']
  name = request['name']
  if prop.get('secret_bytes'):
    print >> sys.stderr, 'Can\'t reproduce task %s with secrets' % task_id
    sys.exit(1)
  cmd = [
    'trigger', '--tag', 'testing:1',
    '-I', inputs['isolatedserver'],
    '--namespace', inputs['namespace'],
    '-s', inputs['isolated'],
    '--task-name', name,
    '--priority', '10',
    '--expiration', request['expiration_secs'],
    '--hard-timeout', prop['execution_timeout_secs'],
    '--io-timeout', prop['io_timeout_secs'],
  ]
  if request.get('service_account'):
    cmd.extend(('--service-account', request['service_account']))
  for d in prop['dimensions']:
    cmd.extend(('-d', d['key'], d['value']))
  for d in prop.get('env', []):
    cmd.extend(('--env', d['key'], d['value']))
  for d in prop.get('env_prefix', []):
    cmd.extend(('--env-prefix', d['key'], d['value']))
  for o in prop.get('outputs', []):
    cmd.extend(('--output', o))
  for d in prop.get('caches', []):
    cmd.extend(('--named-cache', d['name'], d['path']))
  for p in prop.get('cipd_input', {}).get('packages', []):
    # server and client_package in cipd_input are not reused.
    cmd.extend((
      '--cipd-package',
      '%s:%s:%s' % (p['path'], p['package_name'], p['version'])))
  h, tmp = tempfile.mkstemp(prefix='reproduce', suffix='.json')
  os.close(h)
  try:
    cmd.extend(('--dump-json', tmp))
    if prop['extra_args']:
      cmd.append('--')
      cmd.extend(prop['extra_args'])
    if prop.get('command'):
      cmd.append('--raw-cmd')
      cmd.append('--')
      cmd.extend(prop['command'])
    capture_swarming(server_new, *cmd)
    with open(tmp) as f:
      return json.load(f)['tasks'].items()[0][1]['task_id'], name
  finally:
    os.remove(tmp)


def cut(n):
  if len(n) <= 80:
    return n
  return n[:79] + u'…'


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--old', required=True, help='Old Swarming server to take the task from')
  parser.add_argument(
      '--new',
      help='New Swarming server to run the task on; if not specified, --old '
      'is used')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument(
      '--task-id', action='append', default=[], help='Task IDs to reproduce')
  group.add_argument(
      '--tags', action='append', default=[],
      help='Tags to query tasks to reproduce')
  parser.add_argument('-v', '--verbose', action='store_true')
  args = parser.parse_args()

  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.ERROR)

  if args.tags:
    args.task_id = get_all_by_tags(args.old, args.tags)

  if len(args.task_id) == 1:
    task_id, name = reproduce(args.old, args.new or args.old, args.task_id[0])
    print(u'%s %s' % (task_id, cut(name)))
    return 0

  for i, task_id in enumerate(args.task_id):
    task_id, name = reproduce(args.old, args.new or args.old, task_id)
    print(u'%d/%d: %s %s' % (i+1, len(args.task_id), task_id, cut(name)))
  return 0


if __name__ == '__main__':
  sys.exit(main())
