# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import subprocess
import tempfile

from telemetry.core import util
from telemetry.internal import forwarders
from telemetry.internal.forwarders import do_nothing_forwarder

import py_utils


class CrOsForwarderFactory(forwarders.ForwarderFactory):

  def __init__(self, cri):
    super(CrOsForwarderFactory, self).__init__()
    self._cri = cri

  def Create(self, local_port, remote_port, reverse=False):
    if self._cri.local:
      return do_nothing_forwarder.DoNothingForwarder(local_port, remote_port)
    else:
      return CrOsSshForwarder(
          self._cri, local_port, remote_port, port_forward=not reverse)


class CrOsSshForwarder(forwarders.Forwarder):

  def __init__(self, cri, local_port, remote_port, port_forward):
    super(CrOsSshForwarder, self).__init__()
    self._cri = cri
    self._proc = None

    if port_forward:
      assert local_port, 'Local port must be given'
    else:
      assert remote_port, 'Remote port must be given'
      if not local_port:
        # Choose an available port on the host.
        local_port = util.GetUnreservedAvailableLocalPort()

    forwarding_args = _ForwardingArgs(
        local_port, remote_port, self.host_ip, port_forward)

    # TODO(crbug.com/793256): Consider avoiding the extra tempfile and
    # read stderr directly from the subprocess instead.
    with tempfile.NamedTemporaryFile() as stderr_file:
      self._proc = subprocess.Popen(
          self._cri.FormSSHCommandLine(['-NT'], forwarding_args,
                                       port_forward=port_forward),
          stdout=subprocess.PIPE,
          stderr=stderr_file,
          stdin=subprocess.PIPE,
          shell=False)
      if not remote_port:
        remote_port = _ReadRemotePort(stderr_file.name)

    self._StartedForwarding(local_port, remote_port)
    py_utils.WaitFor(self._IsConnectionReady, timeout=60)

  def _IsConnectionReady(self):
    return self._cri.IsHTTPServerRunningOnPort(self.remote_port)

  def Close(self):
    if self._proc:
      self._proc.kill()
      self._proc = None
    super(CrOsSshForwarder, self).Close()


def _ReadRemotePort(filename):
  def TryReadingPort(f):
    # When we specify the remote port '0' in ssh remote port forwarding,
    # the remote ssh server should return the port it binds to in stderr.
    # e.g. 'Allocated port 42360 for remote forward to localhost:12345',
    # the port 42360 is the port created remotely and the traffic to the
    # port will be relayed to localhost port 12345.
    line = f.readline()
    tokens = re.search(r'port (\d+) for remote forward to', line)
    return int(tokens.group(1)) if tokens else None

  with open(filename, 'r') as f:
    return py_utils.WaitFor(lambda: TryReadingPort(f), timeout=60)


def _ForwardingArgs(local_port, remote_port, host_ip, port_forward):
  if port_forward:
    arg_format = '-R{remote_port}:{host_ip}:{local_port}'
  else:
    arg_format = '-L{local_port}:{host_ip}:{remote_port}'
  return [arg_format.format(host_ip=host_ip,
                            local_port=local_port,
                            remote_port=remote_port or 0)]
