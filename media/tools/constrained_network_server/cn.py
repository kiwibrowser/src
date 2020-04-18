#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A script for configuring constraint networks.

Sets up a constrained network configuration on a specific port. Traffic on this
port will be redirected to another local server port.

The configuration includes bandwidth, latency, and packet loss.
"""

import collections
import logging
import optparse
import traffic_control

# Default logging is ERROR. Use --verbose to enable DEBUG logging.
_DEFAULT_LOG_LEVEL = logging.ERROR

Dispatcher = collections.namedtuple('Dispatcher', ['dispatch', 'requires_ports',
                                                   'desc'])

# Map of command names to traffic_control functions.
COMMANDS = {
    # Adds a new constrained network configuration.
    'add': Dispatcher(traffic_control.CreateConstrainedPort,
                      requires_ports=True, desc='Add a new constrained port.'),

    # Deletes an existing constrained network configuration.
    'del': Dispatcher(traffic_control.DeleteConstrainedPort,
                      requires_ports=True, desc='Delete a constrained port.'),

    # Deletes all constrained network configurations.
    'teardown': Dispatcher(traffic_control.TearDown,
                           requires_ports=False,
                           desc='Teardown all constrained ports.')
}


def _ParseArgs():
  """Define and parse command-line arguments.

  Returns:
    tuple as (command, configuration):
    command: one of the possible commands to setup, delete or teardown the
             constrained network.
    configuration: a map of constrained network properties to their values.
  """
  parser = optparse.OptionParser()

  indent_first = parser.formatter.indent_increment
  opt_width = parser.formatter.help_position - indent_first

  cmd_usage = []
  for s in COMMANDS:
    cmd_usage.append('%*s%-*s%s' %
                     (indent_first, '', opt_width, s, COMMANDS[s].desc))

  parser.usage = ('usage: %%prog {%s} [options]\n\n%s' %
                  ('|'.join(COMMANDS.keys()), '\n'.join(cmd_usage)))

  parser.add_option('--port', type='int',
                    help='The port to apply traffic control constraints to.')
  parser.add_option('--server-port', type='int',
                    help='Port to forward traffic on --port to.')
  parser.add_option('--bandwidth', type='int',
                    help='Bandwidth of the network in kbit/s.')
  parser.add_option('--latency', type='int',
                    help=('Latency (delay) added to each outgoing packet in '
                          'ms.'))
  parser.add_option('--loss', type='int',
                    help='Packet-loss percentage on outgoing packets. ')
  parser.add_option('--interface', type='string',
                    help=('Interface to setup constraints on. Use "lo" for a '
                          'local client.'))
  parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                    default=False, help='Turn on verbose output.')
  options, args = parser.parse_args()

  _SetLogger(options.verbose)

  # Check a valid command was entered
  if not args or args[0].lower() not in COMMANDS:
    parser.error('Please specify a command {%s}.' % '|'.join(COMMANDS.keys()))
  user_cmd = args[0].lower()

  # Check if required options are available
  if COMMANDS[user_cmd].requires_ports:
    if not (options.port and options.server_port):
      parser.error('Please provide port and server-port values.')

  config = {
      'port': options.port,
      'server_port': options.server_port,
      'interface': options.interface,
      'latency': options.latency,
      'bandwidth': options.bandwidth,
      'loss': options.loss
  }
  return user_cmd, config


def _SetLogger(verbose):
  log_level = _DEFAULT_LOG_LEVEL
  if verbose:
    log_level = logging.DEBUG
  logging.basicConfig(level=log_level, format='%(message)s')


def Main():
  """Get the command and configuration of the network to set up."""
  user_cmd, config = _ParseArgs()

  try:
    COMMANDS[user_cmd].dispatch(config)
  except traffic_control.TrafficControlError as e:
    logging.error('Error: %s\n\nOutput: %s', e.msg, e.error)


if __name__ == '__main__':
  Main()
