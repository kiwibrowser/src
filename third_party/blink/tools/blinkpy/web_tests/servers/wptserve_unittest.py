# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from blinkpy.common.system.log_testing import LoggingTestCase
from blinkpy.common.host_mock import MockHost
from blinkpy.web_tests.port import test
from blinkpy.web_tests.servers.wptserve import WPTServe


class TestWPTServe(LoggingTestCase):

    # pylint: disable=protected-access

    def test_init_start_cmd(self):
        test_port = test.TestPort(MockHost())
        server = WPTServe(test_port, '/foo')
        self.assertEqual(
            server._start_cmd,  # pylint: disable=protected-access
            [
                'python',
                '-u',
                '/mock-checkout/third_party/blink/tools/blinkpy/third_party/wpt/wpt/wpt',
                'serve',
                '--config',
                '/mock-checkout/third_party/blink/tools/blinkpy/third_party/wpt/wpt.config.json',
                '--doc_root',
                '/test.checkout/LayoutTests/external/wpt'
            ])

    def test_init_env(self):
        test_port = test.TestPort(MockHost())
        server = WPTServe(test_port, '/foo')
        self.assertEqual(
            server._env,  # pylint: disable=protected-access
            {
                'MOCK_ENVIRON_COPY': '1',
                'PATH': '/bin:/mock/bin',
                'PYTHONPATH': '/mock-checkout/third_party/pywebsocket/src'
            })

    def test_start_with_unkillable_zombie_process(self):
        # Allow asserting about debug logs.
        self.set_logging_level(logging.DEBUG)

        host = MockHost()
        test_port = test.TestPort(host)
        host.filesystem.write_text_file('/log_file_dir/access_log', 'foo')
        host.filesystem.write_text_file('/log_file_dir/error_log', 'foo')
        host.filesystem.write_text_file('/tmp/pidfile', '7')

        server = WPTServe(test_port, '/log_file_dir')
        server._pid_file = '/tmp/pidfile'
        server._spawn_process = lambda: 4
        server._is_server_running_on_all_ports = lambda: True

        # Simulate a process that never gets killed.
        host.executive.check_running_pid = lambda _: True

        server.start()
        self.assertEqual(server._pid, 4)
        self.assertIsNone(host.filesystem.files[server._pid_file])

        # In this case, we'll try to kill the process repeatedly,
        # then give up and just try to start a new process anyway.
        logs = self.logMessages()
        self.assertEqual(len(logs), 43)
        self.assertEqual(
            logs[:2],
            [
                'DEBUG: stale wptserve pid file, pid 7\n',
                'DEBUG: pid 7 is running, killing it\n'
            ])
        self.assertEqual(
            logs[-2:],
            [
                'DEBUG: all ports are available\n',
                'DEBUG: wptserve successfully started (pid = 4)\n'
            ])
