# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import unittest

import mock

from infra_libs import app
from infra_libs import logs
from infra_libs import ts_mon


class MockApplication(app.BaseApplication):
  main = mock.Mock()


class AppTest(unittest.TestCase):
  def setUp(self):
    self.mock_logs_process_argparse_options = mock.patch(
        'infra_libs.logs.process_argparse_options').start()
    self.mock_ts_mon_process_argparse_options = mock.patch(
        'infra_libs.ts_mon.process_argparse_options').start()

  def tearDown(self):
    mock.patch.stopall()

  def test_initialises_standard_modules(self):
    with self.assertRaises(SystemExit):
      MockApplication().run(['appname'])

    self.mock_logs_process_argparse_options.assert_called_once()
    self.mock_ts_mon_process_argparse_options.assert_called_once()

  def test_initialises_standard_modules_except_ts_mon(self):
    class Application(MockApplication):
      USES_TS_MON = False

    with self.assertRaises(SystemExit):
      Application().run(['appname'])

    self.mock_logs_process_argparse_options.assert_called_once()
    self.assertFalse(self.mock_ts_mon_process_argparse_options.called)

  def test_initialises_standard_modules_except_logs(self):
    class Application(MockApplication):
      USES_STANDARD_LOGGING = False

    with self.assertRaises(SystemExit):
      Application().run(['appname'])

    self.assertFalse(self.mock_logs_process_argparse_options.called)
    self.mock_ts_mon_process_argparse_options.assert_called_once()

  def test_argparse_options(self):
    test_case = self
    class App(MockApplication):
      def add_argparse_options(self, p):
        test_case.assertIsInstance(p, argparse.ArgumentParser)

      def process_argparse_options(self, o):
        test_case.assertIsInstance(o, argparse.Namespace)

    a = App()
    with self.assertRaises(SystemExit):
      a.run(['appname'])

  def test_exit_status(self):
    class App(app.BaseApplication):
      def main(self, opts):
        return 42

    with self.assertRaises(SystemExit) as cm:
      App().run(['appname'])
    self.assertEqual(42, cm.exception.code)

  def test_catches_exceptions(self):
    a = MockApplication()
    a.main.side_effect = Exception
    ts_mon.reset_for_unittest(disable=True)

    with self.assertRaises(SystemExit) as cm:
      a.run(['appname'])
    self.assertEqual(1, cm.exception.code)

  def test_catches_exceptions_with_ts_mon_disabled(self):
    class Application(MockApplication):
      USES_TS_MON = False

    a = Application()
    a.main.side_effect = Exception
    ts_mon.reset_for_unittest(disable=True)

    with self.assertRaises(SystemExit) as cm:
      a.run(['appname'])
    self.assertEqual(1, cm.exception.code)
