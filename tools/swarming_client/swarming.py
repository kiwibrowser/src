#!/usr/bin/env python
# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Client tool to trigger tasks or retrieve results from a Swarming server."""

__version__ = '0.12'

import collections
import datetime
import json
import logging
import optparse
import os
import re
import sys
import textwrap
import threading
import time
import urllib

from third_party import colorama
from third_party.depot_tools import fix_encoding
from third_party.depot_tools import subcommand

from utils import file_path
from utils import fs
from utils import logging_utils
from third_party.chromium import natsort
from utils import net
from utils import on_error
from utils import subprocess42
from utils import threading_utils
from utils import tools

import auth
import cipd
import isolateserver
import run_isolated


ROOT_DIR = os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding())))


class Failure(Exception):
  """Generic failure."""
  pass


def default_task_name(options):
  """Returns a default task name if not specified."""
  if not options.task_name:
    task_name = u'%s/%s' % (
        options.user,
        '_'.join('%s=%s' % (k, v) for k, v in options.dimensions))
    if options.isolated:
      task_name += u'/' + options.isolated
    return task_name
  return options.task_name


### Triggering.


# See ../appengine/swarming/swarming_rpcs.py.
CipdPackage = collections.namedtuple(
    'CipdPackage',
    [
      'package_name',
      'path',
      'version',
    ])


# See ../appengine/swarming/swarming_rpcs.py.
CipdInput = collections.namedtuple(
    'CipdInput',
    [
      'client_package',
      'packages',
      'server',
    ])


# See ../appengine/swarming/swarming_rpcs.py.
FilesRef = collections.namedtuple(
    'FilesRef',
    [
      'isolated',
      'isolatedserver',
      'namespace',
    ])


# See ../appengine/swarming/swarming_rpcs.py.
StringListPair = collections.namedtuple(
  'StringListPair', [
    'key',
    'value', # repeated string
  ]
)


# See ../appengine/swarming/swarming_rpcs.py.
TaskProperties = collections.namedtuple(
    'TaskProperties',
    [
      'caches',
      'cipd_input',
      'command',
      'relative_cwd',
      'dimensions',
      'env',
      'env_prefixes',
      'execution_timeout_secs',
      'extra_args',
      'grace_period_secs',
      'idempotent',
      'inputs_ref',
      'io_timeout_secs',
      'outputs',
      'secret_bytes',
    ])


# See ../appengine/swarming/swarming_rpcs.py.
NewTaskRequest = collections.namedtuple(
    'NewTaskRequest',
    [
      'name',
      'parent_task_id',
      'priority',
      'task_slices',
      'service_account',
      'tags',
      'user',
    ])


def namedtuple_to_dict(value):
  """Recursively converts a namedtuple to a dict."""
  if hasattr(value, '_asdict'):
    return namedtuple_to_dict(value._asdict())
  if isinstance(value, (list, tuple)):
    return [namedtuple_to_dict(v) for v in value]
  if isinstance(value, dict):
    return {k: namedtuple_to_dict(v) for k, v in value.iteritems()}
  return value


def task_request_to_raw_request(task_request):
  """Returns the json-compatible dict expected by the server for new request.

  This is for the v1 client Swarming API.
  """
  out = namedtuple_to_dict(task_request)
  # Don't send 'service_account' if it is None to avoid confusing older
  # version of the server that doesn't know about 'service_account' and don't
  # use it at all.
  if not out['service_account']:
    out.pop('service_account')
  out['task_slices'][0]['properties']['dimensions'] = [
    {'key': k, 'value': v}
    for k, v in out['task_slices'][0]['properties']['dimensions']
  ]
  out['task_slices'][0]['properties']['env'] = [
    {'key': k, 'value': v}
    for k, v in out['task_slices'][0]['properties']['env'].iteritems()
  ]
  out['task_slices'][0]['properties']['env'].sort(key=lambda x: x['key'])
  return out


def swarming_trigger(swarming, raw_request):
  """Triggers a request on the Swarming server and returns the json data.

  It's the low-level function.

  Returns:
    {
      'request': {
        'created_ts': u'2010-01-02 03:04:05',
        'name': ..
      },
      'task_id': '12300',
    }
  """
  logging.info('Triggering: %s', raw_request['name'])

  result = net.url_read_json(
      swarming + '/api/swarming/v1/tasks/new', data=raw_request)
  if not result:
    on_error.report('Failed to trigger task %s' % raw_request['name'])
    return None
  if result.get('error'):
    # The reply is an error.
    msg = 'Failed to trigger task %s' % raw_request['name']
    if result['error'].get('errors'):
      for err in result['error']['errors']:
        if err.get('message'):
          msg += '\nMessage: %s' % err['message']
        if err.get('debugInfo'):
          msg += '\nDebug info:\n%s' % err['debugInfo']
    elif result['error'].get('message'):
      msg += '\nMessage: %s' % result['error']['message']

    on_error.report(msg)
    return None
  return result


def setup_googletest(env, shards, index):
  """Sets googletest specific environment variables."""
  if shards > 1:
    assert not any(i['key'] == 'GTEST_SHARD_INDEX' for i in env), env
    assert not any(i['key'] == 'GTEST_TOTAL_SHARDS' for i in env), env
    env = env[:]
    env.append({'key': 'GTEST_SHARD_INDEX', 'value': str(index)})
    env.append({'key': 'GTEST_TOTAL_SHARDS', 'value': str(shards)})
  return env


def trigger_task_shards(swarming, task_request, shards):
  """Triggers one or many subtasks of a sharded task.

  Returns:
    Dict with task details, returned to caller as part of --dump-json output.
    None in case of failure.
  """
  def convert(index):
    req = task_request_to_raw_request(task_request)
    if shards > 1:
      req['task_slices'][0]['properties']['env'] = setup_googletest(
          req['task_slices'][0]['properties']['env'], shards, index)
      req['name'] += ':%s:%s' % (index, shards)
    return req

  requests = [convert(index) for index in xrange(shards)]
  tasks = {}
  priority_warning = False
  for index, request in enumerate(requests):
    task = swarming_trigger(swarming, request)
    if not task:
      break
    logging.info('Request result: %s', task)
    if (not priority_warning and
        int(task['request']['priority']) != task_request.priority):
      priority_warning = True
      print >> sys.stderr, (
          'Priority was reset to %s' % task['request']['priority'])
    tasks[request['name']] = {
      'shard_index': index,
      'task_id': task['task_id'],
      'view_url': '%s/user/task/%s' % (swarming, task['task_id']),
    }

  # Some shards weren't triggered. Abort everything.
  if len(tasks) != len(requests):
    if tasks:
      print >> sys.stderr, 'Only %d shard(s) out of %d were triggered' % (
          len(tasks), len(requests))
      for task_dict in tasks.itervalues():
        abort_task(swarming, task_dict['task_id'])
    return None

  return tasks


### Collection.


# How often to print status updates to stdout in 'collect'.
STATUS_UPDATE_INTERVAL = 15 * 60.


class State(object):
  """States in which a task can be.

  WARNING: Copy-pasted from appengine/swarming/server/task_result.py. These
  values are part of the API so if they change, the API changed.

  It's in fact an enum. Values should be in decreasing order of importance.
  """
  RUNNING = 0x10
  PENDING = 0x20
  EXPIRED = 0x30
  TIMED_OUT = 0x40
  BOT_DIED = 0x50
  CANCELED = 0x60
  COMPLETED = 0x70
  KILLED = 0x80
  NO_RESOURCE = 0x100

  STATES = (
      'RUNNING', 'PENDING', 'EXPIRED', 'TIMED_OUT', 'BOT_DIED', 'CANCELED',
      'COMPLETED', 'KILLED', 'NO_RESOURCE')
  STATES_RUNNING = ('RUNNING', 'PENDING')
  STATES_NOT_RUNNING = (
      'EXPIRED', 'TIMED_OUT', 'BOT_DIED', 'CANCELED', 'COMPLETED', 'KILLED',
      'NO_RESOURCE')
  STATES_DONE = ('TIMED_OUT', 'COMPLETED', 'KILLED')
  STATES_ABANDONED = ('EXPIRED', 'BOT_DIED', 'CANCELED', 'NO_RESOURCE')

  _NAMES = {
    RUNNING: 'Running',
    PENDING: 'Pending',
    EXPIRED: 'Expired',
    TIMED_OUT: 'Execution timed out',
    BOT_DIED: 'Bot died',
    CANCELED: 'User canceled',
    COMPLETED: 'Completed',
    KILLED: 'User killed',
    NO_RESOURCE: 'No resource',
  }

  _ENUMS = {
    'RUNNING': RUNNING,
    'PENDING': PENDING,
    'EXPIRED': EXPIRED,
    'TIMED_OUT': TIMED_OUT,
    'BOT_DIED': BOT_DIED,
    'CANCELED': CANCELED,
    'COMPLETED': COMPLETED,
    'KILLED': KILLED,
    'NO_RESOURCE': NO_RESOURCE,
  }

  @classmethod
  def to_string(cls, state):
    """Returns a user-readable string representing a State."""
    if state not in cls._NAMES:
      raise ValueError('Invalid state %s' % state)
    return cls._NAMES[state]

  @classmethod
  def from_enum(cls, state):
    """Returns int value based on the string."""
    if state not in cls._ENUMS:
      raise ValueError('Invalid state %s' % state)
    return cls._ENUMS[state]


class TaskOutputCollector(object):
  """Assembles task execution summary (for --task-summary-json output).

  Optionally fetches task outputs from isolate server to local disk (used when
  --task-output-dir is passed).

  This object is shared among multiple threads running 'retrieve_results'
  function, in particular they call 'process_shard_result' method in parallel.
  """

  def __init__(self, task_output_dir, task_output_stdout, shard_count):
    """Initializes TaskOutputCollector, ensures |task_output_dir| exists.

    Args:
      task_output_dir: (optional) local directory to put fetched files to.
      shard_count: expected number of task shards.
    """
    self.task_output_dir = (
        unicode(os.path.abspath(task_output_dir))
        if task_output_dir else task_output_dir)
    self.task_output_stdout = task_output_stdout
    self.shard_count = shard_count

    self._lock = threading.Lock()
    self._per_shard_results = {}
    self._storage = None

    if self.task_output_dir:
      file_path.ensure_tree(self.task_output_dir)

  def process_shard_result(self, shard_index, result):
    """Stores results of a single task shard, fetches output files if necessary.

    Modifies |result| in place.

    shard_index is 0-based.

    Called concurrently from multiple threads.
    """
    # Sanity check index is in expected range.
    assert isinstance(shard_index, int)
    if shard_index < 0 or shard_index >= self.shard_count:
      logging.warning(
          'Shard index %d is outside of expected range: [0; %d]',
          shard_index, self.shard_count - 1)
      return

    if result.get('outputs_ref'):
      ref = result['outputs_ref']
      result['outputs_ref']['view_url'] = '%s/browse?%s' % (
          ref['isolatedserver'],
          urllib.urlencode(
              [('namespace', ref['namespace']), ('hash', ref['isolated'])]))

    # Store result dict of that shard, ignore results we've already seen.
    with self._lock:
      if shard_index in self._per_shard_results:
        logging.warning('Ignoring duplicate shard index %d', shard_index)
        return
      self._per_shard_results[shard_index] = result

    # Fetch output files if necessary.
    if self.task_output_dir and result.get('outputs_ref'):
      storage = self._get_storage(
          result['outputs_ref']['isolatedserver'],
          result['outputs_ref']['namespace'])
      if storage:
        # Output files are supposed to be small and they are not reused across
        # tasks. So use MemoryCache for them instead of on-disk cache. Make
        # files writable, so that calling script can delete them.
        isolateserver.fetch_isolated(
            result['outputs_ref']['isolated'],
            storage,
            isolateserver.MemoryCache(file_mode_mask=0700),
            os.path.join(self.task_output_dir, str(shard_index)),
            False)

  def finalize(self):
    """Assembles and returns task summary JSON, shutdowns underlying Storage."""
    with self._lock:
      # Write an array of shard results with None for missing shards.
      summary = {
        'shards': [
          self._per_shard_results.get(i) for i in xrange(self.shard_count)
        ],
      }

      # Don't store stdout in the summary if not requested too.
      if "json" not in self.task_output_stdout:
        for shard_json in summary['shards']:
          if not shard_json:
            continue
          if "output" in shard_json:
            del shard_json["output"]
          if "outputs" in shard_json:
            del shard_json["outputs"]

      # Write summary.json to task_output_dir as well.
      if self.task_output_dir:
        tools.write_json(
            os.path.join(self.task_output_dir, u'summary.json'),
            summary,
            False)
      if self._storage:
        self._storage.close()
        self._storage = None
      return summary

  def _get_storage(self, isolate_server, namespace):
    """Returns isolateserver.Storage to use to fetch files."""
    assert self.task_output_dir
    with self._lock:
      if not self._storage:
        self._storage = isolateserver.get_storage(isolate_server, namespace)
      else:
        # Shards must all use exact same isolate server and namespace.
        if self._storage.location != isolate_server:
          logging.error(
              'Task shards are using multiple isolate servers: %s and %s',
              self._storage.location, isolate_server)
          return None
        if self._storage.namespace != namespace:
          logging.error(
              'Task shards are using multiple namespaces: %s and %s',
              self._storage.namespace, namespace)
          return None
      return self._storage


def now():
  """Exists so it can be mocked easily."""
  return time.time()


def parse_time(value):
  """Converts serialized time from the API to datetime.datetime."""
  # When microseconds are 0, the '.123456' suffix is elided. This means the
  # serialized format is not consistent, which confuses the hell out of python.
  for fmt in ('%Y-%m-%dT%H:%M:%S.%f', '%Y-%m-%dT%H:%M:%S'):
    try:
      return datetime.datetime.strptime(value, fmt)
    except ValueError:
      pass
  raise ValueError('Failed to parse %s' % value)


def retrieve_results(
    base_url, shard_index, task_id, timeout, should_stop, output_collector,
    include_perf, fetch_stdout):
  """Retrieves results for a single task ID.

  Returns:
    <result dict> on success.
    None on failure.
  """
  assert timeout is None or isinstance(timeout, float), timeout
  result_url = '%s/api/swarming/v1/task/%s/result' % (base_url, task_id)
  if include_perf:
    result_url += '?include_performance_stats=true'
  output_url = '%s/api/swarming/v1/task/%s/stdout' % (base_url, task_id)
  started = now()
  deadline = started + timeout if timeout > 0 else None
  attempt = 0

  while not should_stop.is_set():
    attempt += 1

    # Waiting for too long -> give up.
    current_time = now()
    if deadline and current_time >= deadline:
      logging.error('retrieve_results(%s) timed out on attempt %d',
          base_url, attempt)
      return None

    # Do not spin too fast. Spin faster at the beginning though.
    # Start with 1 sec delay and for each 30 sec of waiting add another second
    # of delay, until hitting 15 sec ceiling.
    if attempt > 1:
      max_delay = min(15, 1 + (current_time - started) / 30.0)
      delay = min(max_delay, deadline - current_time) if deadline else max_delay
      if delay > 0:
        logging.debug('Waiting %.1f sec before retrying', delay)
        should_stop.wait(delay)
        if should_stop.is_set():
          return None

    # Disable internal retries in net.url_read_json, since we are doing retries
    # ourselves.
    # TODO(maruel): We'd need to know if it's a 404 and not retry at all.
    # TODO(maruel): Sadly, we currently have to poll here. Use hanging HTTP
    # request on GAE v2.
    # Retry on 500s only if no timeout is specified.
    result = net.url_read_json(result_url, retry_50x=bool(timeout == -1))
    if not result:
      if timeout == -1:
        return None
      continue

    if result.get('error'):
      # An error occurred.
      if result['error'].get('errors'):
        for err in result['error']['errors']:
          logging.warning(
              'Error while reading task: %s; %s',
              err.get('message'), err.get('debugInfo'))
      elif result['error'].get('message'):
        logging.warning(
            'Error while reading task: %s', result['error']['message'])
      if timeout == -1:
        return result
      continue

    # When timeout == -1, always return on first attempt. 500s are already
    # retried in this case.
    if result['state'] in State.STATES_NOT_RUNNING or timeout == -1:
      if fetch_stdout:
        out = net.url_read_json(output_url)
        result['output'] = out.get('output', '') if out else ''
      # Record the result, try to fetch attached output files (if any).
      if output_collector:
        # TODO(vadimsh): Respect |should_stop| and |deadline| when fetching.
        output_collector.process_shard_result(shard_index, result)
      if result.get('internal_failure'):
        logging.error('Internal error!')
      elif result['state'] == 'BOT_DIED':
        logging.error('Bot died!')
      return result


def convert_to_old_format(result):
  """Converts the task result data from Endpoints API format to old API format
  for compatibility.

  This goes into the file generated as --task-summary-json.
  """
  # Sets default.
  result.setdefault('abandoned_ts', None)
  result.setdefault('bot_id', None)
  result.setdefault('bot_version', None)
  result.setdefault('children_task_ids', [])
  result.setdefault('completed_ts', None)
  result.setdefault('cost_saved_usd', None)
  result.setdefault('costs_usd', None)
  result.setdefault('deduped_from', None)
  result.setdefault('name', None)
  result.setdefault('outputs_ref', None)
  result.setdefault('server_versions', None)
  result.setdefault('started_ts', None)
  result.setdefault('tags', None)
  result.setdefault('user', None)

  # Convertion back to old API.
  duration = result.pop('duration', None)
  result['durations'] = [duration] if duration else []
  exit_code = result.pop('exit_code', None)
  result['exit_codes'] = [int(exit_code)] if exit_code else []
  result['id'] = result.pop('task_id')
  result['isolated_out'] = result.get('outputs_ref', None)
  output = result.pop('output', None)
  result['outputs'] = [output] if output else []
  # server_version
  # Endpoints result 'state' as string. For compatibility with old code, convert
  # to int.
  result['state'] = State.from_enum(result['state'])
  result['try_number'] = (
      int(result['try_number']) if result.get('try_number') else None)
  if 'bot_dimensions' in result:
    result['bot_dimensions'] = {
      i['key']: i.get('value', []) for i in result['bot_dimensions']
    }
  else:
    result['bot_dimensions'] = None


def yield_results(
    swarm_base_url, task_ids, timeout, max_threads, print_status_updates,
    output_collector, include_perf, fetch_stdout):
  """Yields swarming task results from the swarming server as (index, result).

  Duplicate shards are ignored. Shards are yielded in order of completion.
  Timed out shards are NOT yielded at all. Caller can compare number of yielded
  shards with len(task_keys) to verify all shards completed.

  max_threads is optional and is used to limit the number of parallel fetches
  done. Since in general the number of task_keys is in the range <=10, it's not
  worth normally to limit the number threads. Mostly used for testing purposes.

  output_collector is an optional instance of TaskOutputCollector that will be
  used to fetch files produced by a task from isolate server to the local disk.

  Yields:
    (index, result). In particular, 'result' is defined as the
    GetRunnerResults() function in services/swarming/server/test_runner.py.
  """
  number_threads = (
      min(max_threads, len(task_ids)) if max_threads else len(task_ids))
  should_stop = threading.Event()
  results_channel = threading_utils.TaskChannel()

  with threading_utils.ThreadPool(number_threads, number_threads, 0) as pool:
    try:
      # Adds a task to the thread pool to call 'retrieve_results' and return
      # the results together with shard_index that produced them (as a tuple).
      def enqueue_retrieve_results(shard_index, task_id):
        task_fn = lambda *args: (shard_index, retrieve_results(*args))
        pool.add_task(
            0, results_channel.wrap_task(task_fn), swarm_base_url, shard_index,
            task_id, timeout, should_stop, output_collector, include_perf,
            fetch_stdout)

      # Enqueue 'retrieve_results' calls for each shard key to run in parallel.
      for shard_index, task_id in enumerate(task_ids):
        enqueue_retrieve_results(shard_index, task_id)

      # Wait for all of them to finish.
      shards_remaining = range(len(task_ids))
      active_task_count = len(task_ids)
      while active_task_count:
        shard_index, result = None, None
        try:
          shard_index, result = results_channel.pull(
              timeout=STATUS_UPDATE_INTERVAL)
        except threading_utils.TaskChannel.Timeout:
          if print_status_updates:
            print(
                'Waiting for results from the following shards: %s' %
                ', '.join(map(str, shards_remaining)))
            sys.stdout.flush()
          continue
        except Exception:
          logging.exception('Unexpected exception in retrieve_results')

        # A call to 'retrieve_results' finished (successfully or not).
        active_task_count -= 1
        if not result:
          logging.error('Failed to retrieve the results for a swarming key')
          continue

        # Yield back results to the caller.
        assert shard_index in shards_remaining
        shards_remaining.remove(shard_index)
        yield shard_index, result

    finally:
      # Done or aborted with Ctrl+C, kill the remaining threads.
      should_stop.set()


def decorate_shard_output(swarming, shard_index, metadata, include_stdout):
  """Returns wrapped output for swarming task shard."""
  if metadata.get('started_ts') and not metadata.get('deduped_from'):
    pending = '%.1fs' % (
        parse_time(metadata['started_ts']) - parse_time(metadata['created_ts'])
        ).total_seconds()
  elif (metadata.get('state') in ('BOT_DIED', 'CANCELED', 'EXPIRED') and
        metadata.get('abandoned_ts')):
    pending = '%.1fs' % (
        parse_time(metadata['abandoned_ts']) -
            parse_time(metadata['created_ts'])
        ).total_seconds()
  else:
    pending = 'N/A'

  if metadata.get('duration') is not None:
    duration = '%.1fs' % metadata['duration']
  else:
    duration = 'N/A'

  if metadata.get('exit_code') is not None:
    # Integers are encoded as string to not loose precision.
    exit_code = '%s' % metadata['exit_code']
  else:
    exit_code = 'N/A'

  bot_id = metadata.get('bot_id') or 'N/A'

  url = '%s/user/task/%s' % (swarming, metadata['task_id'])
  tag_header = 'Shard %d  %s' % (shard_index, url)
  tag_footer1 = 'End of shard %d' % (shard_index)
  if metadata.get('state') == 'CANCELED':
    tag_footer2 = ' Pending: %s  CANCELED' % pending
  elif metadata.get('state') == 'EXPIRED':
    tag_footer2 = ' Pending: %s  EXPIRED (lack of capacity)' % pending
  elif metadata.get('state') in ('BOT_DIED', 'TIMED_OUT', 'KILLED'):
    tag_footer2 = ' Pending: %s  Duration: %s  Bot: %s  Exit: %s  %s' % (
        pending, duration, bot_id, exit_code, metadata['state'])
  else:
    tag_footer2 = ' Pending: %s  Duration: %s  Bot: %s  Exit: %s' % (
        pending, duration, bot_id, exit_code)

  tag_len = max(len(x) for x in [tag_header, tag_footer1, tag_footer2])
  dash_pad = '+-%s-+' % ('-' * tag_len)
  tag_header = '| %s |' % tag_header.ljust(tag_len)
  tag_footer1 = '| %s |' % tag_footer1.ljust(tag_len)
  tag_footer2 = '| %s |' % tag_footer2.ljust(tag_len)

  if include_stdout:
    return '\n'.join([
        dash_pad,
        tag_header,
        dash_pad,
        (metadata.get('output') or '').rstrip(),
        dash_pad,
        tag_footer1,
        tag_footer2,
        dash_pad,
        ])
  else:
    return '\n'.join([
        dash_pad,
        tag_header,
        tag_footer2,
        dash_pad,
        ])


def collect(
    swarming, task_ids, timeout, decorate, print_status_updates,
    task_summary_json, task_output_dir, task_output_stdout,
    include_perf):
  """Retrieves results of a Swarming task.

  Returns:
    process exit code that should be returned to the user.
  """
  # Collect summary JSON and output files (if task_output_dir is not None).
  output_collector = TaskOutputCollector(
      task_output_dir, task_output_stdout, len(task_ids))

  seen_shards = set()
  exit_code = None
  total_duration = 0
  try:
    for index, metadata in yield_results(
        swarming, task_ids, timeout, None, print_status_updates,
        output_collector, include_perf,
        (len(task_output_stdout) > 0),
        ):
      seen_shards.add(index)

      # Default to failure if there was no process that even started.
      shard_exit_code = metadata.get('exit_code')
      if shard_exit_code:
        # It's encoded as a string, so bool('0') is True.
        shard_exit_code = int(shard_exit_code)
      if shard_exit_code or exit_code is None:
        exit_code = shard_exit_code
      total_duration += metadata.get('duration', 0)

      if decorate:
        s = decorate_shard_output(
            swarming, index, metadata,
            "console" in task_output_stdout).encode(
                'utf-8', 'replace')
        print(s)
        if len(seen_shards) < len(task_ids):
          print('')
      else:
        print('%s: %s %s' % (
            metadata.get('bot_id', 'N/A'),
            metadata['task_id'],
            shard_exit_code))
        if "console" in task_output_stdout and metadata['output']:
          output = metadata['output'].rstrip()
          if output:
            print(''.join('  %s\n' % l for l in output.splitlines()))
  finally:
    summary = output_collector.finalize()
    if task_summary_json:
      # TODO(maruel): Make this optional.
      for i in summary['shards']:
        if i:
          convert_to_old_format(i)
      tools.write_json(task_summary_json, summary, False)

  if decorate and total_duration:
    print('Total duration: %.1fs' % total_duration)

  if len(seen_shards) != len(task_ids):
    missing_shards = [x for x in range(len(task_ids)) if x not in seen_shards]
    print >> sys.stderr, ('Results from some shards are missing: %s' %
        ', '.join(map(str, missing_shards)))
    return 1

  return exit_code if exit_code is not None else 1


### API management.


class APIError(Exception):
  pass


def endpoints_api_discovery_apis(host):
  """Uses Cloud Endpoints' API Discovery Service to returns metadata about all
  the APIs exposed by a host.

  https://developers.google.com/discovery/v1/reference/apis/list
  """
  # Uses the real Cloud Endpoints. This needs to be fixed once the Cloud
  # Endpoints version is turned down.
  data = net.url_read_json(host + '/_ah/api/discovery/v1/apis')
  if data is None:
    raise APIError('Failed to discover APIs on %s' % host)
  out = {}
  for api in data['items']:
    if api['id'] == 'discovery:v1':
      continue
    # URL is of the following form:
    # url = host + (
    #   '/_ah/api/discovery/v1/apis/%s/%s/rest' % (api['id'], api['version'])
    api_data = net.url_read_json(api['discoveryRestUrl'])
    if api_data is None:
      raise APIError('Failed to discover %s on %s' % (api['id'], host))
    out[api['id']] = api_data
  return out


def get_yielder(base_url, limit):
  """Returns the first query and a function that yields following items."""
  CHUNK_SIZE = 250

  url = base_url
  if limit:
    url += '%slimit=%d' % ('&' if '?' in url else '?', min(CHUNK_SIZE, limit))
  data = net.url_read_json(url)
  if data is None:
    # TODO(maruel): Do basic diagnostic.
    raise Failure('Failed to access %s' % url)
  org_cursor = data.pop('cursor', None)
  org_total = len(data.get('items') or [])
  logging.info('get_yielder(%s) returning %d items', base_url, org_total)
  if not org_cursor or not org_total:
    # This is not an iterable resource.
    return data, lambda: []

  def yielder():
    cursor = org_cursor
    total = org_total
    # Some items support cursors. Try to get automatically if cursors are needed
    # by looking at the 'cursor' items.
    while cursor and (not limit or total < limit):
      merge_char = '&' if '?' in base_url else '?'
      url = base_url + '%scursor=%s' % (merge_char, urllib.quote(cursor))
      if limit:
        url += '&limit=%d' % min(CHUNK_SIZE, limit - total)
      new = net.url_read_json(url)
      if new is None:
        raise Failure('Failed to access %s' % url)
      cursor = new.get('cursor')
      new_items = new.get('items')
      nb_items = len(new_items or [])
      total += nb_items
      logging.info('get_yielder(%s) yielding %d items', base_url, nb_items)
      yield new_items

  return data, yielder


### Commands.


def abort_task(_swarming, _manifest):
  """Given a task manifest that was triggered, aborts its execution."""
  # TODO(vadimsh): No supported by the server yet.


def add_filter_options(parser):
  parser.filter_group = optparse.OptionGroup(parser, 'Bot selection')
  parser.filter_group.add_option(
      '-d', '--dimension', default=[], action='append', nargs=2,
      dest='dimensions', metavar='FOO bar',
      help='dimension to filter on')
  parser.add_option_group(parser.filter_group)


def process_filter_options(parser, options):
  for key, value in options.dimensions:
    if ':' in key:
      parser.error('--dimension key cannot contain ":"')
    if key.strip() != key:
      parser.error('--dimension key has whitespace')
    if not key:
      parser.error('--dimension key is empty')

    if value.strip() != value:
      parser.error('--dimension value has whitespace')
    if not value:
      parser.error('--dimension value is empty')
  options.dimensions.sort()


def add_sharding_options(parser):
  parser.sharding_group = optparse.OptionGroup(parser, 'Sharding options')
  parser.sharding_group.add_option(
      '--shards', type='int', default=1, metavar='NUMBER',
      help='Number of shards to trigger and collect.')
  parser.add_option_group(parser.sharding_group)


def add_trigger_options(parser):
  """Adds all options to trigger a task on Swarming."""
  isolateserver.add_isolate_server_options(parser)
  add_filter_options(parser)

  group = optparse.OptionGroup(parser, 'Task properties')
  group.add_option(
      '-s', '--isolated', metavar='HASH',
      help='Hash of the .isolated to grab from the isolate server')
  group.add_option(
      '-e', '--env', default=[], action='append', nargs=2, metavar='FOO bar',
      help='Environment variables to set')
  group.add_option(
      '--env-prefix', default=[], action='append', nargs=2,
      metavar='VAR local/path',
      help='Prepend task-relative `local/path` to the task\'s VAR environment '
           'variable using os-appropriate pathsep character. Can be specified '
           'multiple times for the same VAR to add multiple paths.')
  group.add_option(
      '--idempotent', action='store_true', default=False,
      help='When set, the server will actively try to find a previous task '
           'with the same parameter and return this result instead if possible')
  group.add_option(
      '--secret-bytes-path', metavar='FILE',
      help='The optional path to a file containing the secret_bytes to use with'
           'this task.')
  group.add_option(
      '--hard-timeout', type='int', default=60*60, metavar='SECS',
      help='Seconds to allow the task to complete.')
  group.add_option(
      '--io-timeout', type='int', default=20*60, metavar='SECS',
      help='Seconds to allow the task to be silent.')
  group.add_option(
      '--raw-cmd', action='store_true', default=False,
      help='When set, the command after -- is used as-is without run_isolated. '
           'In this case, the .isolated file is expected to not have a command')
  group.add_option(
      '--relative-cwd',
      help='Ignore the isolated \'relative_cwd\' and use this one instead; '
           'requires --raw-cmd')
  group.add_option(
      '--cipd-package', action='append', default=[], metavar='PKG',
      help='CIPD packages to install on the Swarming bot. Uses the format: '
           'path:package_name:version')
  group.add_option(
      '--named-cache', action='append', nargs=2, default=[],
      metavar='NAME RELPATH',
      help='"<name> <relpath>" items to keep a persistent bot managed cache')
  group.add_option(
      '--service-account',
      help='Email of a service account to run the task as, or literal "bot" '
           'string to indicate that the task should use the same account the '
           'bot itself is using to authenticate to Swarming. Don\'t use task '
           'service accounts if not given (default).')
  group.add_option(
      '-o', '--output', action='append', default=[], metavar='PATH',
      help='A list of files to return in addition to those written to '
           '${ISOLATED_OUTDIR}. An error will occur if a file specified by'
           'this option is also written directly to ${ISOLATED_OUTDIR}.')
  parser.add_option_group(group)

  group = optparse.OptionGroup(parser, 'Task request')
  group.add_option(
      '--priority', type='int', default=100,
      help='The lower value, the more important the task is')
  group.add_option(
      '-T', '--task-name', metavar='NAME',
      help='Display name of the task. Defaults to '
           '<base_name>/<dimensions>/<isolated hash>/<timestamp> if an '
           'isolated file is provided, if a hash is provided, it defaults to '
           '<user>/<dimensions>/<isolated hash>/<timestamp>')
  group.add_option(
      '--tags', action='append', default=[], metavar='FOO:BAR',
      help='Tags to assign to the task.')
  group.add_option(
      '--user', default='',
      help='User associated with the task. Defaults to authenticated user on '
           'the server.')
  group.add_option(
      '--expiration', type='int', default=6*60*60, metavar='SECS',
      help='Seconds to allow the task to be pending for a bot to run before '
           'this task request expires.')
  group.add_option(
      '--deadline', type='int', dest='expiration',
      help=optparse.SUPPRESS_HELP)
  parser.add_option_group(group)


def process_trigger_options(parser, options, args):
  """Processes trigger options and does preparatory steps.

  Returns:
    NewTaskRequest instance.
  """
  process_filter_options(parser, options)
  options.env = dict(options.env)
  if args and args[0] == '--':
    args = args[1:]

  if not options.dimensions:
    parser.error('Please at least specify one --dimension')
  if not all(len(t.split(':', 1)) == 2 for t in options.tags):
    parser.error('--tags must be in the format key:value')
  if options.raw_cmd and not args:
    parser.error(
        'Arguments with --raw-cmd should be passed after -- as command '
        'delimiter.')
  if options.isolate_server and not options.namespace:
    parser.error(
        '--namespace must be a valid value when --isolate-server is used')
  if not options.isolated and not options.raw_cmd:
    parser.error('Specify at least one of --raw-cmd or --isolated or both')

  # Isolated
  # --isolated is required only if --raw-cmd wasn't provided.
  # TODO(maruel): --isolate-server may be optional as Swarming may have its own
  # preferred server.
  isolateserver.process_isolate_server_options(
      parser, options, False, not options.raw_cmd)
  inputs_ref = None
  if options.isolate_server:
    inputs_ref = FilesRef(
        isolated=options.isolated,
        isolatedserver=options.isolate_server,
        namespace=options.namespace)

  # Command
  command = None
  extra_args = None
  if options.raw_cmd:
    command = args
    if options.relative_cwd:
      a = os.path.normpath(os.path.abspath(options.relative_cwd))
      if not a.startswith(os.getcwd()):
        parser.error(
            '--relative-cwd must not try to escape the working directory')
  else:
    if options.relative_cwd:
      parser.error('--relative-cwd requires --raw-cmd')
    extra_args = args

  # CIPD
  cipd_packages = []
  for p in options.cipd_package:
    split = p.split(':', 2)
    if len(split) != 3:
      parser.error('CIPD packages must take the form: path:package:version')
    cipd_packages.append(CipdPackage(
        package_name=split[1],
        path=split[0],
        version=split[2]))
  cipd_input = None
  if cipd_packages:
    cipd_input = CipdInput(
        client_package=None,
        packages=cipd_packages,
        server=None)

  # Secrets
  secret_bytes = None
  if options.secret_bytes_path:
    with open(options.secret_bytes_path, 'r') as f:
      secret_bytes = f.read().encode('base64')

  # Named caches
  caches = [
    {u'name': unicode(i[0]), u'path': unicode(i[1])}
    for i in options.named_cache
  ]

  env_prefixes = {}
  for k, v in options.env_prefix:
    env_prefixes.setdefault(k, []).append(v)

  properties = TaskProperties(
      caches=caches,
      cipd_input=cipd_input,
      command=command,
      relative_cwd=options.relative_cwd,
      dimensions=options.dimensions,
      env=options.env,
      env_prefixes=[StringListPair(k, v) for k, v in env_prefixes.iteritems()],
      execution_timeout_secs=options.hard_timeout,
      extra_args=extra_args,
      grace_period_secs=30,
      idempotent=options.idempotent,
      inputs_ref=inputs_ref,
      io_timeout_secs=options.io_timeout,
      outputs=options.output,
      secret_bytes=secret_bytes)

  return NewTaskRequest(
      name=default_task_name(options),
      parent_task_id=os.environ.get('SWARMING_TASK_ID', ''),
      priority=options.priority,
      task_slices=[
        {
          'expiration_secs': options.expiration,
          'properties': properties,
        },
      ],
      service_account=options.service_account,
      tags=options.tags,
      user=options.user)


class TaskOutputStdoutOption(optparse.Option):
  """Where to output the each task's console output (stderr/stdout).

  The output will be;
   none    - not be downloaded.
   json    - stored in summary.json file *only*.
   console - shown on stdout *only*.
   all     - stored in summary.json and shown on stdout.
  """

  choices = ['all', 'json', 'console', 'none']

  def __init__(self, *args, **kw):
    optparse.Option.__init__(
        self,
        *args,
        choices=self.choices,
        default=['console', 'json'],
        help=re.sub('\s\s*', ' ', self.__doc__),
        **kw)

  def convert_value(self, opt, value):
    if value not in self.choices:
      raise optparse.OptionValueError("%s must be one of %s not %r" % (
          self.get_opt_string(), self.choices, value))
    stdout_to = []
    if value == 'all':
      stdout_to = ['console', 'json']
    elif value != 'none':
      stdout_to = [value]
    return stdout_to


def add_collect_options(parser):
  parser.server_group.add_option(
      '-t', '--timeout', type='float', default=0.,
      help='Timeout to wait for result, set to -1 for no timeout and get '
           'current state; defaults to waiting until the task completes')
  parser.group_logging.add_option(
      '--decorate', action='store_true', help='Decorate output')
  parser.group_logging.add_option(
      '--print-status-updates', action='store_true',
      help='Print periodic status updates')
  parser.task_output_group = optparse.OptionGroup(parser, 'Task output')
  parser.task_output_group.add_option(
      '--task-summary-json',
      metavar='FILE',
      help='Dump a summary of task results to this file as json. It contains '
           'only shards statuses as know to server directly. Any output files '
           'emitted by the task can be collected by using --task-output-dir')
  parser.task_output_group.add_option(
      '--task-output-dir',
      metavar='DIR',
      help='Directory to put task results into. When the task finishes, this '
           'directory contains per-shard directory with output files produced '
           'by shards: <task-output-dir>/<zero-based-shard-index>/.')
  parser.task_output_group.add_option(TaskOutputStdoutOption(
      '--task-output-stdout'))
  parser.task_output_group.add_option(
      '--perf', action='store_true', default=False,
      help='Includes performance statistics')
  parser.add_option_group(parser.task_output_group)


def process_collect_options(parser, options):
  # Only negative -1 is allowed, disallow other negative values.
  if options.timeout != -1 and options.timeout < 0:
    parser.error('Invalid --timeout value')


@subcommand.usage('bots...')
def CMDbot_delete(parser, args):
  """Forcibly deletes bots from the Swarming server."""
  parser.add_option(
      '-f', '--force', action='store_true',
      help='Do not prompt for confirmation')
  options, args = parser.parse_args(args)
  if not args:
    parser.error('Please specify bots to delete')

  bots = sorted(args)
  if not options.force:
    print('Delete the following bots?')
    for bot in bots:
      print('  %s' % bot)
    if raw_input('Continue? [y/N] ') not in ('y', 'Y'):
      print('Goodbye.')
      return 1

  result = 0
  for bot in bots:
    url = '%s/api/swarming/v1/bot/%s/delete' % (options.swarming, bot)
    if net.url_read_json(url, data={}, method='POST') is None:
      print('Deleting %s failed. Probably already gone' % bot)
      result = 1
  return result


def CMDbots(parser, args):
  """Returns information about the bots connected to the Swarming server."""
  add_filter_options(parser)
  parser.filter_group.add_option(
      '--dead-only', action='store_true',
      help='Filter out bots alive, useful to reap them and reimage broken bots')
  parser.filter_group.add_option(
      '-k', '--keep-dead', action='store_true',
      help='Keep both dead and alive bots')
  parser.filter_group.add_option(
      '--busy', action='store_true', help='Keep only busy bots')
  parser.filter_group.add_option(
      '--idle', action='store_true', help='Keep only idle bots')
  parser.filter_group.add_option(
      '--mp', action='store_true',
      help='Keep only Machine Provider managed bots')
  parser.filter_group.add_option(
      '--non-mp', action='store_true',
      help='Keep only non Machine Provider managed bots')
  parser.filter_group.add_option(
      '-b', '--bare', action='store_true',
      help='Do not print out dimensions')
  options, args = parser.parse_args(args)
  process_filter_options(parser, options)

  if options.keep_dead and options.dead_only:
    parser.error('Use only one of --keep-dead or --dead-only')
  if options.busy and options.idle:
    parser.error('Use only one of --busy or --idle')
  if options.mp and options.non_mp:
    parser.error('Use only one of --mp or --non-mp')

  url = options.swarming + '/api/swarming/v1/bots/list?'
  values = []
  if options.dead_only:
    values.append(('is_dead', 'TRUE'))
  elif options.keep_dead:
    values.append(('is_dead', 'NONE'))
  else:
    values.append(('is_dead', 'FALSE'))

  if options.busy:
    values.append(('is_busy', 'TRUE'))
  elif options.idle:
    values.append(('is_busy', 'FALSE'))
  else:
    values.append(('is_busy', 'NONE'))

  if options.mp:
    values.append(('is_mp', 'TRUE'))
  elif options.non_mp:
    values.append(('is_mp', 'FALSE'))
  else:
    values.append(('is_mp', 'NONE'))

  for key, value in options.dimensions:
    values.append(('dimensions', '%s:%s' % (key, value)))
  url += urllib.urlencode(values)
  try:
    data, yielder = get_yielder(url, 0)
    bots = data.get('items') or []
    for items in yielder():
      if items:
        bots.extend(items)
  except Failure as e:
    sys.stderr.write('\n%s\n' % e)
    return 1
  for bot in natsort.natsorted(bots, key=lambda x: x['bot_id']):
    print bot['bot_id']
    if not options.bare:
      dimensions = {i['key']: i.get('value') for i in bot.get('dimensions', {})}
      print '  %s' % json.dumps(dimensions, sort_keys=True)
      if bot.get('task_id'):
        print '  task: %s' % bot['task_id']
  return 0


@subcommand.usage('task_id')
def CMDcancel(parser, args):
  """Cancels a task."""
  parser.add_option(
      '-k', '--kill-running', action='store_true', default=False,
      help='Kill the task even if it was running')
  options, args = parser.parse_args(args)
  if not args:
    parser.error('Please specify the task to cancel')
  data = {'kill_running': options.kill_running}
  for task_id in args:
    url = '%s/api/swarming/v1/task/%s/cancel' % (options.swarming, task_id)
    resp = net.url_read_json(url, data=data, method='POST')
    if resp is None:
      print('Deleting %s failed. Probably already gone' % task_id)
      return 1
    logging.info('%s', resp)
  return 0


@subcommand.usage('--json file | task_id...')
def CMDcollect(parser, args):
  """Retrieves results of one or multiple Swarming task by its ID.

  The result can be in multiple part if the execution was sharded. It can
  potentially have retries.
  """
  add_collect_options(parser)
  parser.add_option(
      '-j', '--json',
      help='Load the task ids from .json as saved by trigger --dump-json')
  options, args = parser.parse_args(args)
  process_collect_options(parser, options)
  if not args and not options.json:
    parser.error('Must specify at least one task id or --json.')
  if args and options.json:
    parser.error('Only use one of task id or --json.')

  if options.json:
    options.json = unicode(os.path.abspath(options.json))
    try:
      with fs.open(options.json, 'rb') as f:
        data = json.load(f)
    except (IOError, ValueError):
      parser.error('Failed to open %s' % options.json)
    try:
      tasks = sorted(
          data['tasks'].itervalues(), key=lambda x: x['shard_index'])
      args = [t['task_id'] for t in tasks]
    except (KeyError, TypeError):
      parser.error('Failed to process %s' % options.json)
    if not options.timeout:
      # Take in account all the task slices.
      offset = 0
      for s in data['request']['task_slices']:
        m = (offset + s['properties']['execution_timeout_secs'] +
             s['expiration_secs'])
        if m > options.timeout:
          options.timeout = m
        offset += s['expiration_secs']
      options.timeout += 10.
  else:
    valid = frozenset('0123456789abcdef')
    if any(not valid.issuperset(task_id) for task_id in args):
      parser.error('Task ids are 0-9a-f.')

  try:
    return collect(
        options.swarming,
        args,
        options.timeout,
        options.decorate,
        options.print_status_updates,
        options.task_summary_json,
        options.task_output_dir,
        options.task_output_stdout,
        options.perf)
  except Failure:
    on_error.report(None)
    return 1


@subcommand.usage('[method name]')
def CMDpost(parser, args):
  """Sends a JSON RPC POST to one API endpoint and prints out the raw result.

  Input data must be sent to stdin, result is printed to stdout.

  If HTTP response code >= 400, returns non-zero.
  """
  options, args = parser.parse_args(args)
  if len(args) != 1:
    parser.error('Must specify only API name')
  url = options.swarming + '/api/swarming/v1/' + args[0]
  data = sys.stdin.read()
  try:
    resp = net.url_read(url, data=data, method='POST')
  except net.TimeoutError:
    sys.stderr.write('Timeout!\n')
    return 1
  if not resp:
    sys.stderr.write('No response!\n')
    return 1
  sys.stdout.write(resp)
  return 0


@subcommand.usage('[method name]')
def CMDquery(parser, args):
  """Returns raw JSON information via an URL endpoint. Use 'query-list' to
  gather the list of API methods from the server.

  Examples:
    Raw task request and results:
      swarming.py query -S server-url.com task/123456/request
      swarming.py query -S server-url.com task/123456/result

    Listing all bots:
      swarming.py query -S server-url.com bots/list

    Listing last 10 tasks on a specific bot named 'bot1':
      swarming.py query -S server-url.com --limit 10 bot/bot1/tasks

    Listing last 10 tasks with tags os:Ubuntu-14.04 and pool:Chrome. Note that
    quoting is important!:
      swarming.py query -S server-url.com --limit 10 \\
          'tasks/list?tags=os:Ubuntu-14.04&tags=pool:Chrome'
  """
  parser.add_option(
      '-L', '--limit', type='int', default=200,
      help='Limit to enforce on limitless items (like number of tasks); '
           'default=%default')
  parser.add_option(
      '--json', help='Path to JSON output file (otherwise prints to stdout)')
  parser.add_option(
      '--progress', action='store_true',
      help='Prints a dot at each request to show progress')
  options, args = parser.parse_args(args)
  if len(args) != 1:
    parser.error(
        'Must specify only method name and optionally query args properly '
        'escaped.')
  base_url = options.swarming + '/api/swarming/v1/' + args[0]
  try:
    data, yielder = get_yielder(base_url, options.limit)
    for items in yielder():
      if items:
        data['items'].extend(items)
      if options.progress:
        sys.stderr.write('.')
        sys.stderr.flush()
  except Failure as e:
    sys.stderr.write('\n%s\n' % e)
    return 1
  if options.progress:
    sys.stderr.write('\n')
    sys.stderr.flush()
  if options.json:
    options.json = unicode(os.path.abspath(options.json))
    tools.write_json(options.json, data, True)
  else:
    try:
      tools.write_json(sys.stdout, data, False)
      sys.stdout.write('\n')
    except IOError:
      pass
  return 0


def CMDquery_list(parser, args):
  """Returns list of all the Swarming APIs that can be used with command
  'query'.
  """
  parser.add_option(
      '--json', help='Path to JSON output file (otherwise prints to stdout)')
  options, args = parser.parse_args(args)
  if args:
    parser.error('No argument allowed.')

  try:
    apis = endpoints_api_discovery_apis(options.swarming)
  except APIError as e:
    parser.error(str(e))
  if options.json:
    options.json = unicode(os.path.abspath(options.json))
    with fs.open(options.json, 'wb') as f:
      json.dump(apis, f)
  else:
    help_url = (
      'https://apis-explorer.appspot.com/apis-explorer/?base=%s/_ah/api#p/' %
      options.swarming)
    for i, (api_id, api) in enumerate(sorted(apis.iteritems())):
      if i:
        print('')
      print api_id
      print '  ' + api['description'].strip()
      if 'resources' in api:
        # Old.
        for j, (resource_name, resource) in enumerate(
            sorted(api['resources'].iteritems())):
          if j:
            print('')
          for method_name, method in sorted(resource['methods'].iteritems()):
            # Only list the GET ones.
            if method['httpMethod'] != 'GET':
              continue
            print '- %s.%s: %s' % (
                resource_name, method_name, method['path'])
            print('\n'.join(
                '  ' + l for l in textwrap.wrap(
                    method.get('description', 'No description'), 78)))
            print '  %s%s%s' % (help_url, api['servicePath'], method['id'])
      else:
        # New.
        for method_name, method in sorted(api['methods'].iteritems()):
          # Only list the GET ones.
          if method['httpMethod'] != 'GET':
            continue
          print '- %s: %s' % (method['id'], method['path'])
          print('\n'.join(
              '  ' + l for l in textwrap.wrap(method['description'], 78)))
          print '  %s%s%s' % (help_url, api['servicePath'], method['id'])
  return 0


@subcommand.usage('(hash|isolated) [-- extra_args]')
def CMDrun(parser, args):
  """Triggers a task and wait for the results.

  Basically, does everything to run a command remotely.
  """
  add_trigger_options(parser)
  add_collect_options(parser)
  add_sharding_options(parser)
  options, args = parser.parse_args(args)
  process_collect_options(parser, options)
  task_request = process_trigger_options(parser, options, args)
  try:
    tasks = trigger_task_shards(
        options.swarming, task_request, options.shards)
  except Failure as e:
    on_error.report(
        'Failed to trigger %s(%s): %s' %
        (task_request.name, args[0], e.args[0]))
    return 1
  if not tasks:
    on_error.report('Failed to trigger the task.')
    return 1
  print('Triggered task: %s' % task_request.name)
  task_ids = [
    t['task_id']
    for t in sorted(tasks.itervalues(), key=lambda x: x['shard_index'])
  ]
  if not options.timeout:
    offset = 0
    for s in task_request.task_slices:
      m = (offset + s['properties'].execution_timeout_secs +
            s['expiration_secs'])
      if m > options.timeout:
        options.timeout = m
      offset += s['expiration_secs']
    options.timeout += 10.
  try:
    return collect(
        options.swarming,
        task_ids,
        options.timeout,
        options.decorate,
        options.print_status_updates,
        options.task_summary_json,
        options.task_output_dir,
        options.task_output_stdout,
        options.perf)
  except Failure:
    on_error.report(None)
    return 1


@subcommand.usage('task_id -- <extra_args>')
def CMDreproduce(parser, args):
  """Runs a task locally that was triggered on the server.

  This running locally the same commands that have been run on the bot. The data
  downloaded will be in a subdirectory named 'work' of the current working
  directory.

  You can pass further additional arguments to the target command by passing
  them after --.
  """
  parser.add_option(
      '--output-dir', metavar='DIR', default='out',
      help='Directory that will have results stored into')
  options, args = parser.parse_args(args)
  extra_args = []
  if not args:
    parser.error('Must specify exactly one task id.')
  if len(args) > 1:
    if args[1] == '--':
      if len(args) > 2:
        extra_args = args[2:]
    else:
      extra_args = args[1:]

  url = options.swarming + '/api/swarming/v1/task/%s/request' % args[0]
  request = net.url_read_json(url)
  if not request:
    print >> sys.stderr, 'Failed to retrieve request data for the task'
    return 1

  workdir = unicode(os.path.abspath('work'))
  if fs.isdir(workdir):
    parser.error('Please delete the directory \'work\' first')
  fs.mkdir(workdir)
  cachedir = unicode(os.path.abspath('cipd_cache'))
  if not fs.exists(cachedir):
    fs.mkdir(cachedir)

  properties = request['properties']
  env = os.environ.copy()
  env['SWARMING_BOT_ID'] = 'reproduce'
  env['SWARMING_TASK_ID'] = 'reproduce'
  if properties.get('env'):
    logging.info('env: %r', properties['env'])
    for i in properties['env']:
      key = i['key']
      if not i['value']:
        env.pop(key, None)
      else:
        env[key] = i['value']

  if properties.get('env_prefixes'):
    env_prefixes = properties['env_prefixes']
    logging.info('env_prefixes: %r', env_prefixes)
    for i in env_prefixes:
      key = i['key']
      paths = [os.path.normpath(os.path.join(workdir, p)) for p in i['value']]
      cur = env.get(key)
      if cur:
        paths.append(cur)
      env[key] = os.path.pathsep.join(paths)

  command = []
  if (properties.get('inputs_ref') or {}).get('isolated'):
    # Create the tree.
    with isolateserver.get_storage(
          properties['inputs_ref']['isolatedserver'],
          properties['inputs_ref']['namespace']) as storage:
      bundle = isolateserver.fetch_isolated(
          properties['inputs_ref']['isolated'],
          storage,
          isolateserver.MemoryCache(file_mode_mask=0700),
          workdir,
          False)
      command = bundle.command
      if bundle.relative_cwd:
        workdir = os.path.join(workdir, bundle.relative_cwd)
      command.extend(properties.get('extra_args') or [])

  if properties.get('command'):
    command.extend(properties['command'])

  # https://github.com/luci/luci-py/blob/master/appengine/swarming/doc/Magic-Values.md
  command = tools.fix_python_cmd(command, env)
  if not options.output_dir:
    new_command = run_isolated.process_command(command, 'invalid', None)
    if new_command != command:
      parser.error('The task has outputs, you must use --output-dir')
  else:
    # Make the path absolute, as the process will run from a subdirectory.
    options.output_dir = os.path.abspath(options.output_dir)
    new_command = run_isolated.process_command(
        command, options.output_dir, None)
    if not os.path.isdir(options.output_dir):
      os.makedirs(options.output_dir)
  command = new_command
  file_path.ensure_command_has_abs_path(command, workdir)

  if properties.get('cipd_input'):
    ci = properties['cipd_input']
    cp = ci['client_package']
    client_manager = cipd.get_client(
        ci['server'], cp['package_name'], cp['version'], cachedir)

    with client_manager as client:
      by_path = collections.defaultdict(list)
      for pkg in ci['packages']:
        path = pkg['path']
        # cipd deals with 'root' as ''
        if path == '.':
          path = ''
        by_path[path].append((pkg['package_name'], pkg['version']))
      client.ensure(workdir, by_path, cache_dir=cachedir)

  try:
    return subprocess42.call(command + extra_args, env=env, cwd=workdir)
  except OSError as e:
    print >> sys.stderr, 'Failed to run: %s' % ' '.join(command)
    print >> sys.stderr, str(e)
    return 1


@subcommand.usage('bot_id')
def CMDterminate(parser, args):
  """Tells a bot to gracefully shut itself down as soon as it can.

  This is done by completing whatever current task there is then exiting the bot
  process.
  """
  parser.add_option(
      '--wait', action='store_true', help='Wait for the bot to terminate')
  options, args = parser.parse_args(args)
  if len(args) != 1:
    parser.error('Please provide the bot id')
  url = options.swarming + '/api/swarming/v1/bot/%s/terminate' % args[0]
  request = net.url_read_json(url, data={})
  if not request:
    print >> sys.stderr, 'Failed to ask for termination'
    return 1
  if options.wait:
    return collect(
        options.swarming,
        [request['task_id']],
        0.,
        False,
        False,
        None,
        None,
        [],
        False)
  else:
    print request['task_id']
  return 0


@subcommand.usage("(hash|isolated) [-- extra_args|raw command]")
def CMDtrigger(parser, args):
  """Triggers a Swarming task.

  Passes all extra arguments provided after '--' as additional command line
  arguments for an isolated command specified in *.isolate file.
  """
  add_trigger_options(parser)
  add_sharding_options(parser)
  parser.add_option(
      '--dump-json',
      metavar='FILE',
      help='Dump details about the triggered task(s) to this file as json')
  options, args = parser.parse_args(args)
  task_request = process_trigger_options(parser, options, args)
  try:
    tasks = trigger_task_shards(
        options.swarming, task_request, options.shards)
    if tasks:
      print('Triggered task: %s' % task_request.name)
      tasks_sorted = sorted(
          tasks.itervalues(), key=lambda x: x['shard_index'])
      if options.dump_json:
        data = {
          'base_task_name': task_request.name,
          'tasks': tasks,
          'request': task_request_to_raw_request(task_request),
        }
        tools.write_json(unicode(options.dump_json), data, True)
        print('To collect results, use:')
        print('  swarming.py collect -S %s --json %s' %
            (options.swarming, options.dump_json))
      else:
        print('To collect results, use:')
        print('  swarming.py collect -S %s %s' %
            (options.swarming, ' '.join(t['task_id'] for t in tasks_sorted)))
      print('Or visit:')
      for t in tasks_sorted:
        print('  ' + t['view_url'])
    return int(not tasks)
  except Failure:
    on_error.report(None)
    return 1


class OptionParserSwarming(logging_utils.OptionParserWithLogging):
  def __init__(self, **kwargs):
    logging_utils.OptionParserWithLogging.__init__(
        self, prog='swarming.py', **kwargs)
    self.server_group = optparse.OptionGroup(self, 'Server')
    self.server_group.add_option(
        '-S', '--swarming',
        metavar='URL', default=os.environ.get('SWARMING_SERVER', ''),
        help='Swarming server to use')
    self.add_option_group(self.server_group)
    auth.add_auth_options(self)

  def parse_args(self, *args, **kwargs):
    options, args = logging_utils.OptionParserWithLogging.parse_args(
        self, *args, **kwargs)
    auth.process_auth_options(self, options)
    user = self._process_swarming(options)
    if hasattr(options, 'user') and not options.user:
      options.user = user
    return options, args

  def _process_swarming(self, options):
    """Processes the --swarming option and aborts if not specified.

    Returns the identity as determined by the server.
    """
    if not options.swarming:
      self.error('--swarming is required.')
    try:
      options.swarming = net.fix_url(options.swarming)
    except ValueError as e:
      self.error('--swarming %s' % e)
    on_error.report_on_exception_exit(options.swarming)
    try:
      user = auth.ensure_logged_in(options.swarming)
    except ValueError as e:
      self.error(str(e))
    return user


def main(args):
  dispatcher = subcommand.CommandDispatcher(__name__)
  return dispatcher.execute(OptionParserSwarming(version=__version__), args)


if __name__ == '__main__':
  subprocess42.inhibit_os_error_reporting()
  fix_encoding.fix_encoding()
  tools.disable_buffering()
  colorama.init()
  sys.exit(main(sys.argv[1:]))
