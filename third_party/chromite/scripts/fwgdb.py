# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Connect to a DUT in firmware via remote GDB, install custom GDB commands."""

from __future__ import print_function

import errno
import os
import re
import socket
import time

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import timeout_util

# Need to do this before Servo import
cros_build_lib.AssertInsideChroot()

# pylint: disable=import-error
from servo import client
from servo import multiservo
from servo import terminal_freezer
# pylint: enable=import-error


_SRC_ROOT = os.path.join(constants.CHROOT_SOURCE_ROOT, 'src')
_SRC_DC = os.path.join(_SRC_ROOT, 'platform/depthcharge')
_SRC_VB = os.path.join(_SRC_ROOT, 'platform/vboot_reference')
_SRC_LP = os.path.join(_SRC_ROOT, 'third_party/coreboot/payloads/libpayload')

_PTRN_GDB = 'Ready for GDB connection'
_PTRN_BOARD = 'Starting(?: read-only| read/write)? depthcharge on ([a-z_]+)...'


def ParsePortage(board):
  """Parse some data from portage files. equery takes ages in comparison."""
  with open(os.path.join('/build', board, 'packages/Packages'), 'r') as f:
    for line in f:
      if line[:7] == 'CHOST: ':
        return line[7:].strip()


def ParseArgs(argv):
  """Parse and validate command line arguments."""
  parser = commandline.ArgumentParser(default_log_level='warning')

  parser.add_argument('-b', '--board',
                      help='The board overlay name (auto-detect by default)')
  parser.add_argument('-s', '--symbols',
                      help='Root directory or complete path to symbolized ELF '
                           '(defaults to /build/<BOARD>/firmware)')
  parser.add_argument('-r', '--reboot', choices=['yes', 'no', 'auto'],
                      help='Reboot the DUT before connect (default: reboot if '
                           'the remote and is unreachable)', default='auto')
  parser.add_argument('-e', '--execute', action='append', default=[],
                      help='GDB command to run after connect (can be supplied '
                           'multiple times)')

  parser.add_argument('-n', '--servod-name', dest='name')
  parser.add_argument('--servod-rcfile', default=multiservo.DEFAULT_RC_FILE)
  parser.add_argument('--servod-server')
  parser.add_argument('-p', '--servod-port', type=int, dest='port')
  parser.add_argument('-t', '--tty',
                      help='TTY file to connect to (defaults to cpu_uart_pty)')

  opts = parser.parse_args(argv)
  multiservo.get_env_options(logging, opts)
  if opts.name:
    rc = multiservo.parse_rc(logging, opts.servod_rcfile)
    if opts.name not in rc:
      raise parser.error('%s not in %s' % (opts.name, opts.servod_rcfile))
    if not opts.servod_server:
      opts.servod_server = rc[opts.name]['sn']
    if not opts.port:
      opts.port = rc[opts.name].get('port', client.DEFAULT_PORT)
    if not opts.board and 'board' in rc[opts.name]:
      opts.board = rc[opts.name]['board']
      logging.warning('Inferring board %s from %s; make sure this is correct!',
                      opts.board, opts.servod_rcfile)

  if not opts.servod_server:
    opts.servod_server = client.DEFAULT_HOST
  if not opts.port:
    opts.port = client.DEFAULT_PORT

  return opts


def FindSymbols(firmware_dir, board):
  """Find the symbolized depthcharge ELF (may be supplied by -s flag)."""
  if not firmware_dir:
    firmware_dir = os.path.join(cros_build_lib.GetSysroot(board), 'firmware')
  # Allow overriding the file directly just in case our detection screws up
  if firmware_dir.endswith('.elf'):
    return firmware_dir

  # Very old firmware you might still find on GoldenEye had dev.ro.elf.
  basenames = ['dev.elf', 'dev.ro.elf']
  for basename in basenames:
    path = os.path.join(firmware_dir, 'depthcharge', basename)
    if not os.path.exists(path):
      path = os.path.join(firmware_dir, basename)
    if os.path.exists(path):
      logging.warning('Auto-detected symbol file at %s... make sure that this'
                      ' matches the image on your DUT!', path)
      return path

  raise ValueError('Could not find depthcharge symbol file (dev.elf)! '
                   '(You can use -s to supply it manually.)')


# TODO(jwerner): Fine tune |wait| delay or maybe even make it configurable if
# this causes problems due to load on the host. The callers where this is
# critical should all have their own timeouts now, though, so it's questionable
# whether the delay here is even needed at all anymore.
def ReadAll(fd, wait=0.03):
  """Read from |fd| until no more data has come for at least |wait| seconds."""
  data = ''
  try:
    while True:
      time.sleep(wait)
      new_data = os.read(fd, 4096)
      if not new_data:
        break
      data += new_data
  except OSError as e:
    if e.errno != errno.EAGAIN:
      raise
  logging.debug(data)
  return data


def GdbChecksum(message):
  """Calculate a remote-GDB style checksum."""
  chksum = sum([ord(x) for x in message])
  return ('%.2x' % chksum)[-2:]


def TestConnection(fd):
  """Return True iff there is a resposive GDB stub on the other end of 'fd'."""
  cmd = 'vUnknownCommand'
  for _ in xrange(3):
    os.write(fd, '$%s#%s\n' % (cmd, GdbChecksum(cmd)))
    reply = ReadAll(fd)
    if '+$#00' in reply:
      os.write(fd, '+')
      logging.info('TestConnection: Could successfully connect to remote end.')
      return True
  logging.info('TestConnection: Remote end does not respond.')
  return False


def main(argv):
  opts = ParseArgs(argv)
  servo = client.ServoClient(host=opts.servod_server, port=opts.port)

  if not opts.tty:
    try:
      opts.tty = servo.get('cpu_uart_pty')
    except (client.ServoClientError, socket.error):
      logging.error('Cannot auto-detect TTY file without servod. Use the --tty '
                    'option.')
      raise
  with terminal_freezer.TerminalFreezer(opts.tty):
    fd = os.open(opts.tty, os.O_RDWR | os.O_NONBLOCK)

    data = ReadAll(fd)
    if opts.reboot == 'auto':
      if TestConnection(fd):
        opts.reboot = 'no'
      else:
        opts.reboot = 'yes'

    if opts.reboot == 'yes':
      logging.info('Rebooting DUT...')
      try:
        servo.set('warm_reset', 'on')
        time.sleep(0.1)
        servo.set('warm_reset', 'off')
      except (client.ServoClientError, socket.error):
        logging.error('Cannot reboot without a Servo board. You have to boot '
                      'into developer mode and press CTRL+G manually before '
                      'running fwgdb.')
        raise

      # Throw away old data to avoid confusion from messages before the reboot
      data = ''
      msg = ('Could not reboot into depthcharge!')
      with timeout_util.Timeout(10, msg):
        while not re.search(_PTRN_BOARD, data):
          data += ReadAll(fd)

      msg = ('Could not enter GDB mode with CTRL+G! '
             '(Confirm that you flashed an "image.dev.bin" image to this DUT, '
             'and that you have GBB_FLAG_FORCE_DEV_SWITCH_ON (0x8) set.)')
      with timeout_util.Timeout(5, msg):
        while not re.search(_PTRN_GDB, data):
          # Send a CTRL+G to tell depthcharge to trap into GDB.
          logging.debug('[Ctrl+G]')
          os.write(fd, chr(ord('G') & 0x1f))
          data += ReadAll(fd)

    if not opts.board:
      matches = re.findall(_PTRN_BOARD, data)
      if not matches:
        raise ValueError('Could not auto-detect board! Please use -b option.')
      opts.board = matches[-1]
      logging.info('Auto-detected board as %s from DUT console output.',
                   opts.board)

    if not TestConnection(fd):
      raise IOError('Could not connect to remote end! Confirm that your DUT is '
                    'running in GDB mode on %s.' % opts.tty)

    # Eat up leftover data or it will spill back to terminal
    ReadAll(fd)
    os.close(fd)

    opts.execute.insert(0, 'target remote %s' % opts.tty)
    ex_args = sum([['--ex', cmd] for cmd in opts.execute], [])

    chost = ParsePortage(opts.board)
    logging.info('Launching GDB...')
    cros_build_lib.RunCommand(
        [chost + '-gdb',
         '--symbols', FindSymbols(opts.symbols, opts.board),
         '--directory', _SRC_DC,
         '--directory', _SRC_VB,
         '--directory', _SRC_LP] + ex_args,
        ignore_sigint=True, debug_level=logging.WARNING)
