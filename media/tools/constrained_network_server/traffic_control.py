# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Traffic control library for constraining the network configuration on a port.

The traffic controller sets up a constrained network configuration on a port.
Traffic to the constrained port is forwarded to a specified server port.
"""

import logging
import os
import re
import subprocess

# The maximum bandwidth limit.
_DEFAULT_MAX_BANDWIDTH_KBIT = 1000000


class TrafficControlError(BaseException):
  """Exception raised for errors in traffic control library.

  Attributes:
    msg: User defined error message.
    cmd: Command for which the exception was raised.
    returncode: Return code of running the command.
    stdout: Output of running the command.
    stderr: Error output of running the command.
  """

  def __init__(self, msg, cmd=None, returncode=None, output=None,
               error=None):
    BaseException.__init__(self, msg)
    self.msg = msg
    self.cmd = cmd
    self.returncode = returncode
    self.output = output
    self.error = error


def CheckRequirements():
  """Checks if permissions are available to run traffic control commands.

  Raises:
    TrafficControlError: If permissions to run traffic control commands are not
    available.
  """
  if os.geteuid() != 0:
    _Exec(['sudo', '-n', 'tc', '-help'],
          msg=('Cannot run \'tc\' command. Traffic Control must be run as root '
               'or have password-less sudo access to this command.'))
    _Exec(['sudo', '-n', 'iptables', '-help'],
          msg=('Cannot run \'iptables\' command. Traffic Control must be run '
               'as root or have password-less sudo access to this command.'))


def CreateConstrainedPort(config):
  """Creates a new constrained port.

  Imposes packet level constraints such as bandwidth, latency, and packet loss
  on a given port using the specified configuration dictionary. Traffic to that
  port is forwarded to a specified server port.

  Args:
    config: Constraint configuration dictionary, format:
      port: Port to constrain (integer 1-65535).
      server_port: Port to redirect traffic on [port] to (integer 1-65535).
      interface: Network interface name (string).
      latency: Delay added on each packet sent (integer in ms).
      bandwidth: Maximum allowed upload bandwidth (integer in kbit/s).
      loss: Percentage of packets to drop (integer 0-100).

  Raises:
    TrafficControlError: If any operation fails. The message in the exception
    describes what failed.
  """
  _CheckArgsExist(config, 'interface', 'port', 'server_port')
  _AddRootQdisc(config['interface'])

  try:
    _ConfigureClass('add', config)
    _AddSubQdisc(config)
    _AddFilter(config['interface'], config['port'])
    _AddIptableRule(config['interface'], config['port'], config['server_port'])
  except TrafficControlError as e:
    logging.debug('Error creating constrained port %d.\nError: %s\n'
                  'Deleting constrained port.', config['port'], e.error)
    DeleteConstrainedPort(config)
    raise e


def DeleteConstrainedPort(config):
  """Deletes an existing constrained port.

  Deletes constraints set on a given port and the traffic forwarding rule from
  the constrained port to a specified server port.

  The original constrained network configuration used to create the constrained
  port must be passed in.

  Args:
    config: Constraint configuration dictionary, format:
      port: Port to constrain (integer 1-65535).
      server_port: Port to redirect traffic on [port] to (integer 1-65535).
      interface: Network interface name (string).
      bandwidth: Maximum allowed upload bandwidth (integer in kbit/s).

  Raises:
    TrafficControlError: If any operation fails. The message in the exception
    describes what failed.
  """
  _CheckArgsExist(config, 'interface', 'port', 'server_port')
  try:
    # Delete filters first so it frees the class.
    _DeleteFilter(config['interface'], config['port'])
  finally:
    try:
      # Deleting the class deletes attached qdisc as well.
      _ConfigureClass('del', config)
    finally:
      _DeleteIptableRule(config['interface'], config['port'],
                         config['server_port'])


def TearDown(config):
  """Deletes the root qdisc and all iptables rules.

  Args:
    config: Constraint configuration dictionary, format:
      interface: Network interface name (string).

  Raises:
    TrafficControlError: If any operation fails. The message in the exception
    describes what failed.
  """
  _CheckArgsExist(config, 'interface')

  command = ['sudo', 'tc', 'qdisc', 'del', 'dev', config['interface'], 'root']
  try:
    _Exec(command, msg='Could not delete root qdisc.')
  finally:
    _DeleteAllIpTableRules()


def _CheckArgsExist(config, *args):
  """Check that the args exist in config dictionary and are not None.

  Args:
    config: Any dictionary.
    *args: The list of key names to check.

  Raises:
    TrafficControlError: If any key name does not exist in config or is None.
  """
  for key in args:
    if key not in config.keys() or config[key] is None:
      raise TrafficControlError('Missing "%s" parameter.' % key)


def _AddRootQdisc(interface):
  """Sets up the default root qdisc.

  Args:
    interface: Network interface name.

  Raises:
    TrafficControlError: If adding the root qdisc fails for a reason other than
    it already exists.
  """
  command = ['sudo', 'tc', 'qdisc', 'add', 'dev', interface, 'root', 'handle',
             '1:', 'htb']
  try:
    _Exec(command, msg=('Error creating root qdisc. '
                        'Make sure you have root access'))
  except TrafficControlError as e:
    # Ignore the error if root already exists.
    if not 'File exists' in e.error:
      raise e


def _ConfigureClass(option, config):
  """Adds or deletes a class and qdisc attached to the root.

  The class specifies bandwidth, and qdisc specifies delay and packet loss. The
  class ID is based on the config port.

  Args:
    option: Adds or deletes a class option [add|del].
    config: Constraint configuration dictionary, format:
      port: Port to constrain (integer 1-65535).
      interface: Network interface name (string).
      bandwidth: Maximum allowed upload bandwidth (integer in kbit/s).
  """
  # Use constrained port as class ID so we can attach the qdisc and filter to
  # it, as well as delete the class, using only the port number.
  class_id = '1:%x' % config['port']
  if 'bandwidth' not in config.keys() or not config['bandwidth']:
    bandwidth = _DEFAULT_MAX_BANDWIDTH_KBIT
  else:
    bandwidth = config['bandwidth']

  bandwidth = '%dkbit' % bandwidth
  command = ['sudo', 'tc', 'class', option, 'dev', config['interface'],
             'parent', '1:', 'classid', class_id, 'htb', 'rate', bandwidth,
             'ceil', bandwidth]
  _Exec(command, msg=('Error configuring class ID %s using "%s" command.' %
                      (class_id, option)))


def _AddSubQdisc(config):
  """Adds a qdisc attached to the class identified by the config port.

  Args:
    config: Constraint configuration dictionary, format:
      port: Port to constrain (integer 1-65535).
      interface: Network interface name (string).
      latency: Delay added on each packet sent (integer in ms).
      loss: Percentage of packets to drop (integer 0-100).
  """
  port_hex = '%x' % config['port']
  class_id = '1:%x' % config['port']
  command = ['sudo', 'tc', 'qdisc', 'add', 'dev', config['interface'], 'parent',
             class_id, 'handle', port_hex + ':0', 'netem']

  # Check if packet-loss is set in the configuration.
  if 'loss' in config.keys() and config['loss']:
    loss = '%d%%' % config['loss']
    command.extend(['loss', loss])
  # Check if latency is set in the configuration.
  if 'latency' in config.keys() and config['latency']:
    latency = '%dms' % config['latency']
    command.extend(['delay', latency])

  _Exec(command, msg='Could not attach qdisc to class ID %s.' % class_id)


def _AddFilter(interface, port):
  """Redirects packets coming to a specified port into the constrained class.

  Args:
    interface: Interface name to attach the filter to (string).
    port: Port number to filter packets with (integer 1-65535).
  """
  class_id = '1:%x' % port

  command = ['sudo', 'tc', 'filter', 'add', 'dev', interface, 'protocol', 'ip',
             'parent', '1:', 'prio', '1', 'u32', 'match', 'ip', 'sport', port,
             '0xffff', 'flowid', class_id]
  _Exec(command, msg='Error adding filter on port %d.' % port)


def _DeleteFilter(interface, port):
  """Deletes the filter attached to the configured port.

  Args:
    interface: Interface name the filter is attached to (string).
    port: Port number being filtered (integer 1-65535).
  """
  handle_id = _GetFilterHandleId(interface, port)
  command = ['sudo', 'tc', 'filter', 'del', 'dev', interface, 'protocol', 'ip',
             'parent', '1:0', 'handle', handle_id, 'prio', '1', 'u32']
  _Exec(command, msg='Error deleting filter on port %d.' % port)


def _GetFilterHandleId(interface, port):
  """Searches for the handle ID of the filter identified by the config port.

  Args:
    interface: Interface name the filter is attached to (string).
    port: Port number being filtered (integer 1-65535).

  Returns:
    The handle ID.

  Raises:
    TrafficControlError: If handle ID was not found.
  """
  command = ['sudo', 'tc', 'filter', 'list', 'dev', interface, 'parent', '1:']
  output = _Exec(command, msg='Error listing filters.')
  # Search for the filter handle ID associated with class ID '1:port'.
  handle_id_re = re.search(
      '([0-9a-fA-F]{3}::[0-9a-fA-F]{3}).*(?=flowid 1:%x\s)' % port, output)
  if handle_id_re:
    return handle_id_re.group(1)
  raise TrafficControlError(('Could not find filter handle ID for class ID '
                             '1:%x.') % port)


def _AddIptableRule(interface, port, server_port):
  """Forwards traffic from constrained port to a specified server port.

  Args:
    interface: Interface name to attach the filter to (string).
    port: Port of incoming packets (integer 1-65535).
    server_port: Server port to forward the packets to (integer 1-65535).
  """
  # Preroute rules for accessing the port through external connections.
  command = ['sudo', 'iptables', '-t', 'nat', '-A', 'PREROUTING', '-i',
             interface, '-p', 'tcp', '--dport', port, '-j', 'REDIRECT',
             '--to-port', server_port]
  _Exec(command, msg='Error adding iptables rule for port %d.' % port)

  # Output rules for accessing the rule through localhost or 127.0.0.1
  command = ['sudo', 'iptables', '-t', 'nat', '-A', 'OUTPUT', '-p', 'tcp',
             '--dport', port, '-j', 'REDIRECT', '--to-port', server_port]
  _Exec(command, msg='Error adding iptables rule for port %d.' % port)


def _DeleteIptableRule(interface, port, server_port):
  """Deletes the iptable rule associated with specified port number.

  Args:
    interface: Interface name to attach the filter to (string).
    port: Port of incoming packets (integer 1-65535).
    server_port: Server port packets are forwarded to (integer 1-65535).
  """
  command = ['sudo', 'iptables', '-t', 'nat', '-D', 'PREROUTING', '-i',
             interface, '-p', 'tcp', '--dport', port, '-j', 'REDIRECT',
             '--to-port', server_port]
  _Exec(command, msg='Error deleting iptables rule for port %d.' % port)

  command = ['sudo', 'iptables', '-t', 'nat', '-D', 'OUTPUT', '-p', 'tcp',
             '--dport', port, '-j', 'REDIRECT', '--to-port', server_port]
  _Exec(command, msg='Error adding iptables rule for port %d.' % port)


def _DeleteAllIpTableRules():
  """Deletes all iptables rules."""
  command = ['sudo', 'iptables', '-t', 'nat', '-F']
  _Exec(command, msg='Error deleting all iptables rules.')


def _Exec(command, msg=None):
  """Executes a command.

  Args:
    command: Command list to execute.
    msg: Message describing the error in case the command fails.

  Returns:
    The standard output from running the command.

  Raises:
    TrafficControlError: If command fails. Message is set by the msg parameter.
  """
  cmd_list = [str(x) for x in command]
  cmd = ' '.join(cmd_list)
  logging.debug('Running command: %s', cmd)

  p = subprocess.Popen(cmd_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  output, error = p.communicate()
  if p.returncode != 0:
    raise TrafficControlError(msg, cmd, p.returncode, output, error)
  return output.strip()
