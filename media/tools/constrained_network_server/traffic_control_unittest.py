#!/usr/bin/env python

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for traffic control library."""
import unittest

import traffic_control


class TrafficControlUnitTests(unittest.TestCase):
  """Unit tests for traffic control."""

  # Stores commands called by the traffic control _Exec function.
  commands = []

  def _ExecMock(self, command, **kwargs):
    """Mocks traffic_control._Exec and adds the command to commands list."""
    cmd_list = [str(x) for x in command]
    self.commands.append(' '.join(cmd_list))
    return ''

  def setUp(self):
    """Resets the commands list and set the _Exec mock function."""
    self.commands = []
    self._old_Exec = traffic_control._Exec
    traffic_control._Exec = self._ExecMock

  def tearDown(self):
    """Resets the _Exec mock function to the original."""
    traffic_control._Exec = self._old_Exec

  def testCreateConstrainedPort(self):
    config = {
        'interface': 'fakeeth',
        'port': 12345,
        'server_port': 8888,
        'bandwidth': 256,
        'latency': 100,
        'loss': 2
    }
    traffic_control.CreateConstrainedPort(config)
    expected = [
        'sudo tc qdisc add dev fakeeth root handle 1: htb',
        'sudo tc class add dev fakeeth parent 1: classid 1:3039 htb rate '
        '256kbit ceil 256kbit',
        'sudo tc qdisc add dev fakeeth parent 1:3039 handle 3039:0 netem loss '
        '2% delay 100ms',
        'sudo tc filter add dev fakeeth protocol ip parent 1: prio 1 u32 match '
        'ip sport 12345 0xffff flowid 1:3039',
        'sudo iptables -t nat -A PREROUTING -i fakeeth -p tcp --dport 12345 -j '
        'REDIRECT --to-port 8888',
        'sudo iptables -t nat -A OUTPUT -p tcp --dport 12345 -j REDIRECT '
        '--to-port 8888'
    ]
    self.assertEqual(expected, self.commands)

  def testCreateConstrainedPortDefaults(self):
    config = {
        'interface': 'fakeeth',
        'port': 12345,
        'server_port': 8888,
        'latency': None
    }
    traffic_control.CreateConstrainedPort(config)
    expected = [
        'sudo tc qdisc add dev fakeeth root handle 1: htb',
        'sudo tc class add dev fakeeth parent 1: classid 1:3039 htb rate '
        '%dkbit ceil %dkbit' % (traffic_control._DEFAULT_MAX_BANDWIDTH_KBIT,
                                traffic_control._DEFAULT_MAX_BANDWIDTH_KBIT),
        'sudo tc qdisc add dev fakeeth parent 1:3039 handle 3039:0 netem',
        'sudo tc filter add dev fakeeth protocol ip parent 1: prio 1 u32 '
        'match ip sport 12345 0xffff flowid 1:3039',
        'sudo iptables -t nat -A PREROUTING -i fakeeth -p tcp --dport 12345 -j '
        'REDIRECT --to-port 8888',
        'sudo iptables -t nat -A OUTPUT -p tcp --dport 12345 -j REDIRECT '
        '--to-port 8888'
    ]
    self.assertEqual(expected, self.commands)

  def testDeleteConstrainedPort(self):
    config = {
        'interface': 'fakeeth',
        'port': 12345,
        'server_port': 8888,
        'bandwidth': 256,
    }
    _old_GetFilterHandleId = traffic_control._GetFilterHandleId
    traffic_control._GetFilterHandleId = lambda interface, port: '800::800'

    try:
      traffic_control.DeleteConstrainedPort(config)
      expected = [
          'sudo tc filter del dev fakeeth protocol ip parent 1:0 handle '
          '800::800 prio 1 u32',
          'sudo tc class del dev fakeeth parent 1: classid 1:3039 htb rate '
          '256kbit ceil 256kbit',
          'sudo iptables -t nat -D PREROUTING -i fakeeth -p tcp --dport 12345 '
          '-j REDIRECT --to-port 8888',
          'sudo iptables -t nat -D OUTPUT -p tcp --dport 12345 -j REDIRECT '
          '--to-port 8888']
      self.assertEqual(expected, self.commands)
    finally:
      traffic_control._GetFilterHandleId = _old_GetFilterHandleId

  def testTearDown(self):
    config = {'interface': 'fakeeth'}

    traffic_control.TearDown(config)
    expected = [
        'sudo tc qdisc del dev fakeeth root',
        'sudo iptables -t nat -F'
    ]
    self.assertEqual(expected, self.commands)

  def testGetFilterHandleID(self):
    # Check seach for handle ID command.
    self.assertRaises(traffic_control.TrafficControlError,
                      traffic_control._GetFilterHandleId, 'fakeeth', 1)
    self.assertEquals(self.commands, ['sudo tc filter list dev fakeeth parent '
                                      '1:'])

    # Check with handle ID available.
    traffic_control._Exec = (lambda command, msg:
      'filter parent 1: protocol ip pref 1 u32 fh 800::800 order 2048 key ht '
      '800 bkt 0 flowid 1:1\nmatch 08ae0000/ffff0000 at 20')
    output = traffic_control._GetFilterHandleId('fakeeth', 1)
    self.assertEqual(output, '800::800')

    # Check with handle ID not available.
    traffic_control._Exec = (lambda command, msg:
      'filter parent 1: protocol ip pref 1 u32 fh 800::800 order 2048 key ht '
      '800 bkt 0 flowid 1:11\nmatch 08ae0000/ffff0000 at 20')
    self.assertRaises(traffic_control.TrafficControlError,
                      traffic_control._GetFilterHandleId, 'fakeeth', 1)

    traffic_control._Exec = lambda command, msg: 'NO ID IN HERE'
    self.assertRaises(traffic_control.TrafficControlError,
                      traffic_control._GetFilterHandleId, 'fakeeth', 1)


if __name__ == '__main__':
  unittest.main()
