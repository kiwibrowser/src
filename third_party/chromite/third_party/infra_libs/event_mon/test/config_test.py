# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import traceback
import unittest

import mock

import infra_libs
from infra_libs import event_mon
from infra_libs.event_mon import config, router
from infra_libs.event_mon.protos import chrome_infra_log_pb2


class ConfigTest(unittest.TestCase):
  def tearDown(self):
    self._close()

  def _set_up_args(self, args=None):  # pragma: no cover
    parser = argparse.ArgumentParser()
    event_mon.add_argparse_options(parser)
    args = parser.parse_args((args or []))
    # Safety net. We really don't want to send something to a real endpoint.
    self.assertTrue((args.event_mon_run_type not in ('test', 'prod'))
                    or args.dry_run)
    event_mon.process_argparse_options(args)
    r = config._router
    self.assertIsInstance(r, router._Router)
    # Check that process_argparse_options is idempotent
    event_mon.process_argparse_options(args)
    self.assertIs(config._router, r)

  def _close(self):
    self.assertTrue(event_mon.close())
    self.assertFalse(config._cache)
    # Test that calling it twice does not raise an exception.
    self.assertTrue(event_mon.close())

  def test_no_args_smoke(self):  # pragma: no cover
    self._set_up_args()

  def test_args_and_default_event(self):  # pragma: no cover
    # The protobuf structure is actually an API not an implementation detail
    # so it's sane to test for changes.
    hostname = 'a'
    service_name = 'b'
    appengine_name = 'c'

    args = ['--event-mon-run-type', 'dry',
            '--event-mon-hostname', hostname,
            '--event-mon-service-name', service_name,
            '--event-mon-appengine-name', appengine_name]
    self._set_up_args(args=args)
    event = event_mon.get_default_event()
    self.assertEquals(event.event_source.host_name, hostname)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.event_source.appengine_name, appengine_name)

  def test_run_type_file_good(self):
    try:
      with infra_libs.temporary_directory(prefix='config_test-') as tempdir:
        filename = os.path.join(tempdir, 'output.db')
        self._set_up_args(args=['--event-mon-run-type', 'file',
                                '--event-mon-output-file', filename])
        self.assertEqual(config._router.output_file, filename)
    except Exception:  # pragma: no cover
      # help for debugging
      traceback.print_exc()
      raise

  # Direct setup_monitoring testing below this line.
  def test_default_event(self):
    # The protobuf structure is actually an API not an implementation detail
    # so it's sane to test for changes.
    event_mon.setup_monitoring()
    event = event_mon.get_default_event()
    self.assertTrue(event.event_source.HasField('host_name'))
    self.assertFalse(event.event_source.HasField('service_name'))
    self.assertFalse(event.event_source.HasField('appengine_name'))

  @mock.patch('os.environ')
  def test_logging_on_appengine(self, environ):
    environ.get.return_value = 'Google App Engine/1.2.3'
    event_mon.setup_monitoring()
    self.assertIsInstance(config._router, router._LoggingStreamRouter)

    environ.get.return_value = 'Development/1.2'
    event_mon.setup_monitoring()
    self.assertIsInstance(config._router, router._LoggingStreamRouter)

  def test_default_event_with_values(self):
    # The protobuf structure is actually an API not an implementation detail
    # so it's sane to test for changes.
    hostname = 'a'
    service_name = 'b'
    appengine_name = 'c'

    event_mon.setup_monitoring(
      hostname=hostname,
      service_name=service_name,
      appengine_name=appengine_name
    )
    event = event_mon.get_default_event()
    self.assertEquals(event.event_source.host_name, hostname)
    self.assertEquals(event.event_source.service_name, service_name)
    self.assertEquals(event.event_source.appengine_name, appengine_name)

  def test_set_default_event(self):
    event_mon.setup_monitoring()
    orig_event = event_mon.get_default_event()

    # Set the new default event to something different from orig_event
    # to make sure it has changed.
    event = chrome_infra_log_pb2.ChromeInfraEvent()
    new_hostname = orig_event.event_source.host_name + '.foo'
    event.event_source.host_name = new_hostname
    event_mon.set_default_event(event)

    new_event = event_mon.get_default_event()
    self.assertEquals(new_event.event_source.host_name, new_hostname)

  def test_set_default_event_bad_type(self):
    event_mon.setup_monitoring()

    # bad type
    with self.assertRaises(TypeError):
      event_mon.set_default_event({'hostname': 'foo'})

  def test_get_default_event(self):
    event_mon.setup_monitoring()
    orig_event = config._cache['default_event']
    self.assertEqual(orig_event, event_mon.get_default_event())
    self.assertIsNot(orig_event, event_mon.get_default_event())

  def test_run_type_invalid(self):
    event_mon.setup_monitoring(run_type='invalid.')
    self.assertNotIsInstance(config._router, router._HttpRouter)

  def test_run_type_test(self):
    event_mon.setup_monitoring(run_type='test')
    self.assertIsInstance(config._router, router._HttpRouter)
