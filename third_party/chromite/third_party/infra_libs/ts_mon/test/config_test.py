# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import requests
import tempfile
import unittest

import mock

from testing_support import auto_stub

from infra_libs.ts_mon import config
from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import standard_metrics
from infra_libs.ts_mon.common import monitors
from infra_libs.ts_mon.common import targets
import infra_libs


DATA_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data')


class GlobalsTest(auto_stub.TestCase):

  def setUp(self):
    super(GlobalsTest, self).setUp()
    interface.state = interface.State()
    self.mock(config, 'load_machine_config', lambda x: {})

  def tearDown(self):
    # It's important to call close() before un-setting the mock state object,
    # because any FlushThread started by the test is stored in that mock state
    # and needs to be stopped before running any other tests.
    interface.close()
    super(GlobalsTest, self).tearDown()

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  @mock.patch('infra_libs.ts_mon.common.monitors.CredentialFactory.'
              'from_string')
  def test_https_monitor_args(self, _load_creds, fake_fqdn, fake_get):
    print [_load_creds, fake_fqdn, fake_get]
    fake_fqdn.return_value = 'slave1-a1.reg.tld'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-credentials', '/path/to/creds.p8.json',
        '--ts-mon-endpoint', 'https://test/random:insert'])

    config.process_argparse_options(args)

    self.assertIsInstance(interface.state.global_monitor,
                          monitors.HttpsMonitor)

    self.assertIsInstance(interface.state.target, targets.DeviceTarget)
    self.assertEquals(interface.state.target.hostname, 'slave1-a1')
    self.assertEquals(interface.state.target.region, 'reg')
    self.assertEquals(args.ts_mon_flush, 'auto')
    self.assertIsNotNone(interface.state.flush_thread)
    self.assertTrue(standard_metrics.up.get())

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  def test_default_target_uppercase_fqdn(self, fake_fqdn, fake_get):
    fake_fqdn.return_value = 'SLAVE1-A1.REG.TLD'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-credentials', '/path/to/creds.p8.json',
        '--ts-mon-endpoint', 'unsupported://www.googleapis.com/some/api'])
    config.process_argparse_options(args)
    self.assertIsInstance(interface.state.target, targets.DeviceTarget)
    self.assertEquals(interface.state.target.hostname, 'slave1-a1')
    self.assertEquals(interface.state.target.region, 'reg')

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  def test_default_target_fqdn_without_domain(self, fake_fqdn, fake_get):
    fake_fqdn.return_value = 'SLAVE1-A1'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-credentials', '/path/to/creds.p8.json',
        '--ts-mon-endpoint', 'unsupported://www.googleapis.com/some/api'])
    config.process_argparse_options(args)
    self.assertIsInstance(interface.state.target, targets.DeviceTarget)
    self.assertEquals(interface.state.target.hostname, 'slave1-a1')
    self.assertEquals(interface.state.target.region, '')

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  def test_fallback_monitor_args(self, fake_fqdn, fake_get):
    fake_fqdn.return_value = 'foo'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-credentials', '/path/to/creds.p8.json',
        '--ts-mon-endpoint', 'unsupported://www.googleapis.com/some/api'])
    config.process_argparse_options(args)

    self.assertIsInstance(interface.state.global_monitor,
                          monitors.NullMonitor)

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  def test_explicit_disable_args(self, fake_fqdn, fake_get):
    fake_fqdn.return_value = 'foo'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-credentials', '/path/to/creds.p8.json',
        '--ts-mon-endpoint', 'none'])
    config.process_argparse_options(args)

    self.assertIsInstance(interface.state.global_monitor,
                          monitors.NullMonitor)

  @mock.patch('requests.get', autospec=True)
  @mock.patch('socket.getfqdn', autospec=True)
  def test_manual_flush(self, fake_fqdn, fake_get):
    fake_fqdn.return_value = 'foo'
    fake_get.return_value.side_effect = requests.exceptions.ConnectionError
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-flush', 'manual'])

    config.process_argparse_options(args)
    self.assertIsNone(interface.state.flush_thread)

  @mock.patch('infra_libs.ts_mon.common.monitors.DebugMonitor', auto_spec=True)
  def test_dryrun_args(self, fake_monitor):
    singleton = mock.Mock()
    fake_monitor.return_value = singleton
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-endpoint', 'file://foo.txt'])
    config.process_argparse_options(args)
    fake_monitor.assert_called_once_with('foo.txt')
    self.assertIs(interface.state.global_monitor, singleton)

  def test_device_args(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'device',
                         '--ts-mon-device-region', 'reg',
                         '--ts-mon-device-role', 'role',
                         '--ts-mon-device-network', 'net',
                         '--ts-mon-device-hostname', 'host'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.role, 'role')
    self.assertEqual(interface.state.target.network, 'net')
    self.assertEqual(interface.state.target.hostname, 'host')

  def test_autogen_device_args(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'device',
                         '--ts-mon-device-region', 'reg',
                         '--ts-mon-device-role', 'role',
                         '--ts-mon-device-network', 'net',
                         '--ts-mon-device-hostname', 'host',
                         '--ts-mon-autogen-hostname'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.role, 'role')
    self.assertEqual(interface.state.target.network, 'net')
    self.assertEqual(interface.state.target.hostname, 'autogen:host')

  def test_autogen_device_config(self):
    self.mock(config, 'load_machine_config', lambda x: {
        'autogen_hostname': True,
        'credentials': '/path/to/creds.p8.json',
        'endpoint': 'test://endpoint'})
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([
        '--ts-mon-target-type', 'device',
        '--ts-mon-device-region', 'reg',
        '--ts-mon-device-role', 'role',
        '--ts-mon-device-network', 'net',
        '--ts-mon-device-hostname', 'host'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.role, 'role')
    self.assertEqual(interface.state.target.network, 'net')
    self.assertEqual(interface.state.target.hostname, 'autogen:host')

  def test_task_args(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'task',
                         '--ts-mon-task-service-name', 'serv',
                         '--ts-mon-task-job-name', 'job',
                         '--ts-mon-task-region', 'reg',
                         '--ts-mon-task-hostname', 'host',
                         '--ts-mon-task-number', '1'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.service_name, 'serv')
    self.assertEqual(interface.state.target.job_name, 'job')
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.hostname, 'host')
    self.assertEqual(interface.state.target.task_num, 1)

  def test_autogen_task_args(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'task',
                         '--ts-mon-task-service-name', 'serv',
                         '--ts-mon-task-job-name', 'job',
                         '--ts-mon-task-region', 'reg',
                         '--ts-mon-task-hostname', 'host',
                         '--ts-mon-task-number', '1',
                         '--ts-mon-autogen-hostname'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.service_name, 'serv')
    self.assertEqual(interface.state.target.job_name, 'job')
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.hostname, 'autogen:host')
    self.assertEqual(interface.state.target.task_num, 1)

  def test_autogen_task_config(self):
    self.mock(config, 'load_machine_config', lambda x: {
        'autogen_hostname': True,
        'credentials': '/path/to/creds.p8.json',
        'endpoint': 'test://endpoint'})
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-target-type', 'task',
                         '--ts-mon-task-service-name', 'serv',
                         '--ts-mon-task-job-name', 'job',
                         '--ts-mon-task-region', 'reg',
                         '--ts-mon-task-hostname', 'host',
                         '--ts-mon-task-number', '1'])
    config.process_argparse_options(args)
    self.assertEqual(interface.state.target.service_name, 'serv')
    self.assertEqual(interface.state.target.job_name, 'job')
    self.assertEqual(interface.state.target.region, 'reg')
    self.assertEqual(interface.state.target.hostname, 'autogen:host')
    self.assertEqual(interface.state.target.task_num, 1)

  def test_task_args_missing_service_name(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'task',
                         '--ts-mon-task-job-name', 'job',
                         '--ts-mon-task-region', 'reg',
                         '--ts-mon-task-hostname', 'host',
                         '--ts-mon-task-number', '1'])
    with self.assertRaises(SystemExit):
      config.process_argparse_options(args)

  def test_task_args_missing_job_name(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-credentials', '/path/to/creds.p8.json',
                         '--ts-mon-target-type', 'task',
                         '--ts-mon-task-service-name', 'serv',
                         '--ts-mon-task-region', 'reg',
                         '--ts-mon-task-hostname', 'host',
                         '--ts-mon-task-number', '1'])
    with self.assertRaises(SystemExit):
      config.process_argparse_options(args)

  def test_metric_name_prefix(self):
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args(['--ts-mon-metric-name-prefix', '/test/random/'])
    config.process_argparse_options(args)
    self.assertEqual('/test/random/', interface.state.metric_name_prefix)

  @mock.patch('infra_libs.ts_mon.common.monitors.NullMonitor', autospec=True)
  def test_no_args(self, fake_monitor):
    singleton = mock.Mock()
    fake_monitor.return_value = singleton
    p = argparse.ArgumentParser()
    config.add_argparse_options(p)
    args = p.parse_args([])
    config.process_argparse_options(args)
    self.assertEqual(1, len(fake_monitor.mock_calls))
    self.assertEqual('/chrome/infra/', interface.state.metric_name_prefix)
    self.assertIs(interface.state.global_monitor, singleton)

  @mock.patch('requests.get', autospec=True)
  def test_gce_region(self, mock_get):
    r = mock_get.return_value
    r.status_code = 200
    r.text = 'projects/182615506979/zones/us-central1-f'

    self.assertEquals('us-central1-f', config._default_region('foo.golo'))

  @mock.patch('requests.get', autospec=True)
  def test_gce_region_timeout(self, mock_get):
    mock_get.side_effect = requests.exceptions.Timeout
    self.assertEquals('golo', config._default_region('foo.golo'))

  @mock.patch('requests.get', autospec=True)
  def test_gce_region_404(self, mock_get):
    r = mock_get.return_value
    r.status_code = 404

    self.assertEquals('golo', config._default_region('foo.golo'))


class ConfigTest(unittest.TestCase):

  def test_load_machine_config(self):
    with infra_libs.temporary_directory() as temp_dir:
      filename = os.path.join(temp_dir, 'config')
      with open(filename, 'w') as fh:
        json.dump({'foo': 'bar'}, fh)

      self.assertEquals({'foo': 'bar'}, config.load_machine_config(filename))

  def test_load_machine_config_bad(self):
    with infra_libs.temporary_directory() as temp_dir:
      filename = os.path.join(temp_dir, 'config')
      with open(filename, 'w') as fh:
        fh.write('not a json file')

      with self.assertRaises(ValueError):
        config.load_machine_config(filename)

  def test_load_machine_config_not_exists(self):
    self.assertEquals({}, config.load_machine_config('does not exist'))
