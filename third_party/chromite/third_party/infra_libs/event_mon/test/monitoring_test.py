# -*- encoding:utf-8 -*-
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from infra_libs import event_mon
from infra_libs.event_mon import config, router
from infra_libs.event_mon import monitoring
from infra_libs.event_mon.protos.chrome_infra_log_pb2 import (
  ChromeInfraEvent, ServiceEvent, BuildEvent)
from infra_libs.event_mon.protos.log_request_lite_pb2 import LogRequestLite
from infra_libs.event_mon.protos.goma_stats_pb2 import GomaStats


class ConstantTest(unittest.TestCase):
  def test_constants(self):
    # Make sure constants have not been renamed since they're part of the API.
    self.assertTrue(event_mon.EVENT_TYPES)
    self.assertTrue(event_mon.TIMESTAMP_KINDS)
    self.assertTrue(event_mon.BUILD_EVENT_TYPES)
    self.assertTrue(event_mon.BUILD_RESULTS)


class GetServiceEventTest(unittest.TestCase):

  # We have to setup and tear down event_mon for each test to avoid
  # interactions between tests because event_mon stores a global state.
  def setUp(self):
    event_mon.setup_monitoring(run_type='dry')

  def tearDown(self):
    event_mon.close()

  def test_get_service_event_default(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    log_event = monitoring._get_service_event('START').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the correct thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('service_event'))
    self.assertTrue(event.service_event.HasField('type'))
    self.assertEquals(event.service_event.type, ServiceEvent.START)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.BEGIN)

  def test_get_service_event_correct_versions(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    code_version = [
      {'source_url': 'https://fake.url/thing',
       'revision': '708329c2aeece8aac33af6a5a772ffb14b55903f'},
      {'source_url': 'https://other_fake.url/yet_another_thing',
       'version': 'v2.0'},
      {'source_url': 'https://other_fake2.url/yet_another_thing2',
       'dirty': True},
      ]

    log_event = monitoring._get_service_event(
        'START', code_version=code_version).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    code_version_p = event.service_event.code_version
    self.assertEquals(len(code_version_p), len(code_version))

    self.assertEqual(code_version_p[0].source_url,
                     code_version[0]['source_url'])
    self.assertEqual(code_version_p[0].git_hash, code_version[0]['revision'])

    self.assertEqual(code_version_p[1].source_url,
                     code_version[1]['source_url'])
    self.assertEqual(code_version_p[1].version,
                     code_version[1]['version'])

    self.assertEqual(code_version_p[2].source_url,
                     code_version[2]['source_url'])
    self.assertEqual(code_version_p[2].dirty, True)

  def test_get_service_event_stop(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    log_event = monitoring._get_service_event('STOP').log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.STOP)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.END)

  def test_get_service_event_update(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    log_event = monitoring._get_service_event('UPDATE').log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.UPDATE)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.POINT)

  def test_get_service_event_crash_simple(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    log_event = monitoring._get_service_event('CRASH').log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.CRASH)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.END)

  def test_get_service_event_crash_with_ascii_trace(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    stack_trace = 'A nice ascii string'
    log_event = monitoring._get_service_event(
        'CRASH', stack_trace=stack_trace).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.CRASH)
    self.assertEqual(event.service_event.stack_trace, stack_trace)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.END)

  def test_get_service_event_crash_with_unicode_trace(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    stack_trace = u"Soyez prêt à un étrange goût de Noël."
    log_event = monitoring._get_service_event(
        'CRASH', stack_trace=stack_trace).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.CRASH)
    self.assertEqual(event.service_event.stack_trace, stack_trace)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.END)

  def test_get_service_event_crash_with_big_trace(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    stack_trace = "this is way too long" * 55
    self.assertTrue(len(stack_trace) > monitoring.STACK_TRACE_MAX_SIZE)
    log_event = monitoring._get_service_event(
        'CRASH', stack_trace=stack_trace).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertEqual(event.service_event.type, ServiceEvent.CRASH)
    self.assertEqual(len(event.service_event.stack_trace),
                     monitoring.STACK_TRACE_MAX_SIZE)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.END)

  def test_get_service_event_crash_invalid_trace(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    # This is not a stacktrace
    stack_trace = 123456
    # Should not crash
    log_event = monitoring._get_service_event(
        'CRASH', stack_trace=stack_trace).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)

    # Send only valid data this time
    self.assertEqual(event.service_event.type, ServiceEvent.CRASH)
    self.assertFalse(event.service_event.HasField('stack_trace'))

  def test_get_service_event_trace_without_crash(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    stack_trace = 'A nice ascii string'
    log_event = monitoring._get_service_event(
        'START', stack_trace=stack_trace).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)

    # Make sure we send even invalid data.
    self.assertEqual(event.service_event.type, ServiceEvent.START)
    self.assertEqual(event.service_event.stack_trace, stack_trace)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.BEGIN)

  def test_get_service_event_with_non_default_service_name(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    log_event = monitoring._get_service_event(
        'START', service_name='my.nice.service.name').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('service_event'))
    self.assertTrue(event.service_event.HasField('type'))
    self.assertEquals(event.service_event.type, ServiceEvent.START)
    self.assertEquals(event.event_source.service_name, 'my.nice.service.name')

  def test_get_service_event_with_unicode_service_name(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)
    service_name = u'à_la_française_hé_oui'
    log_event = monitoring._get_service_event(
        'START', service_name=service_name).log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('service_event'))
    self.assertTrue(event.service_event.HasField('type'))
    self.assertEquals(event.service_event.type, ServiceEvent.START)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.timestamp_kind, ChromeInfraEvent.BEGIN)

class SendServiceEventTest(unittest.TestCase):
  def setUp(self):
    event_mon.setup_monitoring(run_type='dry')

  def tearDown(self):
    event_mon.close()

  def test_send_service_event_bad_versions(self):
    # Check that an invalid version does not cause any exception.
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    code_version = [{}, {'revision': 'https://fake.url'}]
    log_event = monitoring._get_service_event(
        'START', code_version=code_version).log_event()
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('service_event'))
    self.assertTrue(event.service_event.HasField('type'))
    self.assertEqual(len(event.service_event.code_version), 0)

  def test_send_service_event_bad_type(self):
    # Check that an invalid type for code_version does not raise
    # any exception.

    code_versions = [None, 123, 'string',
                     [None], [123], ['string'], [['list']]]
    for code_version in code_versions:
      log_event = monitoring._get_service_event(
          'START', code_version=code_version).log_event()
      event = ChromeInfraEvent.FromString(log_event.source_extension)
      self.assertTrue(event.HasField('service_event'))
      self.assertTrue(event.service_event.HasField('type'))
      self.assertEqual(len(event.service_event.code_version), 0)

  def test_send_service_event_smoke(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    self.assertTrue(event_mon.send_service_event('START'))
    self.assertTrue(event_mon.send_service_event('START',
                                                 timestamp_kind=None))
    self.assertTrue(event_mon.send_service_event('START',
                                                  timestamp_kind='BEGIN'))
    self.assertTrue(event_mon.send_service_event('STOP',
                                                  timestamp_kind='END',
                                                  event_timestamp=1234))

  def test_send_service_errors(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    self.assertFalse(event_mon.send_service_event('invalid'))
    self.assertFalse(event_mon.send_service_event('START',
                                                   timestamp_kind='invalid'))
    self.assertFalse(event_mon.send_service_event(
      'START', event_timestamp='2015-01-25'))


class GetBuildEventTest(unittest.TestCase):
  def setUp(self):
    event_mon.setup_monitoring(run_type='dry')

  def tearDown(self):
    event_mon.close()

  def test_get_build_event_default(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    log_event = monitoring.get_build_event(
        'BUILD', hostname, build_name).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)

  def test_get_build_event_with_patch_url(self):
    patch_url = 'http://foo.bar/123#456'
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', patch_url=patch_url).log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.patch_url, patch_url)

  def test_get_build_event_with_bbucket_id(self):
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', bbucket_id=123).log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.bbucket_id, 123)

    # Try invalid value. This should not throw any exceptions.
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', bbucket_id='foo').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

  def test_get_build_event_with_category(self):
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name',
        category='git_cl_try').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(
        event.build_event.category, BuildEvent.CATEGORY_GIT_CL_TRY)

    # Try unknown value. Should produce CATEGORY_UNKNOWN.
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', category='foobar').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(
        event.build_event.category, BuildEvent.CATEGORY_UNKNOWN)

    # Try empty value. Should not set category.
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', category='').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertFalse(event.build_event.HasField('category'))

  def test_get_build_event_with_fail_type(self):
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name',
        fail_type='COMPILE_FAILURE').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(
        event.build_event.fail_type, BuildEvent.FAIL_TYPE_COMPILE)

    # Try unknown value. Should produce CATEGORY_UNKNOWN.
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', fail_type='foobar').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(
        event.build_event.fail_type, BuildEvent.FAIL_TYPE_UNKNOWN)

    # Try empty value. Should not set fail_type.
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name', fail_type='').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertFalse(event.build_event.HasField('fail_type'))

  def test_get_build_event_with_head_revision_git_hash(self):
    test_revision = 'da39a3ee5e6b4b0d3255bfef95601890afd80709'
    log_event = monitoring.get_build_event(
        'BUILD', 'bot.host.name', 'build_name',
        head_revision_git_hash=test_revision).log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(
        event.build_event.head_revision.git_hash, test_revision)

  def test_get_build_event_invalid_type(self):
    # An invalid type is a critical error.
    log_event = monitoring.get_build_event('INVALID_TYPE',
                                           'bot.host.name',
                                           'build_name').log_event()
    self.assertIsNone(log_event)

  def test_get_build_event_invalid_build_name(self):
    # an invalid builder name is not a critical error.
    hostname = 'bot.host.name'
    log_event = monitoring.get_build_event('BUILD', hostname, '').log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)

    self.assertFalse(event.build_event.HasField('build_name'))

  def test_get_build_event_invalid_hostname(self):
    # an invalid hostname is not a critical error.
    builder_name = 'builder_name'
    log_event = monitoring.get_build_event(
        'BUILD', None, builder_name).log_event()
    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.build_name, builder_name)

    self.assertFalse(event.build_event.HasField('host_name'))

  def test_get_build_event_with_build_zero(self):
    # testing 0 is important because bool(0) == False
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 0
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

  def test_get_build_event_with_build_non_zero(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265  # int32
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

  def test_get_build_event_invalid_scheduler(self):
    # Providing a build number on a scheduler event is invalid.
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265  # int32
    log_event = monitoring.get_build_event(
      'SCHEDULER',
      hostname,
      build_name,
      build_number=build_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.SCHEDULER)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)

    self.assertFalse(event.build_event.HasField('build_scheduling_time_ms'))

  def test_get_build_event_invalid_buildname(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265  # int32
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

  def test_get_build_event_missing_build_number(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

    self.assertFalse(event.build_event.HasField('build_number'))

  def test_get_build_event_with_default_build_number(self):
    event_mon.setup_monitoring()
    orig_event = event_mon.get_default_event()
    build_number = 123
    orig_event.build_event.build_number = build_number
    event_mon.set_default_event(orig_event)

    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

    self.assertEquals(event.build_event.build_number, build_number)

  def test_get_build_event_build_number_overwrites_default(self):
    event_mon.setup_monitoring()
    orig_event = event_mon.get_default_event()
    orig_event.build_event.build_number = 123
    event_mon.set_default_event(orig_event)

    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 0  # zero should override the default value
    build_scheduling_time = 123456789
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

  def test_get_build_event_with_step_info_wrong_type(self):
    # BUILD event with step info is invalid.
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_name = 'step_name'
    step_text = 'step_text'
    step_number = 0  # valid step number

    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_name=step_name,
      step_text=step_text,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.step_name, step_name)
    self.assertEquals(event.build_event.step_text, step_text)
    self.assertEquals(event.build_event.step_number, step_number)

  def test_get_build_event_with_step_info(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_name = 'step_name'
    step_number = 0  # valid step number

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_name=step_name,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.STEP)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.step_name, step_name)
    self.assertEquals(event.build_event.step_number, step_number)

  def test_get_build_event_step_name_in_default(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_number = 0  # valid step number
    step_name = 'step_name'

    event_mon.setup_monitoring()
    orig_event = event_mon.get_default_event()
    orig_event.build_event.step_name = step_name
    event_mon.set_default_event(orig_event)

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.STEP)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.step_number, step_number)

    self.assertEquals(event.build_event.step_name, step_name)

  def test_get_build_event_missing_step_name(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_number = 0  # valid step number

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.STEP)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.step_number, step_number)

    self.assertFalse(event.build_event.HasField('step_name'))

  def test_get_build_event_missing_step_text(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_number = 0  # valid step number

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertFalse(event.build_event.HasField('step_text'))

  def test_get_build_event_missing_step_number(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    step_name = 'step_name'

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      step_name=step_name).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.STEP)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.step_name, step_name)

    self.assertFalse(event.build_event.HasField('step_number'))

  def test_get_build_event_step_info_missing_build_info(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    step_name = 'step_name'
    step_number = 0  # valid step number

    log_event = monitoring.get_build_event(
      'STEP',
      hostname,
      build_name,
      step_name=step_name,
      step_number=step_number).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.STEP)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.step_name, step_name)
    self.assertEquals(event.build_event.step_number, step_number)

    self.assertFalse(event.build_event.HasField('build_number'))
    self.assertFalse(event.build_event.HasField('build_scheduling_time_ms'))

  def test_get_build_event_with_invalid_result(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    result = '---INVALID---'

    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      result=result).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)

    self.assertFalse(event.build_event.HasField('result'))

  def test_get_build_event_with_valid_result(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    result = 'SUCCESS'

    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      result=result).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.result, BuildEvent.SUCCESS)

  def test_get_build_event_test_result_mapping(self):
    # Tests the hacky mapping between buildbot results and the proto values.
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789

    # WARNINGS -> WARNING
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      result='WARNINGS').log_event()  # with an S

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.result, BuildEvent.WARNING) # no S

    # EXCEPTION -> INFRA_FAILURE
    log_event = monitoring.get_build_event(
      'BUILD',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      result='EXCEPTION').log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.result, BuildEvent.INFRA_FAILURE)

  def test_get_build_event_valid_result_wrong_type(self):
    # SCHEDULER can't have a result
    hostname = 'bot.host.name'
    build_name = 'build_name'
    build_number = 314159265
    build_scheduling_time = 123456789
    result = 'SUCCESS'

    log_event = monitoring.get_build_event(
      'SCHEDULER',
      hostname,
      build_name,
      build_number=build_number,
      build_scheduling_time=build_scheduling_time,
      result=result).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.SCHEDULER)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.build_event.build_number, build_number)
    self.assertEquals(event.build_event.build_scheduling_time_ms,
                      build_scheduling_time)
    self.assertEquals(event.build_event.result, BuildEvent.SUCCESS)

  def test_get_build_event_with_non_default_service_name(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my.other.nice.service'
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)

  def test_get_build_event_with_unicode_service_name(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = u'à_la_française'
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)

  def test_get_build_event_with_invalid_service_name(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 1234  # invalid
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name).log_event()

    self.assertIsNone(log_event)

  def test_get_build_event_with_extra_result_code_string(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      extra_result_code='result').log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.extra_result_code, ['result'])

  def test_get_build_event_with_extra_result_code_list(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    extra_results = ['result1', 'result2']
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      extra_result_code=extra_results).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.extra_result_code, extra_results)

  def test_get_build_event_with_extra_result_code_invalid_scalar(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    extra_results = 1234
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      extra_result_code=extra_results).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(len(event.build_event.extra_result_code), 0)

  def test_get_build_event_with_extra_result_code_invalid_list(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    extra_results = [1234, 'result']
    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      extra_result_code=extra_results).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.extra_result_code, ['result'])

  def test_get_build_event_with_goma_stats(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    goma_stats = GomaStats()
    goma_stats.request_stats.total = 42

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_stats=goma_stats).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.goma_stats, goma_stats)

  def test_get_build_event_invalid_goma_stats(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_stats='what-is-a-string-doing-here?').log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertFalse(event.build_event.HasField('goma_stats'))

  def test_get_build_event_with_goma_error_unknown(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    goma_error = 'GOMA_ERROR_UNKNOWN'

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_error=goma_error).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.goma_error,
                      BuildEvent.GOMA_ERROR_UNKNOWN)

  def test_get_build_event_with_goma_error_crash(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    goma_error = 'GOMA_ERROR_CRASHED'
    goma_crash_report_id = '0123456789abcdef'

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_error=goma_error, goma_crash_report_id=goma_crash_report_id
    ).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.goma_error,
                      BuildEvent.GOMA_ERROR_CRASHED)
    self.assertEquals(event.build_event.goma_crash_report_id,
                      goma_crash_report_id)

  def test_get_build_event_with_goma_error_non_crash_with_crash_id(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    goma_error = 'GOMA_ERROR_UNKNOWN'
    goma_crash_report_id = '0123456789abcdef'

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_error=goma_error, goma_crash_report_id=goma_crash_report_id
    ).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.build_event.goma_error,
                      BuildEvent.GOMA_ERROR_UNKNOWN)
    self.assertEquals(event.build_event.goma_crash_report_id,
                      goma_crash_report_id)

  def test_get_build_event_with_both_goma_error_and_stats(self):
    hostname = 'bot.host.name'
    build_name = 'build_name'
    service_name = 'my nice service'
    goma_error = 'GOMA_ERROR_UNKNOWN'
    goma_stats = GomaStats()
    goma_stats.request_stats.total = 42

    log_event = monitoring.get_build_event(
      'BUILD', hostname, build_name, service_name=service_name,
      goma_stats=goma_stats, goma_error=goma_error,
    ).log_event()

    self.assertIsInstance(log_event, LogRequestLite.LogEventLite)
    self.assertTrue(log_event.HasField('event_time_ms'))
    self.assertTrue(log_event.HasField('source_extension'))

    # Check that source_extension deserializes to the right thing.
    event = ChromeInfraEvent.FromString(log_event.source_extension)
    self.assertTrue(event.HasField('build_event'))
    self.assertEquals(event.build_event.type, BuildEvent.BUILD)
    self.assertEquals(event.build_event.host_name, hostname)
    self.assertEquals(event.build_event.build_name, build_name)
    self.assertEquals(event.event_source.service_name, service_name)
    # warns but allow to have both.
    self.assertEquals(event.build_event.goma_error,
                      BuildEvent.GOMA_ERROR_UNKNOWN)
    self.assertEquals(event.build_event.goma_stats, goma_stats)


class SendBuildEventTest(unittest.TestCase):
  def setUp(self):
    event_mon.setup_monitoring(run_type='dry')

  def tearDown(self):
    event_mon.close()

  def test_send_build_event_smoke(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    self.assertTrue(event_mon.send_build_event('BUILD',
                                               'bot.host.name',
                                               'build.name'))
    self.assertTrue(event_mon.send_build_event(
      'BUILD',
      'bot.host.name',
      'build_name',
      build_number=1,
      build_scheduling_time=123456789,
      result='FAILURE',
      timestamp_kind='POINT',
      event_timestamp=None))


class SendEventsTest(unittest.TestCase):
  def setUp(self):
    event_mon.setup_monitoring(run_type='dry')

  def tearDown(self):
    event_mon.close()

  def test_send_events_smoke(self):
    self.assertIsInstance(config._router, router._Router)
    self.assertIsInstance(config._cache.get('default_event'), ChromeInfraEvent)

    events = [
      event_mon.get_build_event(
        'BUILD',
        'bot.host.name',
        'build_name',
        build_number=1,
        build_scheduling_time=123456789,
        result='FAILURE',
        timestamp_kind='POINT',
        event_timestamp=None),
      event_mon.get_build_event(
        'BUILD',
        'bot2.host.name',
        'build_name2',
        build_number=1,
        build_scheduling_time=123456789,
        result='FAILURE',
        timestamp_kind='POINT',
        event_timestamp=None),
    ]
    self.assertTrue(monitoring.send_events(events))
