# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Support for Linux namespaces"""

from __future__ import print_function

import ctypes
import ctypes.util
import errno
import os
import signal
# Note: We avoid cros_build_lib here as that's a "large" module and we want
# to keep this "light" and standalone.  The subprocess usage in here is also
# simple by design -- if it gets more complicated, we should look at using
# the RunCommand helper.
import subprocess
import sys

from chromite.lib import locking
from chromite.lib import osutils
from chromite.lib import process_util
from chromite.lib import proctitle


CLONE_FS = 0x00000200
CLONE_FILES = 0x00000400
CLONE_NEWNS = 0x00020000
CLONE_NEWUTS = 0x04000000
CLONE_NEWIPC = 0x08000000
CLONE_NEWUSER = 0x10000000
CLONE_NEWPID = 0x20000000
CLONE_NEWNET = 0x40000000


def SetNS(fd, nstype):
  """Binding to the Linux setns system call. See setns(2) for details.

  Args:
    fd: An open file descriptor or path to one.
    nstype: Namespace to enter; one of CLONE_*.

  Raises:
    OSError: if setns failed.
  """
  try:
    fp = None
    if isinstance(fd, basestring):
      fp = open(fd)
      fd = fp.fileno()

    libc = ctypes.CDLL(ctypes.util.find_library('c'), use_errno=True)
    if libc.setns(ctypes.c_int(fd), ctypes.c_int(nstype)) != 0:
      e = ctypes.get_errno()
      raise OSError(e, os.strerror(e))
  finally:
    if fp is not None:
      fp.close()


def Unshare(flags):
  """Binding to the Linux unshare system call. See unshare(2) for details.

  Args:
    flags: Namespaces to unshare; bitwise OR of CLONE_* flags.

  Raises:
    OSError: if unshare failed.
  """
  libc = ctypes.CDLL(ctypes.util.find_library('c'), use_errno=True)
  if libc.unshare(ctypes.c_int(flags)) != 0:
    e = ctypes.get_errno()
    raise OSError(e, os.strerror(e))


def _ReapChildren(pid):
  """Reap all children that get reparented to us until we see |pid| exit.

  Args:
    pid: The main child to watch for.

  Returns:
    The wait status of the |pid| child.
  """
  pid_status = 0

  while True:
    try:
      (wpid, status) = os.wait()
      if pid == wpid:
        # Save the status of our main child so we can exit with it below.
        pid_status = status
    except OSError as e:
      if e.errno == errno.ECHILD:
        break
      elif e.errno != errno.EINTR:
        raise

  return pid_status


def _SafeTcSetPgrp(fd, pgrp):
  """Set |pgrp| as the controller of the tty |fd|."""
  try:
    curr_pgrp = os.tcgetpgrp(fd)
  except OSError as e:
    # This can come up when the fd is not connected to a terminal.
    if e.errno == errno.ENOTTY:
      return
    raise

  # We can change the owner only if currently own it.  Otherwise we'll get
  # stopped by the kernel with SIGTTOU and that'll hit the whole group.
  if curr_pgrp == os.getpgrp():
    os.tcsetpgrp(fd, pgrp)


def _ForwardToChildPid(pid, signal_to_forward):
  """Setup a signal handler that forwards the given signal to the given pid."""
  def _ForwardingHandler(signum, _frame):
    os.kill(pid, signum)
  signal.signal(signal_to_forward, _ForwardingHandler)


def CreatePidNs():
  """Start a new pid namespace

  This will launch all the right manager processes.  The child that returns
  will be isolated in a new pid namespace.

  If functionality is not available, then it will return w/out doing anything.

  A note about the processes generated as a result of calling this function:
  You call CreatePidNs() in pid X
  - X launches Pid Y,
    - Pid X will now do nothing but wait for Pid Y to finish and then sys.exit()
      with that return code
    - Y launches Pid Z
      - Pid Y will now do nothing but wait for Pid Z to finish and then
        sys.exit() with that return code
      - **Pid Z returns from CreatePidNs**. So, the caller of this function
        continues in a different process than the one that made the call.
          - All SIGTERM/SIGINT signals are forwarded down from pid X to pid Z to
            handle.
          - SIGKILL will only kill pid X, and leak Pid Y and Z.

  Returns:
    The last pid outside of the namespace. (i.e., pid X)
  """
  first_pid = os.getpid()

  try:
    # First create the namespace.
    Unshare(CLONE_NEWPID)
  except OSError as e:
    if e.errno == errno.EINVAL:
      # For older kernels, or the functionality is disabled in the config,
      # return silently.  We don't want to hard require this stuff.
      return first_pid
    else:
      # For all other errors, abort.  They shouldn't happen.
      raise

  # Used to make sure process groups are in the right state before we try to
  # forward the controlling terminal.
  lock = locking.PipeLock()

  # Now that we're in the new pid namespace, fork.  The parent is the master
  # of it in the original namespace, so it only monitors the child inside it.
  # It is only allowed to fork once too.
  pid = os.fork()
  if pid:
    proctitle.settitle('pid ns', 'external init')

    # We forward termination signals to the child and trust the child to respond
    # sanely. Later, ExitAsStatus propagates the exit status back up.
    _ForwardToChildPid(pid, signal.SIGINT)
    _ForwardToChildPid(pid, signal.SIGTERM)

    # Forward the control of the terminal to the child so it can manage input.
    _SafeTcSetPgrp(sys.stdin.fileno(), pid)

    # Signal our child it can move forward.
    lock.Post()
    del lock

    # Reap the children as the parent of the new namespace.
    process_util.ExitAsStatus(_ReapChildren(pid))
  else:
    # Make sure to unshare the existing mount point if needed.  Some distros
    # create shared mount points everywhere by default.
    try:
      osutils.Mount('none', '/proc', 0, osutils.MS_PRIVATE | osutils.MS_REC)
    except OSError as e:
      if e.errno != errno.EINVAL:
        raise

    # The child needs its own proc mount as it'll be different.
    osutils.Mount('proc', '/proc', 'proc',
                  osutils.MS_NOSUID | osutils.MS_NODEV | osutils.MS_NOEXEC |
                  osutils.MS_RELATIME)

    # Wait for our parent to finish initialization.
    lock.Wait()
    del lock

    # Resetup the locks for the next phase.
    lock = locking.PipeLock()

    pid = os.fork()
    if pid:
      proctitle.settitle('pid ns', 'init')

      # We forward termination signals to the child and trust the child to
      # respond sanely. Later, ExitAsStatus propagates the exit status back up.
      _ForwardToChildPid(pid, signal.SIGINT)
      _ForwardToChildPid(pid, signal.SIGTERM)

      # Now that we're in a new pid namespace, start a new process group so that
      # children have something valid to use.  Otherwise getpgrp/etc... will get
      # back 0 which tends to confuse -- you can't setpgrp(0) for example.
      os.setpgrp()

      # Forward the control of the terminal to the child so it can manage input.
      _SafeTcSetPgrp(sys.stdin.fileno(), pid)

      # Signal our child it can move forward.
      lock.Post()
      del lock

      # Watch all of the children.  We need to act as the master inside the
      # namespace and reap old processes.
      process_util.ExitAsStatus(_ReapChildren(pid))

  # Wait for our parent to finish initialization.
  lock.Wait()
  del lock

  # Create a process group for the grandchild so it can manage things
  # independent of the init process.
  os.setpgrp()

  # The grandchild will return and take over the rest of the sdk steps.
  return first_pid


def CreateNetNs():
  """Start a new net namespace

  We will bring up the loopback interface, but that is all.

  If functionality is not available, then it will return w/out doing anything.
  """
  # The net namespace was added in 2.6.24 and may be disabled in the kernel.
  try:
    Unshare(CLONE_NEWNET)
  except OSError as e:
    if e.errno == errno.EINVAL:
      return
    else:
      # For all other errors, abort.  They shouldn't happen.
      raise

  # Since we've unshared the net namespace, we need to bring up loopback.
  # The kernel automatically adds the various ip addresses, so skip that.
  try:
    subprocess.call(['ip', 'link', 'set', 'up', 'lo'])
  except OSError as e:
    if e.errno == errno.ENOENT:
      print('warning: could not bring up loopback for network; '
            'install the iproute2 package', file=sys.stderr)
    else:
      raise


def SimpleUnshare(mount=True, uts=True, ipc=True, net=False, pid=False):
  """Simpler helper for setting up namespaces quickly.

  If support for any namespace type is not available, we'll silently skip it.

  Args:
    mount: Create a mount namespace.
    uts: Create a UTS namespace.
    ipc: Create an IPC namespace.
    net: Create a net namespace.
    pid: Create a pid namespace.
  """
  # The mount namespace is the only one really guaranteed to exist --
  # it's been supported forever and it cannot be turned off.
  if mount:
    Unshare(CLONE_NEWNS)

  # The UTS namespace was added 2.6.19 and may be disabled in the kernel.
  if uts:
    try:
      Unshare(CLONE_NEWUTS)
    except OSError as e:
      if e.errno != errno.EINVAL:
        pass

  # The IPC namespace was added 2.6.19 and may be disabled in the kernel.
  if ipc:
    try:
      Unshare(CLONE_NEWIPC)
    except OSError as e:
      if e.errno != errno.EINVAL:
        pass

  if net:
    CreateNetNs()

  if pid:
    CreatePidNs()
