#!/usr/bin/env python
# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import datetime
import json
import logging
import os
import re
import StringIO
import sys
import tempfile
import threading
import time
import traceback
import unittest

# net_utils adjusts sys.path.
import net_utils

from depot_tools import auto_stub

import auth
import isolateserver
import swarming
import test_utils

from depot_tools import fix_encoding
from utils import file_path
from utils import logging_utils
from utils import subprocess42
from utils import tools

import httpserver_mock
import isolateserver_mock


FILE_HASH = u'1' * 40
TEST_NAME = u'unit_tests'


OUTPUT = 'Ran stuff\n'

SHARD_OUTPUT_1 = 'Shard 1 of 3.'
SHARD_OUTPUT_2 = 'Shard 2 of 3.'
SHARD_OUTPUT_3 = 'Shard 3 of 3.'


def gen_yielded_data(index, **kwargs):
  """Returns an entry as it would be yielded by yield_results()."""
  return index, gen_result_response(**kwargs)


def get_results(keys, output_collector=None):
  """Simplifies the call to yield_results().

  The timeout is hard-coded to 10 seconds.
  """
  return list(
      swarming.yield_results(
          'https://host:9001', keys, 10., None, True,
          output_collector, False, True))


def collect(url, task_ids, task_stdout=('console', 'json')):
  """Simplifies the call to swarming.collect()."""
  return swarming.collect(
    swarming=url,
    task_ids=task_ids,
    timeout=10,
    decorate=True,
    print_status_updates=True,
    task_summary_json=None,
    task_output_dir=None,
    task_output_stdout=task_stdout,
    include_perf=False)


def main(args):
  """Bypasses swarming.main()'s exception handling.

  It gets in the way when debugging test failures.
  """
  dispatcher = swarming.subcommand.CommandDispatcher('swarming')
  return dispatcher.execute(swarming.OptionParserSwarming(), args)


def gen_properties(**kwargs):
  out = {
    'caches': [],
    'cipd_input': None,
    'command': None,
    'relative_cwd': None,
    'dimensions': [
      {'key': 'foo', 'value': 'bar'},
      {'key': 'os', 'value': 'Mac'},
    ],
    'env': [],
    'env_prefixes': [],
    'execution_timeout_secs': 60,
    'extra_args': ['--some-arg', '123'],
    'grace_period_secs': 30,
    'idempotent': False,
    'inputs_ref': {
      'isolated': None,
      'isolatedserver': '',
      'namespace': 'default-gzip',
    },
    'io_timeout_secs': 60,
    'outputs': [],
    'secret_bytes': None,
  }
  out.update(kwargs)
  return out


def gen_request_data(properties=None, **kwargs):
  out = {
    'name': 'unit_tests',
    'parent_task_id': '',
    'priority': 101,
    'task_slices': [
      {
        'expiration_secs': 3600,
        'properties': gen_properties(**(properties or {})),
      },
    ],
    'tags': ['tag:a', 'tag:b'],
    'user': 'joe@localhost',
  }
  out.update(kwargs)
  return out


def gen_request_response(request, **kwargs):
  # As seen in services/swarming/handlers_api.py.
  out = {
    'request': request.copy(),
    'task_id': '12300',
  }
  out.update(kwargs)
  return out


def gen_result_response(**kwargs):
  out = {
    u'bot_id': u'swarm6',
    u'completed_ts': u'2014-09-24T13:49:16.012345',
    u'created_ts': u'2014-09-24T13:49:03.012345',
    u'duration': 0.9636809825897217,
    u'exit_code': 0,
    u'failure': False,
    u'internal_failure': False,
    u'modified_ts': u'2014-09-24T13:49:17.012345',
    u'name': u'heartbeat-canary-2014-09-24_13:49:01-os=Ubuntu',
    u'server_versions': [u'1'],
    u'started_ts': u'2014-09-24T13:49:09.012345',
    u'state': 'COMPLETED',
    u'tags': [u'cpu:x86', u'priority:100', u'user:joe@localhost'],
    u'task_id': u'10100',
    u'try_number': 1,
    u'user': u'joe@localhost',
  }
  out.update(kwargs)
  return out


# Silence pylint 'Access to a protected member _Event of a client class'.
class NonBlockingEvent(threading._Event):  # pylint: disable=W0212
  """Just like threading.Event, but a class and ignores timeout in 'wait'.

  Intended to be used as a mock for threading.Event in tests.
  """

  def wait(self, timeout=None):
    return super(NonBlockingEvent, self).wait(0)


class SwarmingServerHandler(httpserver_mock.MockHandler):
  """An extremely minimal implementation of the swarming server API v1.0."""

  def do_GET(self):
    logging.info('S GET %s', self.path)
    if self.path == '/auth/api/v1/server/oauth_config':
      self.send_json({
          'client_id': 'c',
          'client_not_so_secret': 's',
          'primary_url': self.server.url})
    elif self.path == '/auth/api/v1/accounts/self':
      self.send_json({'identity': 'user:joe', 'xsrf_token': 'foo'})
    else:
      m = re.match(r'/api/swarming/v1/task/(\d+)/request', self.path)
      if m:
        logging.info('%s', m.group(1))
        self.send_json(self.server.tasks[int(m.group(1))])
      else:
        self.send_json( {'a': 'b'})
        #raise NotImplementedError(self.path)

  def do_POST(self):
    logging.info('POST %s', self.path)
    raise NotImplementedError(self.path)


class MockSwarmingServer(httpserver_mock.MockServer):
  _HANDLER_CLS = SwarmingServerHandler

  def __init__(self):
    super(MockSwarmingServer, self).__init__()
    self._server.tasks = {}


class Common(object):
  def setUp(self):
    self._tempdir = None
    self.mock(auth, 'ensure_logged_in', lambda _: None)
    self.mock(sys, 'stdout', StringIO.StringIO())
    self.mock(sys, 'stderr', StringIO.StringIO())
    self.mock(logging_utils, 'prepare_logging', lambda *args: None)
    self.mock(logging_utils, 'set_console_level', lambda *args: None)

  def tearDown(self):
    if self._tempdir:
      file_path.rmtree(self._tempdir)
    if not self.has_failed():
      self._check_output('', '')

  @property
  def tempdir(self):
    """Creates the directory on first reference."""
    if not self._tempdir:
      self._tempdir = tempfile.mkdtemp(prefix=u'swarming_test')
    return self._tempdir

  maxDiff = None
  def _check_output(self, out, err):
    self.assertMultiLineEqual(out, sys.stdout.getvalue())
    self.assertMultiLineEqual(err, sys.stderr.getvalue())

    # Flush their content by mocking them again.
    self.mock(sys, 'stdout', StringIO.StringIO())
    self.mock(sys, 'stderr', StringIO.StringIO())

  def main_safe(self, args):
    """Bypasses swarming.main()'s exception handling.

    It gets in the way when debugging test failures.
    """
    # pylint: disable=bare-except
    try:
      return main(args)
    except:
      data = '%s\nSTDOUT:\n%s\nSTDERR:\n%s' % (
          traceback.format_exc(), sys.stdout.getvalue(), sys.stderr.getvalue())
      self.fail(data)


class NetTestCase(net_utils.TestCase, Common):
  """Base class that defines the url_open mock."""
  def setUp(self):
    net_utils.TestCase.setUp(self)
    Common.setUp(self)
    self.mock(time, 'sleep', lambda _: None)
    self.mock(subprocess42, 'call', lambda *_: self.fail())
    self.mock(threading, 'Event', NonBlockingEvent)


class TestIsolated(auto_stub.TestCase, Common):
  """Test functions with isolated_ prefix."""
  def setUp(self):
    auto_stub.TestCase.setUp(self)
    Common.setUp(self)
    self._isolate = isolateserver_mock.MockIsolateServer()
    self._swarming = MockSwarmingServer()

  def tearDown(self):
    try:
      self._isolate.close()
      self._swarming.close()
    finally:
      Common.tearDown(self)
      auto_stub.TestCase.tearDown(self)

  def test_reproduce_isolated(self):
    old_cwd = os.getcwd()
    try:
      os.chdir(self.tempdir)

      def call(cmd, env, cwd):
        # 'out' is the default value for --output-dir.
        outdir = os.path.join(self.tempdir, 'out')
        self.assertTrue(os.path.isdir(outdir))
        self.assertEqual(
            [sys.executable, u'main.py', u'foo', outdir, '--bar'], cmd)
        expected = os.environ.copy()
        expected['SWARMING_TASK_ID'] = 'reproduce'
        expected['SWARMING_BOT_ID'] = 'reproduce'
        self.assertEqual(expected, env)
        self.assertEqual(unicode(os.path.abspath('work')), cwd)
        return 0

      self.mock(subprocess42, 'call', call)

      main_hash = self._isolate.add_content_compressed(
          'default-gzip', 'not executed')
      isolated = {
        'files': {
          'main.py': {
            'h': main_hash,
            's': 12,
            'm': 0700,
          },
        },
        'command': ['python', 'main.py'],
      }
      isolated_hash = self._isolate.add_content_compressed(
          'default-gzip', json.dumps(isolated))
      self._swarming._server.tasks[123] = {
        'properties': {
          'inputs_ref': {
            'isolatedserver': self._isolate.url,
            'namespace': 'default-gzip',
            'isolated': isolated_hash,
          },
          'extra_args': ['foo', '${ISOLATED_OUTDIR}'],
          'secret_bytes': None,
        },
      }
      ret = self.main_safe(
          [
            'reproduce', '--swarming', self._swarming.url, '123', '--',
            '--bar',
          ])
      self._check_output('', '')
      self.assertEqual(0, ret)
    finally:
      os.chdir(old_cwd)


class TestSwarmingTrigger(NetTestCase):
  def test_trigger_task_shards_2_shards(self):
    task_request = swarming.NewTaskRequest(
        name=TEST_NAME,
        parent_task_id=None,
        priority=101,
        task_slices=[
          {
            'expiration_secs': 60*60,
            'properties': swarming.TaskProperties(
                caches=[],
                cipd_input=None,
                command=['a', 'b'],
                relative_cwd=None,
                dimensions=[('foo', 'bar'), ('os', 'Mac')],
                env={},
                env_prefixes=[],
                execution_timeout_secs=60,
                extra_args=[],
                grace_period_secs=30,
                idempotent=False,
                inputs_ref={
                  'isolated': None,
                  'isolatedserver': '',
                  'namespace': 'default-gzip',
                },
                io_timeout_secs=60,
                outputs=[],
                secret_bytes=None),
          },
        ],
        service_account=None,
        tags=['tag:a', 'tag:b'],
        user='joe@localhost')

    request_1 = swarming.task_request_to_raw_request(task_request)
    request_1['name'] = u'unit_tests:0:2'
    request_1['task_slices'][0]['properties']['env'] = [
      {'key': 'GTEST_SHARD_INDEX', 'value': '0'},
      {'key': 'GTEST_TOTAL_SHARDS', 'value': '2'},
    ]
    result_1 = gen_request_response(request_1)

    request_2 = swarming.task_request_to_raw_request(task_request)
    request_2['name'] = u'unit_tests:1:2'
    request_2['task_slices'][0]['properties']['env'] = [
      {'key': 'GTEST_SHARD_INDEX', 'value': '1'},
      {'key': 'GTEST_TOTAL_SHARDS', 'value': '2'},
    ]
    result_2 = gen_request_response(request_2, task_id='12400')
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request_1},
            result_1,
          ),
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request_2},
            result_2,
          ),
        ])

    tasks = swarming.trigger_task_shards(
        swarming='https://localhost:1',
        task_request=task_request,
        shards=2)
    expected = {
      u'unit_tests:0:2': {
        'shard_index': 0,
        'task_id': '12300',
        'view_url': 'https://localhost:1/user/task/12300',
      },
      u'unit_tests:1:2': {
        'shard_index': 1,
        'task_id': '12400',
        'view_url': 'https://localhost:1/user/task/12400',
      },
    }
    self.assertEqual(expected, tasks)

  def test_trigger_task_shards_priority_override(self):
    task_request = swarming.NewTaskRequest(
        name=TEST_NAME,
        parent_task_id='123',
        priority=101,
        task_slices=[
          {
            'expiration_secs': 60*60,
            'properties': swarming.TaskProperties(
                caches=[],
                cipd_input=None,
                command=['a', 'b'],
                relative_cwd=None,
                dimensions=[('foo', 'bar'), ('os', 'Mac')],
                env={},
                env_prefixes=[],
                execution_timeout_secs=60,
                extra_args=[],
                grace_period_secs=30,
                idempotent=False,
                inputs_ref={
                  'isolated': None,
                  'isolatedserver': '',
                  'namespace': 'default-gzip',
                },
                io_timeout_secs=60,
                outputs=[],
                secret_bytes=None),
          },
        ],
        service_account=None,
        tags=['tag:a', 'tag:b'],
        user='joe@localhost')

    request = swarming.task_request_to_raw_request(task_request)
    self.assertEqual('123', request['parent_task_id'])

    result = gen_request_response(request)
    result['request']['priority'] = 200
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])

    os.environ['SWARMING_TASK_ID'] = '123'
    try:
      tasks = swarming.trigger_task_shards(
          swarming='https://localhost:1',
          shards=1,
          task_request=task_request)
    finally:
      os.environ.pop('SWARMING_TASK_ID')
    expected = {
      u'unit_tests': {
        'shard_index': 0,
        'task_id': '12300',
        'view_url': 'https://localhost:1/user/task/12300',
      }
    }
    self.assertEqual(expected, tasks)
    self._check_output('', 'Priority was reset to 200\n')

  def test_trigger_cipd_package(self):
    task_request = swarming.NewTaskRequest(
        name=TEST_NAME,
        parent_task_id='123',
        priority=101,
        task_slices=[
          {
            'expiration_secs': 60*60,
            'properties': swarming.TaskProperties(
                caches=[],
                cipd_input=swarming.CipdInput(
                    client_package=None,
                    packages=[
                        swarming.CipdPackage(
                            package_name='mypackage',
                            path='path/to/package',
                            version='abc123')],
                    server=None),
                command=['a', 'b'],
                relative_cwd=None,
                dimensions=[('foo', 'bar'), ('os', 'Mac')],
                env={},
                env_prefixes=[],
                execution_timeout_secs=60,
                extra_args=[],
                grace_period_secs=30,
                idempotent=False,
                inputs_ref={
                  'isolated': None,
                  'isolatedserver': '',
                  'namespace': 'default-gzip',
                },
                io_timeout_secs=60,
                outputs=[],
                secret_bytes=None),
          },
        ],
        service_account=None,
        tags=['tag:a', 'tag:b'],
        user='joe@localhost')

    request = swarming.task_request_to_raw_request(task_request)
    expected = {
      'client_package': None,
      'packages': [{
          'package_name': 'mypackage',
          'path': 'path/to/package',
          'version': 'abc123',
      }],
      'server': None
    }
    self.assertEqual(
        expected, request['task_slices'][0]['properties']['cipd_input'])

    result = gen_request_response(request)
    result['request']['priority'] = 200
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])

    os.environ['SWARMING_TASK_ID'] = '123'
    try:
      tasks = swarming.trigger_task_shards(
          swarming='https://localhost:1',
          shards=1,
          task_request=task_request)
    finally:
      os.environ.pop('SWARMING_TASK_ID')
    expected = {
      u'unit_tests': {
        'shard_index': 0,
        'task_id': '12300',
        'view_url': 'https://localhost:1/user/task/12300',
      }
    }
    self.assertEqual(expected, tasks)
    self._check_output('', 'Priority was reset to 200\n')


class TestSwarmingCollection(NetTestCase):
  def test_success(self):
    self.expected_requests(
        [
          (
            'https://host:9001/api/swarming/v1/task/10100/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10100/stdout',
            {},
            {'output': OUTPUT},
          ),
        ])
    expected = [gen_yielded_data(0, output=OUTPUT)]
    self.assertEqual(expected, get_results(['10100']))

  def test_failure(self):
    self.expected_requests(
        [
          (
            'https://host:9001/api/swarming/v1/task/10100/result',
            {'retry_50x': False},
            gen_result_response(exit_code=1),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10100/stdout',
            {},
            {'output': OUTPUT},
          ),
        ])
    expected = [gen_yielded_data(0, output=OUTPUT, exit_code=1)]
    self.assertEqual(expected, get_results(['10100']))

  def test_no_ids(self):
    actual = get_results([])
    self.assertEqual([], actual)

  def test_url_errors(self):
    self.mock(logging, 'error', lambda *_, **__: None)
    # NOTE: get_results() hardcodes timeout=10.
    now = {}
    lock = threading.Lock()
    def get_now():
      t = threading.current_thread()
      with lock:
        return now.setdefault(t, range(10)).pop(0)
    self.mock(swarming.net, 'sleep_before_retry', lambda _x, _y: None)
    self.mock(swarming, 'now', get_now)
    # The actual number of requests here depends on 'now' progressing to 10
    # seconds. It's called once per loop. Loop makes 9 iterations.
    self.expected_requests(
        9 * [
          (
            'https://host:9001/api/swarming/v1/task/10100/result',
            {'retry_50x': False},
            None,
          )
        ])
    actual = get_results(['10100'])
    self.assertEqual([], actual)
    self.assertTrue(all(not v for v in now.itervalues()), now)

  def test_many_shards(self):
    self.expected_requests(
        [
          (
            'https://host:9001/api/swarming/v1/task/10100/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10100/stdout',
            {},
            {'output': SHARD_OUTPUT_1},
          ),
          (
            'https://host:9001/api/swarming/v1/task/10200/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10200/stdout',
            {},
            {'output': SHARD_OUTPUT_2},
          ),
          (
            'https://host:9001/api/swarming/v1/task/10300/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10300/stdout',
            {},
            {'output': SHARD_OUTPUT_3},
          ),
        ])
    expected = [
      gen_yielded_data(0, output=SHARD_OUTPUT_1),
      gen_yielded_data(1, output=SHARD_OUTPUT_2),
      gen_yielded_data(2, output=SHARD_OUTPUT_3),
    ]
    actual = get_results(['10100', '10200', '10300'])
    self.assertEqual(expected, sorted(actual))

  def test_output_collector_called(self):
    # Three shards, one failed. All results are passed to output collector.
    self.expected_requests(
        [
          (
            'https://host:9001/api/swarming/v1/task/10100/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10100/stdout',
            {},
            {'output': SHARD_OUTPUT_1},
          ),
          (
            'https://host:9001/api/swarming/v1/task/10200/result',
            {'retry_50x': False},
            gen_result_response(),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10200/stdout',
            {},
            {'output': SHARD_OUTPUT_2},
          ),
          (
            'https://host:9001/api/swarming/v1/task/10300/result',
            {'retry_50x': False},
            gen_result_response(exit_code=1),
          ),
          (
            'https://host:9001/api/swarming/v1/task/10300/stdout',
            {},
            {'output': SHARD_OUTPUT_3},
          ),
        ])

    class FakeOutputCollector(object):
      def __init__(self):
        self.results = []
        self._lock = threading.Lock()

      def process_shard_result(self, index, result):
        with self._lock:
          self.results.append((index, result))

    output_collector = FakeOutputCollector()
    get_results(['10100', '10200', '10300'], output_collector)

    expected = [
      gen_yielded_data(0, output=SHARD_OUTPUT_1),
      gen_yielded_data(1, output=SHARD_OUTPUT_2),
      gen_yielded_data(2, output=SHARD_OUTPUT_3, exit_code=1),
    ]
    self.assertEqual(sorted(expected), sorted(output_collector.results))

  def test_collect_nothing(self):
    self.mock(swarming, 'yield_results', lambda *_: [])
    self.assertEqual(1, collect('https://localhost:1', ['10100', '10200']))
    self._check_output('', 'Results from some shards are missing: 0, 1\n')

  def test_collect_success(self):
    data = gen_result_response(output='Foo')
    self.mock(swarming, 'yield_results', lambda *_: [(0, data)])
    self.assertEqual(0, collect('https://localhost:1', ['10100']))
    expected = u'\n'.join((
      '+------------------------------------------------------+',
      '| Shard 0  https://localhost:1/user/task/10100         |',
      '+------------------------------------------------------+',
      'Foo',
      '+------------------------------------------------------+',
      '| End of shard 0                                       |',
      '|  Pending: 6.0s  Duration: 1.0s  Bot: swarm6  Exit: 0 |',
      '+------------------------------------------------------+',
      'Total duration: 1.0s',
      ''))
    self._check_output(expected, '')

  def test_collect_success_nostdout(self):
    data = gen_result_response(output='Foo')
    self.mock(swarming, 'yield_results', lambda *_: [(0, data)])
    self.assertEqual(0, collect('https://localhost:1', ['10100'], []))
    expected = u'\n'.join((
      '+------------------------------------------------------+',
      '| Shard 0  https://localhost:1/user/task/10100         |',
      '|  Pending: 6.0s  Duration: 1.0s  Bot: swarm6  Exit: 0 |',
      '+------------------------------------------------------+',
      'Total duration: 1.0s',
      ''))
    self._check_output(expected, '')

  def test_collect_fail(self):
    data = gen_result_response(output='Foo', exit_code=-9)
    data['output'] = 'Foo'
    self.mock(swarming, 'yield_results', lambda *_: [(0, data)])
    self.assertEqual(-9, collect('https://localhost:1', ['10100']))
    expected = u'\n'.join((
      '+-------------------------------------------------------+',
      '| Shard 0  https://localhost:1/user/task/10100          |',
      '+-------------------------------------------------------+',
      'Foo',
      '+-------------------------------------------------------+',
      '| End of shard 0                                        |',
      '|  Pending: 6.0s  Duration: 1.0s  Bot: swarm6  Exit: -9 |',
      '+-------------------------------------------------------+',
      'Total duration: 1.0s',
      ''))
    self._check_output(expected, '')

  def test_collect_one_missing(self):
    data = gen_result_response(output='Foo')
    data['output'] = 'Foo'
    self.mock(swarming, 'yield_results', lambda *_: [(0, data)])
    self.assertEqual(1, collect('https://localhost:1', ['10100', '10200']))
    expected = u'\n'.join((
      '+------------------------------------------------------+',
      '| Shard 0  https://localhost:1/user/task/10100         |',
      '+------------------------------------------------------+',
      'Foo',
      '+------------------------------------------------------+',
      '| End of shard 0                                       |',
      '|  Pending: 6.0s  Duration: 1.0s  Bot: swarm6  Exit: 0 |',
      '+------------------------------------------------------+',
      '',
      'Total duration: 1.0s',
      ''))
    self._check_output(expected, 'Results from some shards are missing: 1\n')

  def test_collect_multi(self):
    actual_calls = []
    def fetch_isolated(isolated_hash, storage, cache, outdir, use_symlinks):
      self.assertIs(storage.__class__, isolateserver.Storage)
      self.assertIs(cache.__class__, isolateserver.MemoryCache)
      # Ensure storage is pointing to required location.
      self.assertEqual('https://localhost:2', storage.location)
      self.assertEqual('default', storage.namespace)
      self.assertEqual(False, use_symlinks)
      actual_calls.append((isolated_hash, outdir))
    self.mock(isolateserver, 'fetch_isolated', fetch_isolated)

    collector = swarming.TaskOutputCollector(
        self.tempdir, ['json', 'console'], 2)
    for index in xrange(2):
      collector.process_shard_result(
          index,
          gen_result_response(
              outputs_ref={
                'isolated': str(index) * 40,
                'isolatedserver': 'https://localhost:2',
                'namespace': 'default',
              }))
    summary = collector.finalize()

    expected_calls = [
      ('0'*40, os.path.join(self.tempdir, '0')),
      ('1'*40, os.path.join(self.tempdir, '1')),
    ]
    self.assertEqual(expected_calls, actual_calls)

    # Ensure collected summary is correct.
    outputs_refs = [
      {
        'isolated': '0'*40,
        'isolatedserver': 'https://localhost:2',
        'namespace': 'default',
        'view_url':
            'https://localhost:2/browse?namespace=default&hash=' + '0'*40,
      },
      {
        'isolated': '1'*40,
        'isolatedserver': 'https://localhost:2',
        'namespace': 'default',
        'view_url':
            'https://localhost:2/browse?namespace=default&hash=' + '1'*40,
      },
    ]
    expected = {
      'shards': [gen_result_response(outputs_ref=o) for o in outputs_refs],
    }
    self.assertEqual(expected, summary)

    # Ensure summary dumped to a file is correct as well.
    with open(os.path.join(self.tempdir, 'summary.json'), 'r') as f:
      summary_dump = json.load(f)
    self.assertEqual(expected, summary_dump)

  def test_ensures_same_server(self):
    self.mock(logging, 'error', lambda *_: None)
    # Two shard results, attempt to use different servers.
    actual_calls = []
    self.mock(
        isolateserver, 'fetch_isolated',
        lambda *args: actual_calls.append(args))
    data = [
      gen_result_response(
        outputs_ref={
          'isolatedserver': 'https://server1',
          'namespace': 'namespace',
          'isolated':'hash1',
        }),
      gen_result_response(
        outputs_ref={
          'isolatedserver': 'https://server2',
          'namespace': 'namespace',
          'isolated':'hash1',
        }),
    ]

    # Feed them to collector.
    collector = swarming.TaskOutputCollector(
        self.tempdir, ['json', 'console'], 2)
    for index, result in enumerate(data):
      collector.process_shard_result(index, result)
    collector.finalize()

    # Only first fetch is made, second one is ignored.
    self.assertEqual(1, len(actual_calls))
    isolated_hash, storage, _, outdir, _ = actual_calls[0]
    self.assertEqual(
        ('hash1', os.path.join(self.tempdir, '0')),
        (isolated_hash, outdir))
    self.assertEqual('https://server1', storage.location)


class TestMain(NetTestCase):
  # Tests calling main().
  def test_bot_delete(self):
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/bot/foo/delete',
            {'method': 'POST', 'data': {}},
            {},
          ),
        ])
    ret = self.main_safe(
        ['bot_delete', '--swarming', 'https://localhost:1', 'foo', '--force'])
    self._check_output('', '')
    self.assertEqual(0, ret)

  def test_trigger_raw_cmd(self):
    # Minimalist use.
    request = {
      'name': u'None/foo=bar',
      'parent_task_id': '',
      'priority': 100,
      'task_slices': [
        {
          'expiration_secs': 21600,
          'properties': gen_properties(
              command=['python', '-c', 'print(\'hi\')'],
              dimensions=[{'key': 'foo', 'value': 'bar'}],
              execution_timeout_secs=3600,
              extra_args=None,
              inputs_ref=None,
              io_timeout_secs=1200,
              relative_cwd='deeep'),
        },
      ],
      'tags': [],
      'user': None,
    }
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--dimension', 'foo', 'bar',
        '--raw-cmd',
        '--relative-cwd', 'deeep',
        '--',
        'python',
        '-c',
        'print(\'hi\')',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self._check_output(
        'Triggered task: None/foo=bar\n'
        'To collect results, use:\n'
        '  swarming.py collect -S https://localhost:1 12300\n'
        'Or visit:\n'
        '  https://localhost:1/user/task/12300\n',
        '')

  def test_trigger_raw_cmd_isolated(self):
    # Minimalist use.
    request = {
      'name': u'None/foo=bar/' + FILE_HASH,
      'parent_task_id': '',
      'priority': 100,
      'task_slices': [
        {
          'expiration_secs': 21600,
          'properties': gen_properties(
              command=['python', '-c', 'print(\'hi\')'],
              dimensions=[{'key': 'foo', 'value': 'bar'}],
              execution_timeout_secs=3600,
              extra_args=None,
              inputs_ref={
                'isolated': u'1111111111111111111111111111111111111111',
                'isolatedserver': 'https://localhost:2',
                'namespace': 'default-gzip',
              },
              io_timeout_secs=1200),
        },
      ],
      'tags': [],
      'user': None,
    }
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--dimension', 'foo', 'bar',
        '--raw-cmd',
        '--isolate-server', 'https://localhost:2',
        '--isolated', FILE_HASH,
        '--',
        'python',
        '-c',
        'print(\'hi\')',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self._check_output(
        u'Triggered task: None/foo=bar/' + FILE_HASH + u'\n'
        u'To collect results, use:\n'
        u'  swarming.py collect -S https://localhost:1 12300\n'
        u'Or visit:\n'
        u'  https://localhost:1/user/task/12300\n',
        u'')

  def test_trigger_raw_cmd_with_service_account(self):
    # Minimalist use.
    request = {
      'name': u'None/foo=bar',
      'parent_task_id': '',
      'priority': 100,
      'task_slices': [
        {
          'expiration_secs': 21600,
          'properties': gen_properties(
              command=['python', '-c', 'print(\'hi\')'],
              dimensions=[{'key': 'foo', 'value': 'bar'}],
              execution_timeout_secs=3600,
              extra_args=None,
              inputs_ref=None,
              io_timeout_secs=1200),
        },
      ],
      'service_account': 'bot',
      'tags': [],
      'user': None,
    }
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--dimension', 'foo', 'bar',
        '--service-account', 'bot',
        '--raw-cmd',
        '--',
        'python',
        '-c',
        'print(\'hi\')',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self._check_output(
        'Triggered task: None/foo=bar\n'
        'To collect results, use:\n'
        '  swarming.py collect -S https://localhost:1 12300\n'
        'Or visit:\n'
        '  https://localhost:1/user/task/12300\n',
        '')

  def test_trigger_isolated_hash(self):
    # pylint: disable=unused-argument
    self.mock(swarming, 'now', lambda: 123456)

    request = gen_request_data(
        task_slices=[
          {
            'expiration_secs': 3600,
            'properties': gen_properties(
                inputs_ref={
                  'isolated': u'1111111111111111111111111111111111111111',
                  'isolatedserver': 'https://localhost:2',
                  'namespace': 'default-gzip',
                }),
          },
        ])
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--isolate-server', 'https://localhost:2',
        '--shards', '1',
        '--priority', '101',
        '--dimension', 'foo', 'bar',
        '--dimension', 'os', 'Mac',
        '--expiration', '3600',
        '--user', 'joe@localhost',
        '--tags', 'tag:a',
        '--tags', 'tag:b',
        '--hard-timeout', '60',
        '--io-timeout', '60',
        '--task-name', 'unit_tests',
        '--isolated', FILE_HASH,
        '--',
        '--some-arg',
        '123',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self._check_output(
        'Triggered task: unit_tests\n'
        'To collect results, use:\n'
        '  swarming.py collect -S https://localhost:1 12300\n'
        'Or visit:\n'
        '  https://localhost:1/user/task/12300\n',
        '')

  def test_trigger_isolated_and_json(self):
    # pylint: disable=unused-argument
    write_json_calls = []
    self.mock(tools, 'write_json', lambda *args: write_json_calls.append(args))
    subprocess_calls = []
    self.mock(subprocess42, 'call', lambda *c: subprocess_calls.append(c))
    self.mock(swarming, 'now', lambda: 123456)

    isolated = os.path.join(self.tempdir, 'zaz.isolated')
    content = '{}'
    with open(isolated, 'wb') as f:
      f.write(content)

    isolated_hash = isolateserver_mock.hash_content(content)
    request = gen_request_data(
        task_slices=[
          {
            'expiration_secs': 3600,
            'properties': gen_properties(
                idempotent=True,
                inputs_ref={
                  'isolated': isolated_hash,
                  'isolatedserver': 'https://localhost:2',
                  'namespace': 'default-gzip',
                }),
          },
        ])
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--isolate-server', 'https://localhost:2',
        '--shards', '1',
        '--priority', '101',
        '--dimension', 'foo', 'bar',
        '--dimension', 'os', 'Mac',
        '--expiration', '3600',
        '--user', 'joe@localhost',
        '--tags', 'tag:a',
        '--tags', 'tag:b',
        '--hard-timeout', '60',
        '--io-timeout', '60',
        '--idempotent',
        '--task-name', 'unit_tests',
        '--dump-json', 'foo.json',
        '--isolated', isolated_hash,
        '--',
        '--some-arg',
        '123',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self.assertEqual([], subprocess_calls)
    self._check_output(
        'Triggered task: unit_tests\n'
        'To collect results, use:\n'
        '  swarming.py collect -S https://localhost:1 --json foo.json\n'
        'Or visit:\n'
        '  https://localhost:1/user/task/12300\n',
        '')
    expected = [
      (
        u'foo.json',
        {
          'base_task_name': 'unit_tests',
          'tasks': {
            'unit_tests': {
              'shard_index': 0,
              'task_id': '12300',
              'view_url': 'https://localhost:1/user/task/12300',
            }
          },
          'request': {
            'name': 'unit_tests',
            'parent_task_id': '',
            'priority': 101,
            'task_slices': [
              {
                'expiration_secs': 3600,
                'properties': gen_properties(
                    idempotent=True,
                    inputs_ref={
                      'isolated': isolated_hash,
                      'isolatedserver': 'https://localhost:2',
                      'namespace': 'default-gzip',
                    }),
              },
            ],
            'tags': ['tag:a', 'tag:b'],
            'user': 'joe@localhost',
          },
        },
        True,
      ),
    ]
    self.assertEqual(expected, write_json_calls)

  def test_trigger_cipd(self):
    self.mock(swarming, 'now', lambda: 123456)

    request = gen_request_data(
        task_slices=[
          {
            'expiration_secs': 3600,
            'properties': gen_properties(
                cipd_input={
                  'client_package': None,
                  'packages': [
                    {
                      'package_name': 'super/awesome/pkg',
                      'path': 'path/to/pkg',
                      'version': 'version:42',
                    },
                  ],
                  'server': None,
                },
                inputs_ref={
                  'isolated': u'1111111111111111111111111111111111111111',
                  'isolatedserver': 'https://localhost:2',
                  'namespace': 'default-gzip',
                }),
          },
        ])
    result = gen_request_response(request)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'trigger',
        '--swarming', 'https://localhost:1',
        '--isolate-server', 'https://localhost:2',
        '--shards', '1',
        '--priority', '101',
        '--dimension', 'foo', 'bar',
        '--dimension', 'os', 'Mac',
        '--expiration', '3600',
        '--user', 'joe@localhost',
        '--tags', 'tag:a',
        '--tags', 'tag:b',
        '--hard-timeout', '60',
        '--io-timeout', '60',
        '--task-name', 'unit_tests',
        '--isolated', FILE_HASH,
        '--cipd-package', 'path/to/pkg:super/awesome/pkg:version:42',
        '--',
        '--some-arg',
        '123',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (actual, sys.stderr.getvalue()))
    self._check_output(
        'Triggered task: unit_tests\n'
        'To collect results, use:\n'
        '  swarming.py collect -S https://localhost:1 12300\n'
        'Or visit:\n'
        '  https://localhost:1/user/task/12300\n',
        '')

  def test_trigger_no_request(self):
    with self.assertRaises(SystemExit):
      main([
            'trigger', '--swarming', 'https://host',
            '--isolate-server', 'https://host', '-T', 'foo',
            '-d', 'os', 'amgia',
          ])
    self._check_output(
        '',
        'Usage: swarming.py trigger [options] (hash|isolated) '
          '[-- extra_args|raw command]\n'
        '\n'
        'swarming.py: error: Specify at least one of --raw-cmd or --isolated '
        'or both\n')

  def test_trigger_no_env_vars(self):
    with self.assertRaises(SystemExit):
      main(['trigger'])
    self._check_output(
        '',
        'Usage: swarming.py trigger [options] (hash|isolated) '
          '[-- extra_args|raw command]'
        '\n\n'
        'swarming.py: error: --swarming is required.'
        '\n')

  def test_trigger_no_swarming_env_var(self):
    with self.assertRaises(SystemExit):
      with test_utils.EnvVars({'ISOLATE_SERVER': 'https://host'}):
        main(['trigger', '-T' 'foo', 'foo.isolated'])
    self._check_output(
        '',
        'Usage: swarming.py trigger [options] (hash|isolated) '
          '[-- extra_args|raw command]'
        '\n\n'
        'swarming.py: error: --swarming is required.'
        '\n')

  def test_trigger_no_isolate_server(self):
    with self.assertRaises(SystemExit):
      with test_utils.EnvVars({'SWARMING_SERVER': 'https://host'}):
        main(['trigger', 'foo.isolated', '-d', 'os', 'amiga'])
    self._check_output(
        '',
        'Usage: swarming.py trigger [options] (hash|isolated) '
          '[-- extra_args|raw command]'
        '\n\n'
        'swarming.py: error: Specify at least one of --raw-cmd or --isolated '
          'or both\n')

  def test_trigger_no_dimension(self):
    with self.assertRaises(SystemExit):
      main([
            'trigger', '--swarming', 'https://host', '--raw-cmd', '--', 'foo',
          ])
    self._check_output(
        '',
        'Usage: swarming.py trigger [options] (hash|isolated) '
          '[-- extra_args|raw command]'
        '\n\n'
        'swarming.py: error: Please at least specify one --dimension\n')

  def test_collect_default_json(self):
    j = os.path.join(self.tempdir, 'foo.json')
    data = {
      'base_task_name': 'unit_tests',
      'tasks': {
        'unit_tests': {
          'shard_index': 0,
          'task_id': '12300',
          'view_url': 'https://localhost:1/user/task/12300',
        }
      },
      'request': {
        'name': 'unit_tests',
        'parent_task_id': '',
        'priority': 101,
        'task_slices': [
          {
            'expiration_secs': 3600,
            'properties': gen_properties(
                command=['python', '-c', 'print(\'hi\')'],
                relative_cwd='deeep'),
          },
        ],
        'tags': ['tag:a', 'tag:b'],
        'user': 'joe@localhost',
      },
    }
    with open(j, 'wb') as f:
      json.dump(data, f)
    def stub_collect(
        swarming_server, task_ids, timeout, decorate, print_status_updates,
        task_summary_json, task_output_dir, task_output_stdout, include_perf):
      self.assertEqual('https://host', swarming_server)
      self.assertEqual([u'12300'], task_ids)
      # It is automatically calculated from hard timeout + expiration + 10.
      self.assertEqual(3670., timeout)
      self.assertEqual(True, decorate)
      self.assertEqual(True, print_status_updates)
      self.assertEqual('/a', task_summary_json)
      self.assertEqual('/b', task_output_dir)
      self.assertSetEqual(set(['console', 'json']), set(task_output_stdout))
      self.assertEqual(False, include_perf)
      print('Fake output')
    self.mock(swarming, 'collect', stub_collect)
    self.main_safe(
        ['collect', '--swarming', 'https://host', '--json', j, '--decorate',
          '--print-status-updates', '--task-summary-json', '/a',
          '--task-output-dir', '/b', '--task-output-stdout', 'all'])
    self._check_output('Fake output\n', '')

  def test_post(self):
    out = StringIO.StringIO()
    err = StringIO.StringIO()
    self.mock(sys, 'stdin', StringIO.StringIO('{"a":"b"}'))
    self.mock(sys, 'stdout', out)
    self.mock(sys, 'stderr', err)
    self.expected_requests(
        [
          (
            'http://localhost:1/api/swarming/v1/tasks/new',
            {'data': '{"a":"b"}', 'method': 'POST'},
            '{"yo":"dawg"}',
            {},
          ),
        ])
    ret = self.main_safe(['post', '-S', 'http://localhost:1', 'tasks/new'])
    self.assertEqual(0, ret)
    self.assertEqual('{"yo":"dawg"}', out.getvalue())
    self.assertEqual('', err.getvalue())

  def test_post_fail(self):
    out = StringIO.StringIO()
    err = StringIO.StringIO()
    self.mock(sys, 'stdin', StringIO.StringIO('{"a":"b"}'))
    self.mock(sys, 'stdout', out)
    self.mock(sys, 'stderr', err)
    ret = self.main_safe(['post', '-S', 'http://localhost:1', 'tasks/new'])
    self.assertEqual(1, ret)
    self.assertEqual('', out.getvalue())
    self.assertEqual('No response!\n', err.getvalue())

  def test_query_base(self):
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/bot/botid/tasks?limit=200',
            {},
            {'yo': 'dawg'},
          ),
        ])
    ret = self.main_safe(
        [
          'query', '--swarming', 'https://localhost:1', 'bot/botid/tasks',
        ])
    self._check_output('{\n  "yo": "dawg"\n}\n', '')
    self.assertEqual(0, ret)

  def test_query_cursor(self):
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/bot/botid/tasks?'
                'foo=bar&limit=2',
            {},
            {
              'cursor': '%',
              'extra': False,
              'items': ['A'],
            },
          ),
          (
            'https://localhost:1/api/swarming/v1/bot/botid/tasks?'
                'foo=bar&cursor=%25&limit=1',
            {},
            {
              'cursor': None,
              'items': ['B'],
              'ignored': True,
            },
          ),
        ])
    ret = self.main_safe(
        [
          'query', '--swarming', 'https://localhost:1',
          'bot/botid/tasks?foo=bar',
          '--limit', '2',
        ])
    expected = (
        '{\n'
        '  "extra": false, \n'
        '  "items": [\n'
        '    "A", \n'
        '    "B"\n'
        '  ]\n'
        '}\n')
    self._check_output(expected, '')
    self.assertEqual(0, ret)

  def test_reproduce(self):
    old_cwd = os.getcwd()
    try:
      os.chdir(self.tempdir)

      def call(cmd, env, cwd):
        w = os.path.abspath('work')
        self.assertEqual([os.path.join(w, 'foo'), '--bar'], cmd)
        expected = os.environ.copy()
        expected['aa'] = 'bb'
        expected['PATH'] = os.pathsep.join(
            (os.path.join(w, 'foo', 'bar'), os.path.join(w, 'second'),
              expected['PATH']))
        expected['SWARMING_TASK_ID'] = 'reproduce'
        expected['SWARMING_BOT_ID'] = 'reproduce'
        self.assertEqual(expected, env)
        self.assertEqual(unicode(w), cwd)
        return 0

      self.mock(subprocess42, 'call', call)

      self.expected_requests(
          [
            (
              'https://localhost:1/api/swarming/v1/task/123/request',
              {},
              {
                'properties': {
                  'command': ['foo'],
                  'env': [
                    {'key': 'aa', 'value': 'bb'},
                  ],
                  'env_prefixes': [
                    {'key': 'PATH', 'value': ['foo/bar', 'second']},
                  ],
                  'secret_bytes': None,
                },
              },
            ),
          ])
      ret = self.main_safe(
          [
            'reproduce', '--swarming', 'https://localhost:1', '123', '--',
            '--bar',
          ])
      self._check_output('', '')
      self.assertEqual(0, ret)
    finally:
      os.chdir(old_cwd)

  def test_run(self):
    request = {
      'name': u'None/foo=bar',
      'parent_task_id': '',
      'priority': 100,
      'task_slices': [
        {
          'expiration_secs': 21600,
          'properties': gen_properties(
              command=['python', '-c', 'print(\'hi\')'],
              dimensions=[{'key': 'foo', 'value': 'bar'}],
              execution_timeout_secs=3600,
              extra_args=None,
              inputs_ref=None,
              io_timeout_secs=1200,
              relative_cwd='deeep'),
        },
      ],
      'tags': [],
      'user': None,
    }
    result = gen_request_response(request)

    def stub_collect(
        swarming_server, task_ids, timeout, decorate, print_status_updates,
        task_summary_json, task_output_dir, task_output_stdout, include_perf):
      self.assertEqual('https://localhost:1', swarming_server)
      self.assertEqual([u'12300'], task_ids)
      # It is automatically calculated from hard timeout + expiration + 10.
      self.assertEqual(25210., timeout)
      self.assertEqual(None, decorate)
      self.assertEqual(None, print_status_updates)
      self.assertEqual(None, task_summary_json)
      self.assertEqual(None, task_output_dir)
      self.assertSetEqual(set(['console', 'json']), set(task_output_stdout))
      self.assertEqual(False, include_perf)
      print('Fake output')
      return 0
    self.mock(swarming, 'collect', stub_collect)
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/tasks/new',
            {'data': request},
            result,
          ),
        ])
    ret = self.main_safe([
        'run',
        '--swarming', 'https://localhost:1',
        '--dimension', 'foo', 'bar',
        '--raw-cmd',
        '--relative-cwd', 'deeep',
        '--',
        'python',
        '-c',
        'print(\'hi\')',
      ])
    actual = sys.stdout.getvalue()
    self.assertEqual(0, ret, (ret, actual, sys.stderr.getvalue()))
    self._check_output(
        u'Triggered task: None/foo=bar\nFake output\n', '')

  def test_cancel(self):
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/task/10100/cancel',
            {'data': {'kill_running': False}, 'method': 'POST'},
            {'yo': 'dawg'},
          ),
        ])
    ret = self.main_safe(
        [
          'cancel', '--swarming', 'https://localhost:1', '10100',
        ])
    self._check_output('', '')
    self.assertEqual(0, ret)

  def test_collect_timeout_zero(self):
    j = os.path.join(self.tempdir, 'foo.json')
    pending = gen_result_response(state='PENDING')
    self.expected_requests(
        [
          (
            'https://localhost:1/api/swarming/v1/task/10100/result',
            {'retry_50x': True},
            pending,
          ),
        ])
    self.main_safe(
        [
          'collect', '--swarming', 'https://localhost:1',
          '--task-summary-json', j, '--timeout', '-1', '10100',
        ])
    self._check_output('swarm6: 10100 0\n', '')
    with open(j, 'r') as f:
      actual = json.load(f)
    self.assertEqual({u'shards': [pending]}, actual)


class TestCommandBot(NetTestCase):
  # Specialized test fixture for command 'bot'.
  def setUp(self):
    super(TestCommandBot, self).setUp()
    # Sample data retrieved from actual server.
    self.now = unicode(datetime.datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S'))
    self.bot_1 = {
      u'bot_id': u'swarm1',
      u'created_ts': self.now,
      u'dimensions': [
        {u'key': u'cores', u'value': [u'8']},
        {u'key': u'cpu', u'value': [u'x86', u'x86-64']},
        {u'key': u'gpu', u'value': []},
        {u'key': u'id', u'value': [u'swarm1']},
        {u'key': u'os', u'value': [u'Ubuntu', u'Ubuntu-12.04']},
      ],
      u'external_ip': u'1.1.1.1',
      u'hostname': u'swarm1.example.com',
      u'internal_ip': u'192.168.0.1',
      u'is_dead': True,
      u'last_seen_ts': 'A long time ago',
      u'quarantined': False,
      u'task_id': u'',
      u'task_name': None,
      u'version': u'56918a2ea28a6f51751ad14cc086f118b8727905',
    }
    self.bot_2 = {
      u'bot_id': u'swarm2',
      u'created_ts': self.now,
      u'dimensions': [
        {u'key': u'cores', u'value': [u'8']},
        {u'key': u'cpu', u'value': [u'x86', u'x86-64']},
        {u'key': u'gpu', u'value': [
          u'15ad',
          u'15ad:0405',
          u'VMware Virtual SVGA 3D Graphics Adapter',
        ]},
        {u'key': u'id', u'value': [u'swarm2']},
        {u'key': u'os', u'value': [u'Windows', u'Windows-6.1']},
      ],
      u'external_ip': u'1.1.1.2',
      u'hostname': u'swarm2.example.com',
      u'internal_ip': u'192.168.0.2',
      u'is_dead': False,
      u'last_seen_ts': self.now,
      u'quarantined': False,
      u'task_id': u'',
      u'task_name': None,
      u'version': u'56918a2ea28a6f51751ad14cc086f118b8727905',
    }
    self.bot_3 = {
      u'bot_id': u'swarm3',
      u'created_ts': self.now,
      u'dimensions': [
        {u'key': u'cores', u'value': [u'4']},
        {u'key': u'cpu', u'value': [u'x86', u'x86-64']},
        {u'key': u'gpu', u'value': [u'15ad', u'15ad:0405']},
        {u'key': u'id', u'value': [u'swarm3']},
        {u'key': u'os', u'value': [u'Mac', u'Mac-10.9']},
      ],
      u'external_ip': u'1.1.1.3',
      u'hostname': u'swarm3.example.com',
      u'internal_ip': u'192.168.0.3',
      u'is_dead': False,
      u'last_seen_ts': self.now,
      u'quarantined': False,
      u'task_id': u'148569b73a89501',
      u'task_name': u'browser_tests',
      u'version': u'56918a2ea28a6f51751ad14cc086f118b8727905',
    }
    self.bot_4 = {
      u'bot_id': u'swarm4',
      u'created_ts': self.now,
      u'dimensions': [
        {u'key': u'cores', u'value': [u'8']},
        {u'key': u'cpu', u'value': [u'x86', u'x86-64']},
        {u'key': u'gpu', u'value': []},
        {u'key': u'id', u'value': [u'swarm4']},
        {u'key': u'os', u'value': [u'Ubuntu', u'Ubuntu-12.04']},
      ],
      u'external_ip': u'1.1.1.4',
      u'hostname': u'swarm4.example.com',
      u'internal_ip': u'192.168.0.4',
      u'is_dead': False,
      u'last_seen_ts': self.now,
      u'quarantined': False,
      u'task_id': u'14856971a64c601',
      u'task_name': u'base_unittests',
      u'version': u'56918a2ea28a6f51751ad14cc086f118b8727905',
    }

  def mock_swarming_api(self, bots, cursor):
    """Returns fake /api/swarming/v1/bots/list data."""
    # Sample data retrieved from actual server.
    return {
      u'items': bots,
      u'cursor': cursor,
      u'death_timeout': 1800.0,
      u'limit': 4,
      u'now': unicode(self.now),
    }

  def test_bots(self):
    base_url = 'https://localhost:1/api/swarming/v1/bots/list?'
    self.expected_requests(
        [
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE',
            {},
            self.mock_swarming_api([self.bot_2], 'opaque'),
          ),
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE&cursor=opaque',
            {},
            self.mock_swarming_api([self.bot_3], 'opaque2'),
          ),
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE&cursor=opaque2',
            {},
            self.mock_swarming_api([self.bot_4], None),
          ),
        ])
    ret = self.main_safe(['bots', '--swarming', 'https://localhost:1'])
    expected = (
        u'swarm2\n'
        u'  {"cores": ["8"], "cpu": ["x86", "x86-64"], "gpu": '
          '["15ad", "15ad:0405", "VMware Virtual SVGA 3D Graphics Adapter"], '
          '"id": ["swarm2"], "os": ["Windows", "Windows-6.1"]}\n'
        'swarm3\n'
        '  {"cores": ["4"], "cpu": ["x86", "x86-64"], "gpu": ["15ad", '
          '"15ad:0405"], "id": ["swarm3"], "os": ["Mac", "Mac-10.9"]}\n'
        u'  task: 148569b73a89501\n'
        u'swarm4\n'
        u'  {"cores": ["8"], "cpu": ["x86", "x86-64"], "gpu": [], '
          '"id": ["swarm4"], "os": ["Ubuntu", "Ubuntu-12.04"]}\n'
        u'  task: 14856971a64c601\n')
    self._check_output(expected, '')
    self.assertEqual(0, ret)

  def test_bots_bare(self):
    base_url = 'https://localhost:1/api/swarming/v1/bots/list?'
    self.expected_requests(
        [
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE',
            {},
            self.mock_swarming_api([self.bot_2], 'opaque'),
          ),
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE&cursor=opaque',
            {},
            self.mock_swarming_api([self.bot_3], 'opaque2'),
          ),
          (
            base_url + 'is_dead=FALSE&is_busy=NONE&is_mp=NONE&cursor=opaque2',
            {},
            self.mock_swarming_api([self.bot_4], None),
          ),
        ])
    ret = self.main_safe(
        ['bots', '--swarming', 'https://localhost:1', '--bare'])
    self._check_output("swarm2\nswarm3\nswarm4\n", '')
    self.assertEqual(0, ret)

  def test_bots_filter(self):
    base_url = 'https://localhost:1/api/swarming/v1/bots/list?'
    self.expected_requests(
        [
          (
            base_url +
              'is_dead=FALSE&is_busy=TRUE&is_mp=NONE&dimensions=os%3AWindows',
            {},
            self.mock_swarming_api([self.bot_2], None),
          ),
        ])
    ret = self.main_safe(
        [
          'bots', '--swarming', 'https://localhost:1',
          '--busy',
          '--dimension', 'os', 'Windows',
        ])
    expected = (
        u'swarm2\n  {"cores": ["8"], "cpu": ["x86", "x86-64"], '
          '"gpu": ["15ad", "15ad:0405", "VMware Virtual SVGA 3D Graphics '
          'Adapter"], "id": ["swarm2"], '
          '"os": ["Windows", "Windows-6.1"]}\n')
    self._check_output(expected, '')
    self.assertEqual(0, ret)

  def test_bots_filter_keep_dead(self):
    base_url = 'https://localhost:1/api/swarming/v1/bots/list?'
    self.expected_requests(
        [
          (
            base_url + 'is_dead=NONE&is_busy=NONE&is_mp=NONE',
            {},
            self.mock_swarming_api([self.bot_1, self.bot_4], None),
          ),
        ])
    ret = self.main_safe(
        [
          'bots', '--swarming', 'https://localhost:1',
          '--keep-dead',
        ])
    expected = (
        u'swarm1\n  {"cores": ["8"], "cpu": ["x86", "x86-64"], "gpu": [], '
          '"id": ["swarm1"], "os": ["Ubuntu", "Ubuntu-12.04"]}\n'
        u'swarm4\n'
        u'  {"cores": ["8"], "cpu": ["x86", "x86-64"], "gpu": [], '
          '"id": ["swarm4"], "os": ["Ubuntu", "Ubuntu-12.04"]}\n'
        u'  task: 14856971a64c601\n')
    self._check_output(expected, '')
    self.assertEqual(0, ret)

  def test_bots_filter_dead_only(self):
    base_url = 'https://localhost:1/api/swarming/v1/bots/list?'
    self.expected_requests(
        [
          (
            base_url +
              'is_dead=TRUE&is_busy=NONE&is_mp=NONE&dimensions=os%3AUbuntu',
            {},
            self.mock_swarming_api([self.bot_1], None),
          ),
        ])
    ret = self.main_safe(
        [
          'bots', '--swarming', 'https://localhost:1',
          '--dimension', 'os', 'Ubuntu', '--dead-only',
        ])
    expected = (
        u'swarm1\n  {"cores": ["8"], "cpu": ["x86", "x86-64"], "gpu": [], '
          '"id": ["swarm1"], "os": ["Ubuntu", "Ubuntu-12.04"]}\n')
    self._check_output(expected, '')
    self.assertEqual(0, ret)


if __name__ == '__main__':
  fix_encoding.fix_encoding()
  logging.basicConfig(
      level=logging.DEBUG if '-v' in sys.argv else logging.CRITICAL)
  if '-v' in sys.argv:
    unittest.TestCase.maxDiff = None
  for e in ('ISOLATE_SERVER', 'SWARMING_TASK_ID', 'SWARMING_SERVER'):
    os.environ.pop(e, None)
  unittest.main()
