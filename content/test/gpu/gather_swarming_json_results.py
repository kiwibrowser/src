#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script which gathers and merges the JSON results from multiple
swarming shards of a step on the waterfall.

This is used to feed in the per-test times of previous runs of tests
to the browser_test_runner's sharding algorithm, to improve shard
distribution.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib
import urllib2

SWARMING_SERVICE = 'https://chromium-swarm.appspot.com'

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(THIS_DIR)))
SWARMING_CLIENT_DIR = os.path.join(SRC_DIR, 'tools', 'swarming_client')

class Swarming:
  @staticmethod
  def CheckAuth():
    output = subprocess.check_output([
      'python',
      os.path.join(SWARMING_CLIENT_DIR, 'auth.py'),
      'check',
      '--service',
      SWARMING_SERVICE])
    if not output.startswith('user:'):
      print 'Must run:'
      print '  tools/swarming_client/auth.py login --service ' + \
          SWARMING_SERVICE
      print 'and authenticate with @google.com credentials.'
      sys.exit(1)

  @staticmethod
  def Collect(taskIDs, output_dir, verbose):
    cmd = [
      'python',
      os.path.join(SWARMING_CLIENT_DIR, 'swarming.py'),
      'collect',
      '-S',
      SWARMING_SERVICE,
      '--task-output-dir',
      output_dir] + taskIDs
    if verbose:
      print 'Collecting Swarming results:'
      print cmd
    if verbose > 1:
      # Print stdout from the collect command.
      stdout = None
    else:
      fnull = open(os.devnull, 'w')
      stdout = fnull
    subprocess.check_call(cmd, stdout=stdout, stderr=subprocess.STDOUT)

  @staticmethod
  def ExtractShardTaskIDs(urls):
    SWARMING_URL = 'https://chromium-swarm.appspot.com/user/task/'
    task_ids = []
    for u in urls:
      if not u.startswith(SWARMING_URL):
        raise Exception('Illegally formatted \'urls\' value %s' % v)
      task_ids.append(u[len(SWARMING_URL):])
    return task_ids

class Waterfall:
  def __init__(self, waterfall):
    self._waterfall = waterfall

  def GetJsonForBuild(self, bot, build):
    # Explorable via RPC explorer:
    # https://luci-milo.appspot.com/rpcexplorer/services/milo.BuildInfo/Get

    # The Python docs are wrong. It's fine for this payload to be just
    # a JSON string.
    call_arg = json.dumps({ "buildbot": { "masterName": self._waterfall,
                                          "builderName": bot,
                                          "buildNumber": build }})
    headers = {
      "content-type": "application/json",
      "accept": "application/json"
    }
    request = urllib2.Request(
      "https://luci-milo.appspot.com/prpc/milo.BuildInfo/Get",
      call_arg,
      headers)
    conn = urllib2.urlopen(request)
    result = conn.read()
    conn.close()

    # Result is a two-line string the first line of which is
    # deliberate garbage and the second of which is a JSON payload.
    return json.loads(result.splitlines()[1])

def JsonLoadStrippingUnicode(file, **kwargs):
  def StripUnicode(obj):
    if isinstance(obj, unicode):
      try:
        return obj.encode('ascii')
      except UnicodeEncodeError:
        return obj

    if isinstance(obj, list):
      return map(StripUnicode, obj)

    if isinstance(obj, dict):
      new_obj = type(obj)(
          (StripUnicode(k), StripUnicode(v)) for k, v in obj.iteritems() )
      return new_obj

    return obj

  return StripUnicode(json.load(file, **kwargs))

def FindStepRecursive(node, step_name):
  # The format of this JSON-encoded protobuf is defined here:
  # https://chromium.googlesource.com/infra/luci/luci-go/+/master/
  #   common/proto/milo/annotations.proto
  # It's easiest to just use the RPC explorer to fetch one and see
  # what's desired to extract.
  if 'name' in node:
    if node['name'].startswith(step_name):
      return node
  if 'substep' in node:
    for subnode in node['substep']:
      # The substeps all wrap the node we care about in a wrapper
      # object which has one field named "step".
      res = FindStepRecursive(subnode['step'], step_name)
      if res:
        return res
  return None

def Merge(dest, src):
  if isinstance(dest, list):
    if not isinstance(src, list):
      raise Exception('Both must be lists: ' + dest + ' and ' + src)
    return dest + src

  if isinstance(dest, dict):
    if not isinstance(src, dict):
      raise Exception('Both must be dicts: ' + dest + ' and ' + src)
    for k in src.iterkeys():
      if k not in dest:
        dest[k] = src[k]
      else:
        dest[k] = Merge(dest[k], src[k])
    return dest

  return src


def ExtractTestTimes(node, node_name, dest):
  if 'times' in node:
    dest[node_name] = sum(node['times']) / len(node['times'])
  else:
    # Currently the prefix names in the trie are dropped. Could
    # concatenate them if the naming convention is changed.
    for k in node.iterkeys():
      if isinstance(node[k], dict):
        ExtractTestTimes(node[k], k, dest)

def main():
  rest_args = sys.argv[1:]
  parser = argparse.ArgumentParser(
    description='Gather JSON results from a run of a Swarming test.',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('-v', '--verbose', action='count', default=0,
                      help='Enable verbose output (specify multiple times '
                      'for more output)')
  parser.add_argument('--waterfall', type=str, default='chromium.gpu.fyi',
                      help='Which waterfall to examine')
  parser.add_argument('--bot', type=str, default='Linux Release (NVIDIA)',
                      help='Which bot on the waterfall to examine')
  parser.add_argument('--build', default=-1, type=int,
                      help='Which build to fetch (must be specified)')
  parser.add_argument('--step', type=str, default='webgl2_conformance_tests',
                      help='Which step to fetch (treated as a prefix)')
  parser.add_argument('--output', type=str, default='output.json',
                      help='Name of output file; contains only test run times')
  parser.add_argument('--full-output', type=str, default='',
                      help='Name of complete output file if desired')
  parser.add_argument('--leak-temp-dir', action='store_true', default=False,
                      help='Deliberately leak temporary directory')
  parser.add_argument('--start-from-temp-dir', type=str, default='',
                      help='Start from temporary directory (for debugging)')

  options = parser.parse_args(rest_args)

  if options.start_from_temp_dir:
    tmpdir = options.start_from_temp_dir
    shard_dirs = [f for f in os.listdir(tmpdir)
                  if os.path.isdir(os.path.join(tmpdir, f))]
    numTaskIDs = len(shard_dirs)
  else:
    Swarming.CheckAuth()

    waterfall = Waterfall(options.waterfall)
    build = options.build
    if build < 0:
      print "Build number must be specified; check the bot's page"
      return 1

    build_json = waterfall.GetJsonForBuild(options.bot, build)

    if options.verbose:
      print 'Fetching information from %s, bot %s, build %s' % (
        options.waterfall, options.bot, build)

    step = FindStepRecursive(build_json['step'], options.step)
    if not step:
      print "Unable to find step starting with " + options.step
      return 1

    shard_urls = []
    expected_prefix = 'https://chromium-swarm.appspot.com/user/task/'
    for link in step['otherLinks']:
      label = link['label']
      if label.startswith('shard #') and not label.endswith('isolated out'):
        shard_urls.append(link['url'])
    task_ids = Swarming.ExtractShardTaskIDs(shard_urls)

    if not task_ids:
      print 'Problem gathering the Swarming task IDs for %s' % options.step
      return 1

    # Collect the results.
    tmpdir = tempfile.mkdtemp()
    Swarming.Collect(task_ids, tmpdir, options.verbose)
    num_task_ids = len(task_ids)

  # Shards' JSON outputs are in sequentially-numbered subdirectories
  # of the output directory.
  merged_json = None
  for i in xrange(num_task_ids):
    with open(os.path.join(tmpdir, str(i), 'output.json')) as f:
      cur_json = JsonLoadStrippingUnicode(f)
      if not merged_json:
        merged_json = cur_json
      else:
        merged_json = Merge(merged_json, cur_json)
  extracted_times = {'times':{}}
  ExtractTestTimes(merged_json, '', extracted_times['times'])

  with open(options.output, 'w') as f:
    json.dump(extracted_times, f, sort_keys=True, indent=2,
              separators=(',', ': '))

  if options.full_output:
    json.dump(merged_json, f, sort_keys=True, indent=2,
              separators=(',', ': '))

  if options.leak_temp_dir:
    print 'Temporary directory: %s' % tmpdir
  else:
    shutil.rmtree(tmpdir)

  return 0


if __name__ == "__main__":
  sys.exit(main())
