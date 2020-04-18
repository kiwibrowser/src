#!/usr/bin/env python

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""End-to-end tests for traffic control library."""
import os
import re
import sys
import unittest

import traffic_control


class TrafficControlTests(unittest.TestCase):
  """System tests for traffic_control functions.

  These tests require root access.
  """
  # A dummy interface name to use instead of real interface.
  _INTERFACE = 'myeth'

  def setUp(self):
    """Setup a dummy interface."""
    # If we update to python version 2.7 or newer we can use setUpClass() or
    # unittest.skipIf().
    if os.getuid() != 0:
      sys.exit('You need root access to run these tests.')

    command = ['ip', 'link', 'add', 'name', self._INTERFACE, 'type', 'dummy']
    traffic_control._Exec(command, 'Error creating dummy interface %s.' %
                          self._INTERFACE)

  def tearDown(self):
    """Teardown the dummy interface and any network constraints on it."""
    # Deleting the dummy interface deletes all associated constraints.
    command = ['ip', 'link', 'del', self._INTERFACE]
    traffic_control._Exec(command)

  def testExecOutput(self):
    output = traffic_control._Exec(['echo', '    Test    '])
    self.assertEqual(output, 'Test')

  def testExecException(self):
    self.assertRaises(traffic_control.TrafficControlError,
                      traffic_control._Exec, command=['ls', '!doesntExist!'])

  def testExecErrorCustomMsg(self):
    try:
      traffic_control._Exec(['ls', '!doesntExist!'], msg='test_msg')
      self.fail('No exception raised for invalid command.')
    except traffic_control.TrafficControlError as e:
      self.assertEqual(e.msg, 'test_msg')

  def testAddRootQdisc(self):
    """Checks adding a root qdisc is successful."""
    config = {'interface': self._INTERFACE}
    root_detail = 'qdisc htb 1: root'
    # Assert no htb root at startup.
    command = ['tc', 'qdisc', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    self.assertFalse(root_detail in output)

    traffic_control._AddRootQdisc(config['interface'])
    output = traffic_control._Exec(command)
    # Assert htb root is added.
    self.assertTrue(root_detail in output)

  def testConfigureClassAdd(self):
    """Checks adding and deleting a class to the root qdisc."""
    config = {
        'interface': self._INTERFACE,
        'port': 12345,
        'server_port': 33333,
        'bandwidth': 2000
    }
    class_detail = ('class htb 1:%x root prio 0 rate %dKbit ceil %dKbit' %
                    (config['port'], config['bandwidth'], config['bandwidth']))

    # Add root qdisc.
    traffic_control._AddRootQdisc(config['interface'])

    # Assert class does not exist prior to adding it.
    command = ['tc', 'class', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    self.assertFalse(class_detail in output)

    # Add class to root.
    traffic_control._ConfigureClass('add', config)

    # Assert class is added.
    command = ['tc', 'class', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    self.assertTrue(class_detail in output)

    # Delete class.
    traffic_control._ConfigureClass('del', config)

    # Assert class is deleted.
    command = ['tc', 'class', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    self.assertFalse(class_detail in output)

  def testAddSubQdisc(self):
    """Checks adding a sub qdisc to existing class."""
    config = {
        'interface': self._INTERFACE,
        'port': 12345,
        'server_port': 33333,
        'bandwidth': 2000,
        'latency': 250,
        'loss': 5
    }
    qdisc_re_detail = ('qdisc netem %x: parent 1:%x .* delay %d.0ms loss %d%%' %
                       (config['port'], config['port'], config['latency'],
                        config['loss']))
    # Add root qdisc.
    traffic_control._AddRootQdisc(config['interface'])

    # Add class to root.
    traffic_control._ConfigureClass('add', config)

    # Assert qdisc does not exist prior to adding it.
    command = ['tc', 'qdisc', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    handle_id_re = re.search(qdisc_re_detail, output)
    self.assertEqual(handle_id_re, None)

    # Add qdisc to class.
    traffic_control._AddSubQdisc(config)

    # Assert qdisc is added.
    command = ['tc', 'qdisc', 'ls', 'dev', config['interface']]
    output = traffic_control._Exec(command)
    handle_id_re = re.search(qdisc_re_detail, output)
    self.assertNotEqual(handle_id_re, None)

  def testAddDeleteFilter(self):
    config = {
        'interface': self._INTERFACE,
        'port': 12345,
        'bandwidth': 2000
    }
    # Assert no filter exists.
    command = ['tc', 'filter', 'list', 'dev', config['interface'], 'parent',
               '1:0']
    output = traffic_control._Exec(command)
    self.assertEqual(output, '')

    # Create the root and class to which the filter will be attached.
    # Add root qdisc.
    traffic_control._AddRootQdisc(config['interface'])

    # Add class to root.
    traffic_control._ConfigureClass('add', config)

    # Add the filter.
    traffic_control._AddFilter(config['interface'], config['port'])
    handle_id = traffic_control._GetFilterHandleId(config['interface'],
                                                   config['port'])
    self.assertNotEqual(handle_id, None)

    # Delete the filter.
    # The output of tc filter list is not None because tc adds default filters.
    traffic_control._DeleteFilter(config['interface'], config['port'])
    self.assertRaises(traffic_control.TrafficControlError,
                      traffic_control._GetFilterHandleId, config['interface'],
                      config['port'])


if __name__ == '__main__':
  unittest.main()
